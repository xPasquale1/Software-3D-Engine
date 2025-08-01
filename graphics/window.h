#pragma once

#include <windows.h>
#include <windowsx.h>
#include <fstream>
#include <d2d1.h>

#include "util.h"
#include "math.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/*
	TODO namespace?
	TODO triangle struct sollte weg... lieber alles per pointer übergeben, dann kann man auch sauber punkte, attribute,... trennen und viel wichtiger die Dreiecke
	haben nicht immer einen pointer der vllt nicht gebraucht wird, da es keine attribute gibt
	TODO Applikation ist ja eigentlich nur noch da wegen Direct2D... vllt kann man einfach init/deinit Funktionen machen und die Anwender müssen sich selbst merken
	was für einen State die Applikation hat
	TODO GPUs arbeiten eigentlich mit Quads, nicht Pixelen direkt, man könnte also einen Lower Res Depthbuffer bauen, welcher für jedes Quad den max. Depthwert speichert
	und dann sucht man den max. Wert für das Quad und vergleicht, damit kann man direkt 4 Pixel auf einmal discarden, falls der Depthtest failed
*/

struct Colorbuffer{
	WORD width = 0;
	WORD height = 0;
	DWORD* data = nullptr;
};

ErrCode createColorbuffer(Colorbuffer& buffer, WORD width, WORD height)noexcept{
	buffer.width = width;
	buffer.height = height;
	buffer.data = alloc<DWORD>(width*height, "Colorbuffer");
	if(buffer.data == nullptr) return ERR_BAD_ALLOC;
	return ERR_SUCCESS; 
}

void destroyColorbuffer(Colorbuffer& buffer)noexcept{
	dealloc(buffer.data);
}

struct Floatbuffer{
	WORD width;
	WORD height;
	float* data = nullptr;
};

ErrCode createFloatbuffer(Floatbuffer& buffer, WORD width, WORD height)noexcept{
	buffer.width = width;
	buffer.height = height;
	buffer.data = alloc<float>(width*height, "Floatbuffer");
	if(buffer.data == nullptr) return ERR_BAD_ALLOC;
	return ERR_SUCCESS;
}

void destroyFloatbuffer(Floatbuffer& buffer)noexcept{
	dealloc(buffer.data);
}

#define WINDOWFLAGSTYPE BYTE
enum WINDOWFLAGS : WINDOWFLAGSTYPE{
	WINDOWFLAG_NONE=0,
	WINDOWFLAG_CLOSE=1,
	WINDOWFLAG_RESIZE=2
};
//Hat viele Attribute die man auch über die win api abrufen könnte, aber diese extra zu speichern macht alles übersichtlicher
struct Window{
	HWND handle = NULL;								//Fensterhandle
	WORD windowWidth = 800;							//Fensterbreite
	WORD windowHeight = 800;						//Fensterhöhe
	Colorbuffer framebuffer;						//Farbbuffer für das Fenster
	WORD pixelSize = 1;								//Größe der Pixel in Bildschirmpixeln
	WINDOWFLAGS flags = WINDOWFLAG_NONE;			//Fensterflags
	ID2D1HwndRenderTarget* renderTarget = nullptr;	//Direct 2D Rendertarget
	std::string windowClassName;					//Ja, jedes Fenster hat seine eigene Klasse... GROSSES TODO
	void* data;										//Generischer Datenpointer der Überschrieben werden darf
};

struct Application{
	ID2D1Factory* factory = nullptr;			//Direct2D Factory
}; Application app;

ErrCode initApp()noexcept{
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &app.factory);
	if(hr){
		std::cerr << hr << std::endl;
		return ERR_APP_INIT;
	}
	return ERR_SUCCESS;
}

//TODO wirft das errors?
ErrCode destroyApp()noexcept{
	app.factory->Release();
	return ERR_SUCCESS;
}

typedef LRESULT (*window_callback_function)(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK default_window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
ErrCode createWindow(HINSTANCE hInstance, LONG windowWidth, LONG windowHeight, LONG x, LONG y, WORD pixelSize, Window& window, const char* name = "Window", window_callback_function callback = default_window_callback, HWND parentWindow = NULL)noexcept{
	//Erstelle Fenster Klasse
	WNDCLASS window_class = {};
	window_class.hInstance = hInstance;
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	std::string className = "Window-Class" + std::to_string((unsigned long long)&window);	//TODO meh...
	window_class.lpszClassName = className.c_str();
	window_class.lpfnWndProc = callback;

	window.windowClassName = className;
	//Registriere Fenster Klasse
	if(!RegisterClass(&window_class)){
		std::cerr << "Register-Class: " << GetLastError() << std::endl;
		return ERR_CREATE_WINDOW;
	}

	RECT rect;
    rect.top = 0;
    rect.bottom = windowHeight;
    rect.left = 0;
    rect.right = windowWidth;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	DWORD w = rect.right - rect.left;
	DWORD h = rect.bottom - rect.top;

	//Erstelle das Fenster
	window.handle = CreateWindow(window_class.lpszClassName, name, WS_VISIBLE | WS_OVERLAPPEDWINDOW, x, y, w, h, parentWindow, NULL, hInstance, NULL);
	if(window.handle == NULL){
		std::cerr << "Create-Window: "<< GetLastError() << std::endl;
		return ERR_CREATE_WINDOW;
	}

	//TODO idk ob das so ok ist, win32 doku sagt nicht viel darüber aus... aber angeblich ist USERDATA genau für sowas gedacht und es sollte auch nie überschrieben werden...
	SetWindowLongPtr(window.handle, GWLP_USERDATA, (LONG_PTR)&window);

	D2D1_RENDER_TARGET_PROPERTIES properties = {};
	properties.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
	properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	properties.dpiX = 96;
	properties.dpiY = 96;
	properties.usage = D2D1_RENDER_TARGET_USAGE_NONE;
	properties.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
	D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProperties = {};
	hwndProperties.hwnd = window.handle;
	hwndProperties.pixelSize = D2D1::SizeU(windowWidth, windowHeight);
	hwndProperties.presentOptions = D2D1_PRESENT_OPTIONS_IMMEDIATELY;
    HRESULT hr = app.factory->CreateHwndRenderTarget(properties, hwndProperties, &window.renderTarget);
    if(hr){
		std::cerr << "Create-Rendertarget: "<< hr << std::endl;
		return ERR_INIT_RENDER_TARGET;
    }

	WORD buffer_width = windowWidth/pixelSize;
	WORD buffer_height = windowHeight/pixelSize;
	window.pixelSize = pixelSize;
	createColorbuffer(window.framebuffer, buffer_width, buffer_height);
	window.windowWidth = windowWidth;
	window.windowHeight = windowHeight;
	return ERR_SUCCESS;
}

ErrCode destroyWindow(Window& window)noexcept{
	if(!UnregisterClassA(window.windowClassName.c_str(), NULL)){
		std::cerr << GetLastError() << std::endl;
		return ERR_GENERIC_ERROR;
	}
	DestroyWindow(window.handle);
	destroyColorbuffer(window.framebuffer);
	return ERR_SUCCESS;
}

ErrCode setWindowFlag(Window& window, WINDOWFLAGS state)noexcept{
	window.flags = (WINDOWFLAGS)(window.flags | state);
	return ERR_SUCCESS;
}
ErrCode resetWindowFlag(Window& window, WINDOWFLAGS state)noexcept{
	window.flags = (WINDOWFLAGS)(window.flags & ~state);
	return ERR_SUCCESS;
}
bool getWindowFlag(Window& window, WINDOWFLAGS state)noexcept{
	return (window.flags & state);
}

//TODO Sollte ERRCODE zurückgeben und WINDOWFLAG als Referenzparameter übergeben bekommen
//Gibt den nächsten Zustand des Fensters zurück und löscht diesen anschließend, Anwendung z.B. while(state = getNextWindowState() != WINDOW_NONE)...
WINDOWFLAGS getNextWindowState(Window& window)noexcept{
	WINDOWFLAGS flag = (WINDOWFLAGS)(window.flags & -window.flags);
	window.flags = (WINDOWFLAGS)(window.flags & ~flag);
	return flag;
}

ErrCode resizeWindow(Window& window, WORD width, WORD height, WORD pixel_size)noexcept{
	window.windowWidth = width;
	window.windowHeight = height;
	window.pixelSize = pixel_size;
	destroyColorbuffer(window.framebuffer);
	WORD bufferWidth = width/pixel_size;
	WORD bufferHeight = height/pixel_size;
	ErrCode code;
	if((code = createColorbuffer(window.framebuffer, bufferWidth, bufferHeight)) != ERR_SUCCESS) return code;
	window.renderTarget->Resize({width, height});
	return ERR_SUCCESS;
}

//TODO anstatt solch eine komplexe Funktion in createWindow rein zu geben, könnte man seine eigene schreiben mit Window* und uMsg,... als Parameter
//und diese default funktion ruft diese dann optional nur auf
LRESULT CALLBACK default_window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if(window == nullptr) return DefWindowProc(hwnd, uMsg, wParam, lParam);	//TODO das ist ein Fehler, wie melden aber?
	switch(uMsg){
		case WM_DESTROY:{
			ErrCheck(setWindowFlag(*window, WINDOWFLAG_CLOSE), "setze close Fensterstatus");
			break;
		}
		case WM_SIZE:{
			UINT width = LOWORD(lParam);
			UINT height = HIWORD(lParam);
			if(!width || !height) break;
			ErrCheck(setWindowFlag(*window, WINDOWFLAG_RESIZE), "setzte resize Fensterstatus");
			ErrCheck(resizeWindow(*window, width, height, 1), "Fenster skalieren");
			break;
		}
		case WM_LBUTTONDOWN:{
			setButton(mouse, MOUSEBUTTON_LMB);
			break;
		}
		case WM_LBUTTONUP:{
			resetButton(mouse, MOUSEBUTTON_LMB);
			break;
		}
		case WM_RBUTTONDOWN:{
			setButton(mouse, MOUSEBUTTON_RMB);
			break;
		}
		case WM_RBUTTONUP:{
			resetButton(mouse, MOUSEBUTTON_RMB);
			break;
		}
		case WM_MOUSEMOVE:{
			mouse.pos.x = GET_X_LPARAM(lParam)/window->pixelSize;
			mouse.pos.y = GET_Y_LPARAM(lParam)/window->pixelSize;
			break;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void getMessages(Window& window)noexcept{
	MSG msg;
	while(PeekMessage(&msg, window.handle, 0, 0, PM_REMOVE)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

constexpr DWORD RGBA(BYTE r, BYTE g, BYTE b, BYTE a=255)noexcept{return DWORD(b|g<<8|r<<16|a<<24);}
constexpr BYTE A(DWORD color)noexcept{return BYTE(color>>24);}
constexpr BYTE R(DWORD color)noexcept{return BYTE(color>>16);}
constexpr BYTE G(DWORD color)noexcept{return BYTE(color>>8);}
constexpr BYTE B(DWORD color)noexcept{return BYTE(color);}

constexpr DWORD A(DWORD color, DWORD a)noexcept{return (color&0x00FFFFFF)|a<<24;}
constexpr DWORD R(DWORD color, DWORD r)noexcept{return (color&0xFF00FFFF)|r<<16;}
constexpr DWORD G(DWORD color, DWORD g)noexcept{return (color&0xFFFF00FF)|g<<8;}
constexpr DWORD B(DWORD color, DWORD b)noexcept{return (color&0xFFFFFF00)|b;}

ErrCode clearWindow(Window& window, DWORD color = RGBA(0, 0, 0))noexcept{
	WORD buffer_width = window.windowWidth/window.pixelSize;
	WORD buffer_height = window.windowHeight/window.pixelSize;
	for(DWORD i=0; i < window.framebuffer.width*window.framebuffer.height; ++i) window.framebuffer.data[i] = color;
	return ERR_SUCCESS;
}

ErrCode drawWindow(Window& window)noexcept{
	if(window.windowWidth == 0 || window.windowHeight == 0) return ERR_GENERIC_ERROR;
	ID2D1Bitmap* bitmap;
	D2D1_BITMAP_PROPERTIES properties = {};
	properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	properties.dpiX = 96;
	properties.dpiY = 96;
	window.renderTarget->BeginDraw();
	HRESULT hr = window.renderTarget->CreateBitmap({window.framebuffer.width, window.framebuffer.height}, window.framebuffer.data, window.framebuffer.width*4, properties, &bitmap);
	if(hr){
		std::cerr << hr << std::endl;
		exit(-2);	//TODO uhhh hallo was?
	}
	window.renderTarget->DrawBitmap(bitmap, D2D1::RectF(0, 0, window.windowWidth, window.windowHeight), 1, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, D2D1::RectF(0, 0, window.framebuffer.width, window.framebuffer.height));
	bitmap->Release();
	window.renderTarget->EndDraw();
	return ERR_SUCCESS;
}

ErrCode drawRectangle(const Colorbuffer& buffer, WORD x, WORD y, WORD dx, WORD dy, DWORD color)noexcept{
	for(WORD i=y; i < y+dy; ++i){
		for(WORD j=x; j < x+dx; ++j){
			buffer.data[i*buffer.width+j] = color;
		}
	}
	return ERR_SUCCESS;
}

ErrCode drawLine(const Colorbuffer& buffer, WORD start_x, WORD start_y, WORD end_x, WORD end_y, DWORD color)noexcept{
	int dx = end_x-start_x;
	int dy = end_y-start_y;
	int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
	float xinc = dx/(float)steps;
	float yinc = dy/(float)steps;
	float x = start_x+0.5f;
	float y = start_y+0.5f;
	for(int i = 0; i <= steps; ++i){
		buffer.data[(int)y*buffer.width+(int)x] = color;
		x += xinc;
		y += yinc;
	}
	return ERR_SUCCESS;
}

ErrCode drawCircle(const Colorbuffer& buffer, WORD x, WORD y, WORD radius, DWORD color)noexcept{
	for(SWORD i=-radius; i < radius; ++i){
		for(SWORD j=-radius; j < radius; ++j){
			if(j*j+i*i <= radius*radius) buffer.data[(i+y)*buffer.width+j+x] = color;
		}
	}
	return ERR_SUCCESS;
}

ErrCode drawCircleSave(const Colorbuffer& buffer, WORD x, WORD y, WORD radius, DWORD color)noexcept{
	DWORD totalBufferSize = buffer.width*buffer.height;
	for(SWORD i=-radius; i < radius; ++i){
		for(SWORD j=-radius; j < radius; ++j){
			if(j*j+i*i <= radius*radius){
				DWORD dy = i+y;
				DWORD dx = j+x;
				if(dx >= buffer.width || dy >= buffer.height) continue;
				buffer.data[dy*buffer.width+dx] = color;
			}
		}
	}
	return ERR_SUCCESS;
}

struct Image{
	WORD width = 0;			//x-Dimension
	WORD height = 0;		//y-Dimension
	DWORD* data = nullptr;
};

ErrCode createImage(Image& image, WORD width, WORD height)noexcept{
	image.data = alloc<DWORD>(width*height, "Imagebuffer");
	if(!image.data) return ERR_BAD_ALLOC;
	image.width = width;
	image.height = height;
	return ERR_SUCCESS;
}

ErrCode loadImage(const char* name, Image& image)noexcept{
	std::fstream file;
	file.open(name, std::ios::in | std::ios::binary);
	if(!file.is_open()) return ERR_FILE_NOT_FOUND;
	file.read((char*)&image.width, 2);
	file.read((char*)&image.height, 2);
	image.data = alloc<DWORD>(image.width*image.height, "Imagebuffer");
	if(!image.data) return ERR_BAD_ALLOC;
	BYTE val[4];
	for(DWORD i=0; i < image.width*image.height; ++i){
		file.read((char*)&val[0], 1);
		file.read((char*)&val[1], 1);
		file.read((char*)&val[2], 1);
		file.read((char*)&val[3], 1);
		image.data[i] = RGBA(val[0], val[1], val[2], val[3]);
	}
	file.close();
	return ERR_SUCCESS;
}

ErrCode loadPng(const char* filename, Image& image)noexcept{
    int width, height, channels;
    unsigned char* img = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);
    if(!img) return ERR_FILE_NOT_FOUND;
    image.width = width;
    image.height = height;
    image.data = alloc<DWORD>(width*height);
    for(DWORD i=0; i < width*height*4; i+=4){
        DWORD color = RGBA(img[i], img[i+1], img[i+2], img[i+3]);
        image.data[i/4] = color;
    }
    stbi_image_free(img);
    return ERR_SUCCESS;
}

ErrCode createBlankImage(Image& image, WORD width, WORD height, DWORD color)noexcept{
	ErrCode code;
	code = createImage(image, width, height);
	if(code != ERR_SUCCESS) return code;
	for(DWORD i=0; i < width*height; ++i) image.data[i] = color;
	return ERR_SUCCESS;
}

//TODO sollte ErrCode zurückgeben
Image generateMipMap(Image& image)noexcept{
	Image out;
	out.width = image.width/2;
	out.height = image.height/2;
	out.data = alloc<DWORD>(out.width*out.height, "Imagebuffer");	//TODO Fehler testen
	for(WORD y=0; y < image.height; y+=2){
		for(WORD x=0; x < image.width; x+=2){
			out.data[(y/2)*out.width+(x/2)] = image.data[y*image.width+x];
		}
	}
	return out;
}

void destroyImage(Image& image)noexcept{
	dealloc(image.data);
	image.data = nullptr;
}

//x und y von 0 - 1, keine Tests auf Überlauf!
DWORD getImage(Image& image, float x, float y)noexcept{
	DWORD ry = y*image.height;
	DWORD rx = x*(image.width-1);
	return image.data[ry*image.width+rx];
}

void rotateImage180(Image& image)noexcept{
	DWORD revIdx = image.width*image.height-1;
	for(DWORD i=0; i < image.width*image.height/2; ++i){
		std::swap(image.data[i], image.data[revIdx--]);
	}
}

void flipImageVertically(Image& image)noexcept{
	WORD revY = image.height-1;
	for(WORD y=0; y < image.height/2; ++y, --revY){
		for(WORD x=0; x < image.width; ++x){
			std::swap(image.data[y*image.width+x], image.data[revY*image.width+x]);
		}
	}
}

//Kopiert das gesamte Image in den angegebenen Bereich von start_x bis end_x und start_y bis end_y
//TODO Warum hat das ErrCode?
//TODO Kopiere nicht das gesamte Image, sondern auch das sollte man angeben können
//TODO up-/downscaling methoden wie nearest, bilinear,...
ErrCode copyImageToColorbuffer(Colorbuffer& buffer, const Image& image, WORD start_x, WORD start_y, WORD end_x, WORD end_y)noexcept{
	for(int y=start_y; y < end_y; ++y){
		WORD ry = (float)(y-start_y)/(end_y-start_y)*image.height;
		for(int x=start_x; x < end_x; ++x){
			WORD rx = (float)(x-start_x)/(end_x-start_x)*image.width;
			DWORD color = image.data[ry*image.width+rx];
			if(A(color) > 0) buffer.data[y*buffer.width+x] = color;
		}
	}
	return ERR_SUCCESS;
}

//Funktion testet ob jeder pixel im gültigen Fensterbereich liegt! idx ist der window index
//TODO Warum hat das n ErrCode?
//TODO ist das wirklich nötig eine ganze extra Funktion dafür zu machen?
ErrCode copyImageToColorbufferSave(Colorbuffer& buffer, Image& image, WORD start_x, WORD start_y, WORD end_x, WORD end_y)noexcept{
	for(int y=start_y; y < end_y; ++y){
		if(y < 0 || y >= (int)buffer.height) continue;
		WORD ry = (float)(y-start_y)/(end_y-start_y)*image.height;
		for(int x=start_x; x < end_x; ++x){
			if(x < 0 || x >= (int)buffer.width) continue;
			WORD rx = (float)(x-start_x)/(end_x-start_x)*image.width;
			DWORD color = image.data[ry*image.width+rx];
			if(A(color) > 0) buffer.data[y*buffer.width+x] = color;
		}
	}
	return ERR_SUCCESS;
}

struct Font{
	Image image;
	ivec2 char_size;		//Größe eines Symbols im Image
	WORD font_size = 12;	//Größe der Symbole in Pixel
	BYTE char_sizes[96];	//Größe der Symbole in x-Richtung
};

//Zerstört die Font und alle weiteren allokierten Elemente
void destroyFont(Font& font)noexcept{
	destroyImage(font.image);
}

ErrCode loadFont(const char* path, Font& font, ivec2 char_size)noexcept{
	ErrCode code;
	font.char_size = char_size;
	if((code = ErrCheck(loadImage(path, font.image), "font image laden")) != ERR_SUCCESS){
		return code;
	}
	//Lese max x von jedem Zeichen
	font.char_sizes[0] = font.char_size.x/2;	//Leerzeichen
	for(DWORD i=1; i < 96; ++i){
		DWORD x_max = 0;
		for(DWORD y=(i/16)*char_size.y; y < (i/16)*char_size.y+char_size.y; ++y){
			for(DWORD x=(i%16)*char_size.x; x < (i%16)*char_size.x+char_size.x; ++x){
				if(A(font.image.data[y*font.image.width+x]) > 0 && x > x_max){
					x_max = x;
				}
			}
		}
		font.char_sizes[i] = x_max - (i%16)*char_size.x + 10;
	}
	return ERR_SUCCESS;
}

//Gibts zurück wie viele Pixel der Text unter der gegebenen Font benötigt
//TODO font könnte nullptr sein -> INVALIDHANDLEERRORS?
WORD getStringFontSize(Font& font, std::string& text)noexcept{
	float div = (float)font.char_size.y/font.font_size;
	WORD offset = 0;
	for(size_t i=0; i < text.size(); ++i){
		BYTE idx = (text[i]-32);
		offset += font.char_sizes[idx]/div;
	}
	return offset;
}

//Gibt zurück wie breit das Symbol war das gezeichnet wurde
//TODO Errors wegen invalidem Symbol etc.? übergebe symbol größe als Referenz Parameter
DWORD drawFontChar(Colorbuffer& buffer, Font& font, char symbol, DWORD start_x, DWORD start_y)noexcept{
	DWORD idx = (symbol-32);
	if(idx > 223) return 0;
	float div = (float)font.char_size.y/font.font_size;
	DWORD end_x = start_x+font.char_sizes[idx]/div;
	DWORD end_y = start_y+font.font_size;
	DWORD x_offset = (idx%16)*font.char_size.x;
	DWORD y_offset = (idx/16)*font.char_size.y;
	for(DWORD y=start_y; y < end_y; ++y){
		float scaled_y = (float)(y-start_y)/(end_y-start_y);
		for(DWORD x=start_x; x < end_x; ++x){
			DWORD ry = scaled_y*font.char_size.y;
			DWORD rx = (float)(x-start_x)/(end_x-start_x)*(font.char_sizes[idx]-1);
			DWORD color = font.image.data[(ry+y_offset)*font.image.width+rx+x_offset];
			if(A(color) > 0) buffer.data[y*buffer.width+x] = color;
		}
	}
	return end_x-start_x;
}

//Gibt zurück, wie breit der String zu zeichnen war, String muss \0 terminiert sein!
//TODO Errors? meh, if window wird evtl bei jedem Aufruf von drawFontChar gemacht
DWORD drawFontString(Colorbuffer& buffer, Font& font, const char* string, DWORD start_x, DWORD start_y)noexcept{
	DWORD offset = 0;
	DWORD idx = 0;
	while(string[idx] != '\0'){
		offset += drawFontChar(buffer, font, string[idx++], start_x+offset, start_y);
	}
	return offset;
}

ErrCode _defaultButtonEvent(void*)noexcept{return ERR_SUCCESS;}
enum BUTTONFLAGS{
	BUTTONFLAG_NONE=0,
	BUTTONFLAG_VISIBLE=1,
	BUTTONFLAG_CAN_HOVER=2,
	BUTTONFLAG_HOVER=4,
	BUTTONFLAG_PRESSED=8,
	BUTTONFLAG_TEXT_CENTER=16,
	BUTTONFLAG_DISABLED=32
};
struct Button{
	ErrCode (*event)(void*)noexcept = _defaultButtonEvent;
	std::string text;
	Image* image = nullptr;
	Image* disabledImage = nullptr;
	ivec2 pos = {0, 0};
	ivec2 repos = {0, 0};
	ivec2 size = {50, 10};
	ivec2 resize = {55, 11};
	BYTE flags = BUTTONFLAG_VISIBLE | BUTTONFLAG_CAN_HOVER | BUTTONFLAG_TEXT_CENTER;
	DWORD color = RGBA(120, 120, 120);
	DWORD hover_color = RGBA(120, 120, 255);
	DWORD textcolor = RGBA(180, 180, 180);
	DWORD disabledColor = RGBA(90, 90, 90);
	WORD textsize = 16;
	void* data = nullptr;
};

void destroyButton(Button& button)noexcept{
	destroyImage(*button.image);
}

constexpr void setButtonFlag(Button& button, BUTTONFLAGS flag)noexcept{button.flags |= flag;}
constexpr void resetButtonFlag(Button& button, BUTTONFLAGS flag)noexcept{button.flags &= ~flag;}
constexpr bool getButtonFlag(Button& button, BUTTONFLAGS flag)noexcept{return (button.flags & flag);}
//TODO kann bestimmt besser geschrieben werden... und ErrCheck aufs Event sollte mit einem BUTTONSTATE entschieden werden
void buttonsClicked(Button* buttons, WORD button_count)noexcept{
	for(WORD i=0; i < button_count; ++i){
		Button& b = buttons[i];
		if(!getButtonFlag(b, BUTTONFLAG_VISIBLE) || getButtonFlag(b, BUTTONFLAG_DISABLED)) continue;
		ivec2 delta = {mouse.pos.x - b.pos.x, mouse.pos.y - b.pos.y};
		if(delta.x >= 0 && delta.x <= b.size.x && delta.y >= 0 && delta.y <= b.size.y){
			if(getButtonFlag(b, BUTTONFLAG_CAN_HOVER)) b.flags |= BUTTONFLAG_HOVER;
			if(getButton(mouse, MOUSEBUTTON_LMB) && !getButtonFlag(b, BUTTONFLAG_PRESSED)){
				ErrCheck(b.event(b.data), "Button-Event");
				b.flags |= BUTTONFLAG_PRESSED;
			}
			else if(!getButton(mouse, MOUSEBUTTON_LMB)) b.flags &= ~BUTTONFLAG_PRESSED;
		}else if(getButtonFlag(b, BUTTONFLAG_CAN_HOVER)){
			b.flags &= ~BUTTONFLAG_HOVER;
		}
	}
}

void drawButtons(Colorbuffer& buffer, Font& font, Button* buttons, WORD button_count)noexcept{
	for(WORD i=0; i < button_count; ++i){
		Button& b = buttons[i];
		if(!getButtonFlag(b, BUTTONFLAG_VISIBLE)) continue;
		if(getButtonFlag(b, BUTTONFLAG_DISABLED)){
			if(b.disabledImage == nullptr)
				drawRectangle(buffer, b.pos.x, b.pos.y, b.size.x, b.size.y, b.disabledColor);
			else
				copyImageToColorbuffer(buffer, *b.disabledImage, b.pos.x, b.pos.y, b.pos.x+b.size.x, b.pos.y+b.size.y);
		}else if(b.image == nullptr){
			if(getButtonFlag(b, BUTTONFLAG_CAN_HOVER) && getButtonFlag(b, BUTTONFLAG_HOVER))
				drawRectangle(buffer, b.pos.x, b.pos.y, b.size.x, b.size.y, b.hover_color);
			else
				drawRectangle(buffer, b.pos.x, b.pos.y, b.size.x, b.size.y, b.color);
		}else{
			if(getButtonFlag(b, BUTTONFLAG_CAN_HOVER) && getButtonFlag(b, BUTTONFLAG_HOVER))
				copyImageToColorbuffer(buffer, *b.image, b.repos.x, b.repos.y, b.repos.x+b.resize.x, b.repos.y+b.resize.y);
			else
				copyImageToColorbuffer(buffer, *b.image, b.pos.x, b.pos.y, b.pos.x+b.size.x, b.pos.y+b.size.y);
		}
		if(getButtonFlag(b, BUTTONFLAG_TEXT_CENTER)){
			DWORD offset = 0;
			WORD tmp_font_size = font.font_size;
			font.font_size = b.textsize;
			WORD str_size = getStringFontSize(font, b.text);
			for(size_t i=0; i < b.text.size(); ++i){
				offset += drawFontChar(buffer, font, b.text[i], b.pos.x+offset+b.size.x/2-str_size/2, b.pos.y+b.size.y/2-b.textsize/2);
			}
			font.font_size = tmp_font_size;
		}else{
			DWORD offset = 0;
			WORD tmp_font_size = font.font_size;
			font.font_size = b.textsize;
			for(size_t i=0; i < b.text.size(); ++i){
				offset += drawFontChar(buffer, font, b.text[i], b.pos.x+offset, b.pos.y+b.size.y/2-b.textsize/2);
			}
			font.font_size = tmp_font_size;
		}
	}
}

void updateButtons(Colorbuffer& buffer, Font& font, Button* buttons, WORD button_count)noexcept{
	buttonsClicked(buttons, button_count);
	drawButtons(buffer, font, buttons, button_count);
}

struct Label{
	std::string text;
	ivec2 pos = {0, 0};
	DWORD textcolor = RGBA(180, 180, 180);
	WORD text_size = 2;
};

enum MENUFLAGS{
	MENUFLAG_OPEN=1,
	MENUFLAG_OPEN_TOGGLE=2
};
//TODO sollte alles dynamisch hinzugefügt werden
#define MAX_BUTTONS 10
#define MAX_STRINGS 20
#define MAX_IMAGES 5
struct Menu{
	Image images[MAX_IMAGES];		//Sind für die Buttons
	BYTE imageCount = 0;
	Button buttons[MAX_BUTTONS];
	BYTE buttonCount = 0;
	BYTE flags = MENUFLAG_OPEN;
	ivec2 pos = {};					//TODO Position in Bildschirmpixelkoordinaten
	ivec2 size = {};				//TODO Größe in Bildschirmkoordinaten
	Label labels[MAX_STRINGS];
	BYTE labelCount = 0;
};

void destroyMenu(Menu& menu)noexcept{
	for(WORD i=0; i < menu.imageCount; ++i){
		destroyImage(menu.images[i]);
	}
}

constexpr void setMenuFlag(Menu& menu, MENUFLAGS flag)noexcept{menu.flags |= flag;}
constexpr void resetMenuFlag(Menu& menu, MENUFLAGS flag)noexcept{menu.flags &= ~flag;}
constexpr bool getMenuFlag(Menu& menu, MENUFLAGS flag)noexcept{return (menu.flags&flag);}

void updateMenu(Colorbuffer& buffer, Menu& menu, Font& font)noexcept{
	if(getMenuFlag(menu, MENUFLAG_OPEN)){
		updateButtons(buffer, font, menu.buttons, menu.buttonCount);
		for(WORD i=0; i < menu.labelCount; ++i){
			Label& label = menu.labels[i];
			DWORD offset = 0;
			for(size_t j=0; j < label.text.size(); ++j){
				WORD tmp = font.font_size;
				font.font_size = label.text_size;
				offset += drawFontChar(buffer, font, label.text[j], label.pos.x+offset, label.pos.y);
				font.font_size = tmp;
			}
		}
	}
}

enum FLOATSLIDERFLAGS{
	FLOATSLIDERFLAG_NONE=0,
	FLOATSLIDERFLAG_CAPTURED=1
};

struct FloatSlider{
	ivec2 pos = {0, 0};
	ivec2 size = {100, 3};
	WORD sliderRadius = 6;
	WORD sliderPos = 0;
	DWORD color = RGBA(180, 180, 180);
	BYTE flags = FLOATSLIDERFLAG_NONE;
	float minValue = 0;
	float maxValue = 100;
	float value = 0;
};

constexpr void setSliderFlag(FloatSlider& slider, FLOATSLIDERFLAGS flag)noexcept{slider.flags |= flag;}
constexpr void resetSliderFlag(FloatSlider& slider, FLOATSLIDERFLAGS flag)noexcept{slider.flags &= ~flag;}
constexpr bool getSliderFlag(FloatSlider& slider, FLOATSLIDERFLAGS flag)noexcept{return (slider.flags&flag);}

WORD getFloatSliderPosFromValue(FloatSlider& slider){
	return (slider.value-slider.minValue)/(slider.maxValue-slider.minValue)*slider.size.x; 
}

void updateFloatSliders(Colorbuffer& buffer, Font& font, FloatSlider* sliders, WORD sliderCount)noexcept{
	for(WORD i=0; i < sliderCount; ++i){
		if(getButton(mouse, MOUSEBUTTON_LMB) && !getButton(mouse, MOUSEBUTTON_PREV_LMB)){
			WORD x = mouse.pos.x-sliders[i].pos.x-sliders[i].sliderPos+sliders[i].sliderRadius;
			WORD y = mouse.pos.y-sliders[i].pos.y+sliders[i].sliderRadius;
			if(x <= sliders[i].sliderRadius*2 && y <= sliders[i].sliderRadius*2) setSliderFlag(sliders[i], FLOATSLIDERFLAG_CAPTURED);
		}
		if(!getButton(mouse, MOUSEBUTTON_LMB)) resetSliderFlag(sliders[i], FLOATSLIDERFLAG_CAPTURED);
		if(getSliderFlag(sliders[i], FLOATSLIDERFLAG_CAPTURED)){
			sliders[i].sliderPos = clamp(mouse.pos.x-sliders[i].pos.x, 0, sliders[i].size.x);
			sliders[i].value = (sliders[i].sliderPos*(sliders[i].maxValue-sliders[i].minValue))/sliders[i].size.x+sliders[i].minValue;
		}
		drawRectangle(buffer, sliders[i].pos.x, sliders[i].pos.y, sliders[i].size.x, sliders[i].size.y, sliders[i].color);
		drawCircle(buffer, sliders[i].pos.x+sliders[i].sliderPos, sliders[i].pos.y+sliders[i].size.y/2, sliders[i].sliderRadius, sliders[i].color);
		drawFontString(buffer, font, floatToString(sliders[i].value).c_str(), sliders[i].pos.x+sliders[i].size.x+sliders[i].sliderRadius, sliders[i].pos.y+sliders[i].size.y/2-font.font_size/2);
	}
}

enum CHECKBOXFLAGS{
	CHECKBOXFLAG_NONE=0,
	CHECKBOXFLAG_CHECKED=1
};

ErrCode _defaultCheckboxEvent(void*)noexcept{return ERR_SUCCESS;}
struct Checkbox{
	ivec2 pos = {0, 0};
	ivec2 size = {20, 20};
	BYTE flags = CHECKBOXFLAG_NONE;
	std::string label;
	DWORD color = RGBA(64, 64, 64);
	DWORD checkedColor = RGBA(128, 128, 128);
	ErrCode (*event)(void*)noexcept = _defaultCheckboxEvent;
	void* data = nullptr;
};

constexpr void setCheckBoxFlag(Checkbox& box, CHECKBOXFLAGS flag)noexcept{box.flags |= flag;}
constexpr void toggleCheckBoxFlag(Checkbox& box, CHECKBOXFLAGS flag)noexcept{box.flags ^= flag;}
constexpr void resetCheckBoxFlag(Checkbox& box, CHECKBOXFLAGS flag)noexcept{box.flags &= ~flag;}
constexpr bool getCheckBoxFlag(Checkbox& box, CHECKBOXFLAGS flag)noexcept{return (box.flags&flag);}

void updateCheckBoxes(Colorbuffer& buffer, Font& font, Checkbox* boxes, WORD boxCount)noexcept{
	for(WORD i=0; i < boxCount; ++i){
		if(getButton(mouse, MOUSEBUTTON_LMB) && !getButton(mouse, MOUSEBUTTON_PREV_LMB)){
			WORD x = mouse.pos.x-boxes[i].pos.x;
			WORD y = mouse.pos.y-boxes[i].pos.y;
			if(x <= boxes[i].size.x && y <= boxes[i].size.y){
				toggleCheckBoxFlag(boxes[i], CHECKBOXFLAG_CHECKED);
				ErrCheck(boxes[i].event(boxes[i].data), "Checkbox-Event");
			}
		}
		drawRectangle(buffer, boxes[i].pos.x, boxes[i].pos.y, boxes[i].size.x, boxes[i].size.y, boxes[i].color);
		if(getCheckBoxFlag(boxes[i], CHECKBOXFLAG_CHECKED)) drawRectangle(buffer, boxes[i].pos.x+boxes[i].size.x/4, boxes[i].pos.y+boxes[i].size.y/4, boxes[i].size.x/2, boxes[i].size.y/2, boxes[i].checkedColor);
		drawFontString(buffer, font, boxes[i].label.c_str(), boxes[i].pos.x+boxes[i].size.x, boxes[i].pos.y+boxes[i].size.y/2-font.font_size/2);
	}
}

//Öffnet den System Explorer um eine Datei zu öffnen, schreibt bei Erfolg den Dateipfad in filepath
//Filepath muss allokiert sein, Empfehlung: char filepath[MAX_PATH]{0};
ErrCode openExplorer(char* filepath, DWORD maxPathLength, const char filterStr[] = "Any .*\0*.*\0"){
	OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.lpstrFile = (LPSTR)filepath;
    ofn.nMaxFile = maxPathLength;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    char currentDir[MAX_PATH]{0};
    DWORD directoryLength = GetCurrentDirectoryA(MAX_PATH, currentDir);
    if(directoryLength == 0) return ERR_GENERIC_ERROR;				//TODO sollte ein eigener Fehler sein
    currentDir[directoryLength] = '\\';
    ofn.lpstrInitialDir = currentDir;
    ofn.lpstrFilter = filterStr;
    ofn.nFilterIndex = 1;
    if(GetOpenFileName(&ofn) != TRUE) return ERR_SUCCESS;	//TODO ehhh... es ist nicht zwangsweiße kein Fehler...
	strcpy(filepath, (char*)ofn.lpstrFile);
	return ERR_SUCCESS;
}

struct ProgressBar{
	ivec2 pos = {0, 0};
	ivec2 size = {200, 20};
	DWORD backgroundColor = RGBA(128, 128, 128);
	DWORD barColor = RGBA(0, 200, 255);
	float value = 0;
	float min = 0;
	float max = 100;
};

void updateProgressBar(Colorbuffer& buffer, ProgressBar* progressBars, WORD count)noexcept{
	for(WORD i=0; i < count; ++i){
		float t = (progressBars[i].value-progressBars[i].min)/(progressBars[i].max-progressBars[i].min);
		DWORD endX = progressBars[i].size.x*t;
		drawRectangle(buffer, progressBars[i].pos.x, progressBars[i].pos.y, progressBars[i].size.x, progressBars[i].size.y, progressBars[i].backgroundColor);
		drawRectangle(buffer, progressBars[i].pos.x, progressBars[i].pos.y, endX, progressBars[i].size.y, progressBars[i].barColor);
	}
}

//------------------------------ Für 3D und "erweiterte" Grafiken ------------------------------

// #define EARLYZCULLING
#define DEPTH_DIVISOR 100000000000000000000000000000000000.f	//TODO klar...

struct Depthbuffer{
	WORD width = 0;
	WORD height = 0;
	float* data = nullptr;
};

ErrCode createDepthbuffer(Depthbuffer& buffer, WORD width, WORD height)noexcept{
	buffer.width = width;
	buffer.height = height;
	buffer.data = alloc<float>(width*height, "Depthbuffer");
	if(buffer.data == nullptr) return ERR_BAD_ALLOC;
	return ERR_SUCCESS;
}

void destroyDepthbuffer(Depthbuffer& buffer)noexcept{
	dealloc(buffer.data);
}

void clearDepthbuffer(Depthbuffer& buffer)noexcept{
	DWORD totalSize = buffer.width*buffer.height;
	for(DWORD i=0; i < totalSize; ++i) buffer.data[i] = FLOAT_MAX;
}

struct RenderBuffers{
	WORD width;
	WORD height;
	DWORD* frameBuffer;
	float* depthBuffer;
	BYTE* fragmentFlags;		//TODO Markiert eh nur ob ein Fragment gezeichnet wurde oder nicht, daher bits anstatt bytes
	float* attributeBuffers;
	BYTE attributeBuffersCount = 0;
};

ErrCode createRenderBuffers(RenderBuffers& renderBuffers, WORD width, WORD height, BYTE attributesCount)noexcept{
	renderBuffers.width = width;
	renderBuffers.height = height;
	renderBuffers.attributeBuffersCount = attributesCount;
	renderBuffers.frameBuffer = alloc<DWORD>(width*height, "Framebuffer");
	if(renderBuffers.frameBuffer == nullptr) return ERR_BAD_ALLOC;
	renderBuffers.depthBuffer = alloc<float>(width*height, "Depthbuffer");
	if(renderBuffers.depthBuffer == nullptr) return ERR_BAD_ALLOC;
	renderBuffers.fragmentFlags = alloc<BYTE>(width*height, "Fragmentflagsbuffer");
	if(renderBuffers.fragmentFlags == nullptr) return ERR_BAD_ALLOC;
	renderBuffers.attributeBuffers = alloc<float>(width*height*attributesCount, "Attributebuffer");
	if(renderBuffers.attributeBuffers == nullptr) return ERR_BAD_ALLOC;
	return ERR_SUCCESS;
}

void destroyRenderBuffers(RenderBuffers& renderBuffers)noexcept{
	dealloc(renderBuffers.frameBuffer);
	dealloc(renderBuffers.depthBuffer);
	dealloc(renderBuffers.fragmentFlags);
	dealloc(renderBuffers.attributeBuffers);
}

//TODO Muss ja eigentlich nur neu allokiert werden, wenn der Größer werden soll
ErrCode resizeRenderBuffers(RenderBuffers& renderBuffers, WORD width, WORD height)noexcept{
	dealloc(renderBuffers.frameBuffer);
	renderBuffers.frameBuffer = alloc<DWORD>(width*height, "Framebuffer");
	if(renderBuffers.frameBuffer == nullptr) return ERR_BAD_ALLOC;
	dealloc(renderBuffers.depthBuffer);
	renderBuffers.depthBuffer = alloc<float>(width*height, "Depthbuffer");
	if(renderBuffers.depthBuffer == nullptr) return ERR_BAD_ALLOC;
	dealloc(renderBuffers.fragmentFlags);
	renderBuffers.fragmentFlags = alloc<BYTE>(width*height, "Fragmentflagsbuffer");
	if(renderBuffers.fragmentFlags == nullptr) return ERR_BAD_ALLOC;
	dealloc(renderBuffers.attributeBuffers);
	renderBuffers.attributeBuffers = alloc<float>(width*height*renderBuffers.attributeBuffersCount, "Attributebuffer");
	if(renderBuffers.attributeBuffers == nullptr) return ERR_BAD_ALLOC;
	renderBuffers.width = width;
	renderBuffers.height = height;
	return ERR_SUCCESS;
}

void clearRenderBuffers(RenderBuffers& renderBuffers)noexcept{
	for(DWORD i=0; i < renderBuffers.width*renderBuffers.height; ++i){
		renderBuffers.frameBuffer[i] = RGBA(0, 0, 0);
		renderBuffers.depthBuffer[i] = FLOAT_MAX;
		renderBuffers.fragmentFlags[i] = 0;
	}
}

//TODO Attribute für always_inline?
inline constexpr DWORD getAttrLoc(DWORD totalBufferSize, BYTE location)noexcept{
	return totalBufferSize*location;
}

//TODO vllt weg? Oder zumindest passender benennen wie TriangleVertexPositions oder so
struct Triangle{
	fvec3 points[3];
};

//Speichert ein Material aus einer .mtl file
struct Material{
	std::string name;
	DWORD baseColor = RGBA(0, 0, 0);
	Image textures[3];		//TODO sollte dynamisch sein
	BYTE textureCount = 0;
};

void destroyMaterial(Material& material)noexcept{
	for(BYTE i=0; i < material.textureCount; ++i){
		destroyImage(material.textures[i]);
	}
}

//Speichert ein Modell + optionales Material
//TODO MUSS eine create Funktion bekommen, da destroy davon ausgeht, dass gewisste Daten auf dem Heap liegen
//TODO könnte dann noch sowas wie eine AABB bekommen oder so, um das clipping zu beschleunigen
struct TriangleModel{
	Triangle* triangles = nullptr;
	DWORD triangleCount = 0;
	DWORD triangleCapacity = 0;
	Material* material = nullptr;
	float* attributesBuffer = nullptr;
	BYTE attributesCount;
};

void destroyTriangleModel(TriangleModel& model)noexcept{
	dealloc(model.triangles);
	dealloc(model.attributesBuffer);
	// dealloc(model.material);	//TODO hm is doof
}

ErrCode increaseTriangleCapacity(TriangleModel& model, DWORD additionalCapacity)noexcept{
	Triangle* newArray = alloc<Triangle>(model.triangleCapacity+additionalCapacity, "Trianglebuffer");
	float* newAttributeArray = alloc<float>((model.triangleCapacity+additionalCapacity)*model.attributesCount*3, "Triangleattributebuffer");
	if(newArray == nullptr || newAttributeArray == nullptr) return ERR_BAD_ALLOC;
	for(DWORD i=0; i < model.triangleCount; ++i){
		newArray[i] = model.triangles[i];
	}
	for(DWORD i=0; i < model.triangleCount*model.attributesCount*3; ++i){
		newAttributeArray[i] = model.attributesBuffer[i];
	}
	Triangle* oldArray = model.triangles;
	model.triangles = newArray;
	dealloc(oldArray);
	float* oldAttributeArray = model.attributesBuffer;
	model.attributesBuffer = newAttributeArray;
	dealloc(oldAttributeArray);
	model.triangleCapacity += additionalCapacity;
	return ERR_SUCCESS;
}

constexpr float* getAttrLoc(TriangleModel& model, DWORD triangleIdx, BYTE pointIdx, BYTE location){
	return &model.attributesBuffer[triangleIdx*model.attributesCount*3+model.attributesCount*pointIdx+location];
}

//TODO aktuell noch falsch, da 1 auf 0 abgebildet wird
constexpr DWORD textureRepeated(Image& image, float u, float v)noexcept{
	u = u - floor(u);
	v = v - floor(v);
	WORD u1 = u*(image.width-1);
	WORD v1 = v*(image.height-1);
	return image.data[v1*image.width+u1];
}

constexpr DWORD textureClipped(Image& image, float u, float v)noexcept{
	u = clamp(u, 0.f, 1.f);
	v = clamp(v, 0.f, 1.f);
	WORD u1 = u*(image.width-1);
	WORD v1 = v*(image.height-1);
	return image.data[v1*image.width+u1];
}

constexpr DWORD textureUnsafe(Image& image, float u, float v)noexcept{
	WORD u1 = u*(image.width-1);
	WORD v1 = v*(image.height-1);
	return image.data[v1*image.width+u1];
}

//Zeichnet nur die Umrandung eines Dreiecks
void drawTriangleOutline(RenderBuffers& buffer, Triangle& tri)noexcept{
	fvec3 pt0 = tri.points[0]; fvec3 pt1 = tri.points[1]; fvec3 pt2 = tri.points[2];
	pt0.x = ((pt0.x*0.5)+0.5)*buffer.width; pt1.x = ((pt1.x*0.5)+0.5)*buffer.width; pt2.x = ((pt2.x*0.5)+0.5)*buffer.width;
	pt0.y = ((pt0.y*0.5)+0.5)*buffer.height; pt1.y = ((pt1.y*0.5)+0.5)*buffer.height; pt2.y = ((pt2.y*0.5)+0.5)*buffer.height;
	Colorbuffer dummyBuffer;
	dummyBuffer.width = buffer.width;
	dummyBuffer.height = buffer.height;
	dummyBuffer.data = buffer.frameBuffer;
	drawLine(dummyBuffer, pt0.x, pt0.y, pt1.x, pt1.y, RGBA(255, 255, 255, 255));
	drawLine(dummyBuffer, pt0.x, pt0.y, pt2.x, pt2.y, RGBA(255, 255, 255, 255));
	drawLine(dummyBuffer, pt1.x, pt1.y, pt2.x, pt2.y, RGBA(255, 255, 255, 255));
}

//TODO man könnte nur pointer in einem Buffer speichern und einmal zum ende dann über alle Attribute,... loopen, spart eine Menge Kopieren und so
//Also im Sinne von man kopiert die finalen Attribute erst, nachdem der depthBuffer alles getestet hat
void drawTriangleFilledOld(RenderBuffers& renderBuffers, float* attributesBuffer, BYTE attributesCount, Triangle& tri)noexcept{
	fvec3 pt0 = tri.points[0]; fvec3 pt1 = tri.points[1]; fvec3 pt2 = tri.points[2];
	pt0.x = ((pt0.x*0.5)+0.5)*renderBuffers.width; pt1.x = ((pt1.x*0.5f)+0.5)*renderBuffers.width; pt2.x = ((pt2.x*0.5)+0.5)*renderBuffers.width;
	pt0.y = ((pt0.y*0.5)+0.5)*renderBuffers.height; pt1.y = ((pt1.y*0.5f)+0.5)*renderBuffers.height; pt2.y = ((pt2.y*0.5)+0.5)*renderBuffers.height;

	DWORD ymin = min(pt0.y, min(pt1.y, pt2.y));
	DWORD ymax = max(pt0.y, max(pt1.y, pt2.y));
	//TODO irgendwie kann man das doch bestimmt ausnutzen
	// if(ymin == ymax){
	// 	#ifdef PERFORMANCE_ANALYZER
	// 	_perfAnalyzer.pointlessTriangles++;
	// 	#endif
	// 	return;
	// }
	DWORD xmin = min(pt0.x, min(pt1.x, pt2.x));
	DWORD xmax = max(pt0.x, max(pt1.x, pt2.x));

	fvec2 vs1 = {pt1.x - pt0.x, pt1.y - pt0.y};
	fvec2 vs2 = {pt2.x - pt0.x, pt2.y - pt0.y};
	float div = 1/cross(vs1, vs2);

	float invZ[3] = {1/pt0.z, 1/pt1.z, 1/pt2.z};
	float attr[attributesCount*3];
	DWORD attribIdx = 0;
	for(BYTE i=0; i < 3; ++i){
		for(BYTE j=0; j < attributesCount; ++j){
			attr[attribIdx] = attributesBuffer[attribIdx]*invZ[i];
			attribIdx++;
		}
	}

	//Berechne u und v initial und inkrementiere dann nur noch entsprechend
	fvec2 q = {xmin - pt0.x, ymin - pt0.y};
	float u = cross(q, vs2)*div;
	float v = cross(vs1, q)*div;
	float deltaX_u = (pt2.y - pt0.y)*div;
	float deltaX_v = (pt1.y - pt0.y)*div;
	float deltaY_u = (pt2.x - pt0.x)*div;
	float deltaY_v = (pt1.x - pt0.x)*div;
	float tmp_u; float tmp_v;
	for(DWORD y = ymin; y <= ymax; ++y){
		tmp_u = u; tmp_v = v;
		bool wasIn = false;
		for(DWORD x = xmin; x <= xmax; ++x){
			//w -> pt0, u -> pt1, v -> pt2
			float w = 1-u-v;
			if((v >= 0)&&(u >= 0)&&(w >= 0)){
				wasIn = true;
				DWORD idx = y*renderBuffers.width+x;
				float depth = 1/(w*invZ[0] + u*invZ[1] + v*invZ[2]);	//TODO Iterativ lösbar?
				//TODO depth buffer endlich eine Range geben damit eine erwartete Genauigkeit erfasst werden kann
				float inc_depth = depth*DEPTH_DIVISOR;
				if(inc_depth <= renderBuffers.depthBuffer[idx]){
					#ifdef PERFORMANCE_ANALYZER
					_perfAnalyzer.pixelsDrawn++;
					#endif
					renderBuffers.depthBuffer[idx] = inc_depth;
					renderBuffers.fragmentFlags[idx] = 1;
					WORD attribIdx1 = 0;
					WORD attribIdx2 = attributesCount;
					WORD attribIdx3 = attributesCount*2;
					for(BYTE i=0; i < attributesCount; ++i){
						renderBuffers.attributeBuffers[idx+renderBuffers.width*renderBuffers.height*i] = (w*attr[attribIdx1] + u*attr[attribIdx2] + v*attr[attribIdx3])*depth;
						attribIdx1++;
						attribIdx2++;
						attribIdx3++;
					}
				}
				#ifdef PERFORMANCE_ANALYZER
				else _perfAnalyzer.pixelsCulled++;
				#endif
			}
			else if(wasIn) break;
	        u += deltaX_u;
			v -= deltaX_v;
		}
		u = tmp_u;
		v = tmp_v;
		u -= deltaY_u;
		v += deltaY_v;
	}
}

void drawTriangleFilledDepthOnly(Depthbuffer& depthbuffer, Triangle& tri)noexcept{
	fvec3 pt0 = tri.points[0]; fvec3 pt1 = tri.points[1]; fvec3 pt2 = tri.points[2];
	pt0.x = ((pt0.x*0.5)+0.5)*depthbuffer.width; pt1.x = ((pt1.x*0.5f)+0.5)*depthbuffer.width; pt2.x = ((pt2.x*0.5)+0.5)*depthbuffer.width;
	pt0.y = ((pt0.y*0.5)+0.5)*depthbuffer.height; pt1.y = ((pt1.y*0.5f)+0.5)*depthbuffer.height; pt2.y = ((pt2.y*0.5)+0.5)*depthbuffer.height;

	DWORD ymin = min(pt0.y, min(pt1.y, pt2.y));
	DWORD ymax = max(pt0.y, max(pt1.y, pt2.y));
	DWORD xmin = min(pt0.x, min(pt1.x, pt2.x));
	DWORD xmax = max(pt0.x, max(pt1.x, pt2.x));

	fvec2 vs1 = {pt1.x - pt0.x, pt1.y - pt0.y};
	fvec2 vs2 = {pt2.x - pt0.x, pt2.y - pt0.y};
	float div = 1/cross(vs1, vs2);

	//Berechne u und v initial und inkrementiere dann nur noch entsprechend
	fvec2 q = {xmin - pt0.x, ymin - pt0.y};
	float u = cross(q, vs2)*div;
	float v = cross(vs1, q)*div;
	float deltaX_u = (pt2.y - pt0.y)*div;
	float deltaX_v = (pt1.y - pt0.y)*div;
	float deltaY_u = (pt2.x - pt0.x)*div;
	float deltaY_v = (pt1.x - pt0.x)*div;
	float tmp_u;
	float tmp_v;
	for(DWORD y = ymin; y <= ymax; ++y){
		tmp_u = u;
		tmp_v = v;
		bool wasIn = false;
		for(DWORD x = xmin; x <= xmax; ++x){
			//w -> pt0, u -> pt1, v -> pt2
			float w = 1-u-v;
			if((v >= 0)&&(u >= 0)&&(w >= 0)){
				wasIn = true;
				DWORD idx = y*depthbuffer.width+x;
				float depth = w*pt0.z + u*pt1.z + v*pt2.z;
				//TODO depth buffer endlich eine Range geben damit eine erwartete Genauigkeit erfasst werden kann
				float inc_depth = depth*DEPTH_DIVISOR;
				if(inc_depth <= depthbuffer.data[idx]){
					depthbuffer.data[idx] = inc_depth;
				}
			}
			else if(wasIn) break;
	        u += deltaX_u;
			v -= deltaX_v;
		}
		u = tmp_u;
		v = tmp_v;
		u -= deltaY_u;
		v += deltaY_v;
	}
}

//TODO besser wie der alte, aber die UV-Koordinaten sind an den Rändern von Texturen noch nicht ganz korrekt
void drawTriangleFilledNew(RenderBuffers& renderBuffers, float* attributesBuffer, BYTE attributesCount, Triangle& tri)noexcept{
	fvec3 pt0 = tri.points[0]; fvec3 pt1 = tri.points[1]; fvec3 pt2 = tri.points[2];
	pt0.x = ((pt0.x*0.5)+0.5)*renderBuffers.width; pt1.x = ((pt1.x*0.5f)+0.5)*renderBuffers.width; pt2.x = ((pt2.x*0.5)+0.5)*renderBuffers.width;
	pt0.y = ((pt0.y*0.5)+0.5)*renderBuffers.height; pt1.y = ((pt1.y*0.5f)+0.5)*renderBuffers.height; pt2.y = ((pt2.y*0.5)+0.5)*renderBuffers.height;

	int pt0x = pt0.x;
	int pt0y = pt0.y;
	int pt1x = pt1.x;
	int pt1y = pt1.y;
	int pt2x = pt2.x;
	int pt2y = pt2.y;
	if(pt0y > pt1y) std::swap(pt0x, pt1x), std::swap(pt0y, pt1y);
	if(pt0y > pt2y) std::swap(pt0x, pt2x), std::swap(pt0y, pt2y);
	if(pt1y > pt2y) std::swap(pt1x, pt2x), std::swap(pt1y, pt2y);

	float invZ[3] = {1/pt0.z, 1/pt1.z, 1/pt2.z};
	float attr[attributesCount*3];
	DWORD attribIdx = 0;
	for(BYTE i=0; i < 3; ++i){
		for(BYTE j=0; j < attributesCount; ++j){
			attr[attribIdx] = attributesBuffer[attribIdx]*invZ[i];
			attribIdx++;
		}
	}

	float dx = pt2x-pt0x;
	int dy = pt2y-pt0y;
	if(dx == 0) dx = 1e-8;
	float mP2P0 = dy/dx;
	float imP2P0 = 1/mP2P0;
	float bP2P0 = pt0y-mP2P0*pt0x;

	float div = 1/((pt1.x-pt0.x)*(pt2.y-pt1.y)-(pt1.y-pt0.y)*(pt2.x-pt1.x));
	float incM1 = (float)(pt2.y-pt1.y)*div;
	float incM2 = (float)(pt0.y-pt2.y)*div;

	dx = pt1x-pt0x;
	dy = pt1y-pt0y;
	if(dy != 0){
		if(dx == 0) dx = 1e-8;
		float mP1P0 = dy/dx;
		float imP1P0 = 1/mP1P0;
		float bP1P0 = pt0y-mP1P0*pt0x;

		for(int y=pt0y; y <= pt1y; ++y){
			int xP2P0 = (y-bP2P0)*imP2P0;
			int xP1P0 = (y-bP1P0)*imP1P0;
			int xBeg = min(xP2P0, xP1P0);
			int xEnd = max(xP2P0, xP1P0);
			float m1 = (float)((pt1.x-xBeg)*(pt2.y-y)-(pt2.x-xBeg)*(pt1.y-y))*div;
			float m2 = (float)((pt2.x-xBeg)*(pt0.y-y)-(pt0.x-xBeg)*(pt2.y-y))*div;
			for(;xBeg < xEnd; ++xBeg){
				float m3 = 1-m2-m1;
				DWORD idx = y*renderBuffers.width+xBeg;
				float depth = 1/(m1*invZ[0] + m2*invZ[1] + m3*invZ[2]);		//TODO Iterativ lösbar?
				//TODO depth buffer endlich eine Range geben damit eine erwartete Genauigkeit erfasst werden kann
				float inc_depth = depth*DEPTH_DIVISOR;
				if(inc_depth <= renderBuffers.depthBuffer[idx]){
					#ifdef PERFORMANCE_ANALYZER
					_perfAnalyzer.pixelsDrawn++;
					#endif
					renderBuffers.depthBuffer[idx] = inc_depth;
					renderBuffers.fragmentFlags[idx] = 1;
					WORD attribIdx1 = 0;
					WORD attribIdx2 = attributesCount;
					WORD attribIdx3 = attributesCount*2;
					for(BYTE i=0; i < attributesCount; ++i){
						renderBuffers.attributeBuffers[idx+renderBuffers.width*renderBuffers.height*i] = (m1*attr[attribIdx1] + m2*attr[attribIdx2] + m3*attr[attribIdx3])*depth;
						attribIdx1++;
						attribIdx2++;
						attribIdx3++;
					}
				}
				#ifdef PERFORMANCE_ANALYZER
				else _perfAnalyzer.pixelsCulled++;
				#endif
				m1 -= incM1;
				m2 -= incM2;
			}
		}
	}

	dx = pt2x-pt1x;
	dy = pt2y-pt1y;
	if(dy == 0) return;
	if(dx == 0) dx = 1e-8;
	float mP2P1 = dy/dx;
	float imP2P1 = 1/mP2P1;
	float bP2P1 = pt1y-mP2P1*pt1x;

	for(int y=pt1y; y <= pt2y; ++y){
		int xP2P0 = (y-bP2P0)*imP2P0;
		int xP2P1 = (y-bP2P1)*imP2P1;
		int xBeg = min(xP2P0, xP2P1);
		int xEnd = max(xP2P0, xP2P1);
		float m1 = (float)((pt1.x-xBeg)*(pt2.y-y)-(pt2.x-xBeg)*(pt1.y-y))*div;
		float m2 = (float)((pt2.x-xBeg)*(pt0.y-y)-(pt0.x-xBeg)*(pt2.y-y))*div;
		for(;xBeg < xEnd; ++xBeg){
			float m3 = 1-m2-m1;
			DWORD idx = y*renderBuffers.width+xBeg;
			float depth = 1/(m1*invZ[0] + m2*invZ[1] + m3*invZ[2]);		//TODO Iterativ lösbar?
			//TODO depth buffer endlich eine Range geben damit eine erwartete Genauigkeit erfasst werden kann
			float inc_depth = depth*DEPTH_DIVISOR;
			if(inc_depth <= renderBuffers.depthBuffer[idx]){
				#ifdef PERFORMANCE_ANALYZER
				_perfAnalyzer.pixelsDrawn++;
				#endif
				renderBuffers.depthBuffer[idx] = inc_depth;
				renderBuffers.fragmentFlags[idx] = 1;
				WORD attribIdx1 = 0;
				WORD attribIdx2 = attributesCount;
				WORD attribIdx3 = attributesCount*2;
				for(BYTE i=0; i < attributesCount; ++i){
					renderBuffers.attributeBuffers[idx+renderBuffers.width*renderBuffers.height*i] = (m1*attr[attribIdx1] + m2*attr[attribIdx2] + m3*attr[attribIdx3])*depth;
					attribIdx1++;
					attribIdx2++;
					attribIdx3++;
				}
			}
			#ifdef PERFORMANCE_ANALYZER
			else _perfAnalyzer.pixelsCulled++;
			#endif
			m1 -= incM1;
			m2 -= incM2;
		}
	}
}

struct plane{
	fvec3 pos;
	fvec3 normal;
};

//Berechnet Kollisionspunkt zwischen einem Plane und 2 punkten (die eine linie bilden)
//Gibt zurück, ob eine Kollision stattgefunden hat und den Kollisionspunkt cp, sowie t als skalares Parameter (zwischen 0 - 1, start*(1-t)+end*t)
bool rayPlaneIntersection(plane& p, fvec3& start, fvec3& end, float& t, fvec3& cp)noexcept{
	fvec3 dir = {end.x-start.x, end.y-start.y, end.z-start.z};
	float d = dot(p.normal, dir);
	if(d != 0){
		fvec3 tmp = {p.pos.x-start.x, p.pos.y-start.y, p.pos.z-start.z};
	    t = dot(tmp, p.normal)/d;
	    cp = {start.x+dir.x*t, start.y+dir.y*t, start.z+dir.z*t};
	    return true;
	}
	return false;
}
//Nur wenn man schon weiß, dass es eine Kollision geben muss...
float rayPlaneIntersectionFast(plane& p, fvec3& start, fvec3& end, fvec3& cp)noexcept{
	fvec3 dir = {end.x-start.x, end.y-start.y, end.z-start.z};
	float d = dot(p.normal, dir);
	fvec3 tmp = {p.pos.x-start.x, p.pos.y-start.y, p.pos.z-start.z};
	float t = dot(tmp, p.normal)/d;
	cp = {start.x+dir.x*t, start.y+dir.y*t, start.z+dir.z*t};
	return t;
}

//TODO kann man bestimmt auch noch weiter optimieren, indexe zu speichern hat nichts gebracht...
void clipPlane(plane& p, Triangle* buffer, float* attributesBuffer, BYTE attributesCount, BYTE& count)noexcept{
	BYTE tmp_off = count;			//Offset wo das aktuelle neue Dreieck hinzugefügt werden soll
	BYTE offset = count;			//Originaler Offset der neuen Dreiecke
	BYTE temp_count = count;		//Index des letzten originalen Dreiecks
	fvec3 in_v[3];
	fvec3 out_v[3];
	float in_attr[attributesCount][3];
	float out_attr[attributesCount][3];
	fvec3 vec;
	WORD attributeOffset = 0;
	const DWORD attributesCountPerTriangle = attributesCount*3;
	for(BYTE i=0; i < temp_count; ++i){
		const DWORD currentAttributeLocation = i*attributesCountPerTriangle;
		BYTE in = 0;
		BYTE out = 0;
		for(BYTE j=0; j < 3; ++j){
			vec = buffer[i].points[j];
			vec.x -= p.pos.x;
			vec.y -= p.pos.y;
			vec.z -= p.pos.z;
			float dist = dot(vec, p.normal);
			if(dist < 0){
				out_v[out] = buffer[i].points[j];
				for(BYTE k=0; k < attributesCount; ++k) out_attr[k][out] = attributesBuffer[currentAttributeLocation+attributesCount*j+k];
				++out;
			}else{
				in_v[in] = buffer[i].points[j];
				for(BYTE k=0; k < attributesCount; ++k) in_attr[k][in] = attributesBuffer[currentAttributeLocation+attributesCount*j+k];
				++in;
			}
			attributeOffset += attributesCount;
		}
		switch(in){
			case 0:{	//Dreieck liegt komplett ausserhalb, es muss entfernt werden
				buffer[i] = buffer[temp_count-1];
				for(BYTE j=0; j < attributesCountPerTriangle; ++j){
					attributesBuffer[currentAttributeLocation+j] = attributesBuffer[(temp_count-1)*attributesCountPerTriangle+j];
				}
				--temp_count;
				--i;
				--count;
				break;
			}
			case 1:{	//Das aktuelle Dreieck kann einfach geändert werden
				float t = rayPlaneIntersectionFast(p, in_v[0], out_v[0], buffer[i].points[1]);
				for(BYTE k=0; k < attributesCount; ++k){
					attributesBuffer[currentAttributeLocation+attributesCount+k] = in_attr[k][0]*(1-t)+out_attr[k][0]*t;
				}
				t = rayPlaneIntersectionFast(p, in_v[0], out_v[1], buffer[i].points[2]);
				for(BYTE k=0; k < attributesCount; ++k){
					attributesBuffer[currentAttributeLocation+attributesCount*2+k] = in_attr[k][0]*(1-t)+out_attr[k][1]*t;
					attributesBuffer[currentAttributeLocation+k] = in_attr[k][0];
				}
				buffer[i].points[0] = in_v[0];
				break;
			}
			case 2:{	//2 neue Dreiecke müssen hinzugefügt werden und das aktuelle entfernt
				buffer[i] = buffer[temp_count-1];
				for(BYTE j=0; j < attributesCountPerTriangle; ++j){
					attributesBuffer[currentAttributeLocation+j] = attributesBuffer[(temp_count-1)*attributesCountPerTriangle+j];
				}
				--temp_count;
				--i;
				--count;
				float t = rayPlaneIntersectionFast(p, in_v[0], out_v[0], buffer[tmp_off].points[2]);
				for(BYTE k=0; k < attributesCount; ++k){
					attributesBuffer[tmp_off*attributesCountPerTriangle+attributesCount*2+k] = in_attr[k][0]*(1-t)+out_attr[k][0]*t;
					attributesBuffer[tmp_off*attributesCountPerTriangle+k] = in_attr[k][0];
					attributesBuffer[tmp_off*attributesCountPerTriangle+attributesCount+k] = in_attr[k][1];
				}
				buffer[tmp_off].points[0] = in_v[0];
				buffer[tmp_off].points[1] = in_v[1];
				t = rayPlaneIntersectionFast(p, in_v[1], out_v[0], buffer[tmp_off+1].points[2]);
				for(BYTE k=0; k < attributesCount; ++k){
					attributesBuffer[(tmp_off+1)*attributesCountPerTriangle+attributesCount*2+k] = in_attr[k][1]*(1-t)+out_attr[k][0]*t;
					attributesBuffer[(tmp_off+1)*attributesCountPerTriangle+k] = attributesBuffer[tmp_off*attributesCountPerTriangle+attributesCount*2+k];
					attributesBuffer[(tmp_off+1)*attributesCountPerTriangle+attributesCount+k] = in_attr[k][1];
				}
				buffer[tmp_off+1].points[0] = buffer[tmp_off].points[2];
				buffer[tmp_off+1].points[1] = in_v[1];
				tmp_off += 2;
				count += 2;
				break;
			}
		}
	}
	for(BYTE i=0; i < count-temp_count; ++i){
		buffer[temp_count+i] = buffer[offset+i];
		for(BYTE j=0; j < attributesCountPerTriangle; ++j){
			attributesBuffer[(temp_count+i)*attributesCountPerTriangle+j] = attributesBuffer[(offset+i)*attributesCountPerTriangle+j];
		}
	}
	return;
}

#define XMIN -1.001f
#define XMAX 1.001f
#define YMIN -1.001f
#define YMAX 1.001f

//TODO kann bestimmt um einiges optimiert werden
BYTE clipping(RenderBuffers& renderBuffers, float* attributesBuffer, BYTE attributesCount, Triangle* buffer)noexcept{
	BYTE count = 1;
	float aspect_ratio = (float)renderBuffers.height/renderBuffers.width;

	plane p = {};
	p.normal = {0, 0, 1};
	p.pos = {0, 0, 0};
	clipPlane(p, buffer, attributesBuffer, attributesCount, count);

	p.normal = {XMIN*aspect_ratio, 0, 1};
	p.normal = normalize(p.normal);
	clipPlane(p, buffer, attributesBuffer, attributesCount, count);

	p.normal = {XMAX*aspect_ratio, 0, 1};
	p.normal = normalize(p.normal);
	clipPlane(p, buffer, attributesBuffer, attributesCount, count);

	p.normal = {0, YMIN, 1};
	p.normal = normalize(p.normal);
	clipPlane(p, buffer, attributesBuffer, attributesCount, count);

	p.normal = {0, YMAX, 1};
	p.normal = normalize(p.normal);
	clipPlane(p, buffer, attributesBuffer, attributesCount, count);

	return count;
}

constexpr bool cmpLess(const float val, const float region)noexcept{
	return val <= region;
}
constexpr bool cmpLarg(const float val, const float region)noexcept{
	return val >= region;
}

void clipEdgeX(Triangle* buffer, BYTE& count, const float regionSize, bool(*cmpFunc)(float, float)noexcept)noexcept{
	BYTE tmpCount = count;
	BYTE newOffset = count;
	BYTE offset = count;
	for(BYTE i=0; i < tmpCount; ++i){
		BYTE in = 0;
		BYTE out = 0;
		fvec3 inPts[3];
		fvec3 outPts[3];
		for(BYTE j=0; j < 3; ++j){
			if(cmpFunc(buffer[i].points[j].x, regionSize)) inPts[in++] = buffer[i].points[j];
			else outPts[out++] = buffer[i].points[j];
		}
		switch(in){
			case 0:
				buffer[i] = buffer[tmpCount-1];
				tmpCount--;
				i--;
				count--;
				break;
			case 1:{
				float t = (regionSize-inPts[0].x)/(outPts[0].x-inPts[0].x);
				fvec3 pt1 = {regionSize, lerp(inPts[0].y, outPts[0].y, t), lerp(inPts[0].z, outPts[0].z, t)};
				t = (regionSize-inPts[0].x)/(outPts[1].x-inPts[0].x);
				fvec3 pt2 = {regionSize, lerp(inPts[0].y, outPts[1].y, t), lerp(inPts[0].z, outPts[1].z, t)};
				buffer[i].points[0] = inPts[0];
				buffer[i].points[2] = pt1;
				buffer[i].points[1] = pt2;
				break;
			}
			case 2:
				buffer[i] = buffer[tmpCount-1];
				tmpCount--;
				i--;
				count--;
				float t = (regionSize-inPts[0].x)/(outPts[0].x-inPts[0].x);
				fvec3 pt1 = {regionSize, lerp(inPts[0].y, outPts[0].y, t), lerp(inPts[0].z, outPts[0].z, t)};
				t = (regionSize-inPts[1].x)/(outPts[0].x-inPts[1].x);
				fvec3 pt2 = {regionSize, lerp(inPts[1].y, outPts[0].y, t), lerp(inPts[1].z, outPts[0].z, t)};
				buffer[newOffset].points[0] = inPts[0];
				buffer[newOffset].points[2] = pt1;
				buffer[newOffset].points[1] = inPts[1];
				buffer[newOffset+1].points[1] = inPts[1];
				buffer[newOffset+1].points[0] = pt1;
				buffer[newOffset+1].points[2] = pt2;
				newOffset += 2;
				count += 2;
				break;
		}
	}
	for(BYTE i=0; i < count-tmpCount; ++i){
		buffer[tmpCount+i] = buffer[offset+i];
	}
}

void clipEdgeY(Triangle* buffer, BYTE& count, const float regionSize, bool(*cmpFunc)(float, float)noexcept)noexcept{
	BYTE tmpCount = count;
	BYTE newOffset = count;
	BYTE offset = count;
	for(BYTE i=0; i < tmpCount; ++i){
		BYTE in = 0;
		BYTE out = 0;
		fvec3 inPts[3];
		fvec3 outPts[3];
		for(BYTE j=0; j < 3; ++j){
			if(cmpFunc(buffer[i].points[j].y, regionSize)) inPts[in++] = buffer[i].points[j];
			else outPts[out++] = buffer[i].points[j];
		}
		switch(in){
			case 0:
				buffer[i] = buffer[tmpCount-1];
				tmpCount--;
				i--;
				count--;
				break;
			case 1:{
				float t = (regionSize-inPts[0].y)/(outPts[0].y-inPts[0].y);
				fvec3 pt1 = {lerp(inPts[0].x, outPts[0].x, t), regionSize, lerp(inPts[0].z, outPts[0].z, t)};
				t = (regionSize-inPts[0].y)/(outPts[1].y-inPts[0].y);
				fvec3 pt2 = {lerp(inPts[0].x, outPts[1].x, t), regionSize, lerp(inPts[0].z, outPts[1].z, t)};
				buffer[i].points[0] = inPts[0];
				buffer[i].points[2] = pt1;
				buffer[i].points[1] = pt2;
				break;
			}
			case 2:
				buffer[i] = buffer[tmpCount-1];
				tmpCount--;
				i--;
				count--;
				float t = (regionSize-inPts[0].y)/(outPts[0].y-inPts[0].y);
				fvec3 pt1 = {lerp(inPts[0].x, outPts[0].x, t), regionSize, lerp(inPts[0].z, outPts[0].z, t)};
				t = (regionSize-inPts[1].y)/(outPts[0].y-inPts[1].y);
				fvec3 pt2 = {lerp(inPts[1].x, outPts[0].x, t), regionSize, lerp(inPts[1].z, outPts[0].z, t)};
				buffer[newOffset].points[0] = inPts[0];
				buffer[newOffset].points[2] = pt1;
				buffer[newOffset].points[1] = inPts[1];
				buffer[newOffset+1].points[1] = inPts[1];
				buffer[newOffset+1].points[0] = pt1;
				buffer[newOffset+1].points[2] = pt2;
				newOffset += 2;
				count += 2;
				break;
		}
	}
	for(BYTE i=0; i < count-tmpCount; ++i){
		buffer[tmpCount+i] = buffer[offset+i];
	}
}

#define CLIPPINGREGIONSIZE 800
const float INVCLIPPINGREGIONSIZE = 1.f/CLIPPINGREGIONSIZE;
BYTE clippingOrthographic(Triangle* triangles)noexcept{
	BYTE count = 1;

	clipEdgeX(triangles, count, 0.999, cmpLess);
	clipEdgeX(triangles, count, -0.999, cmpLarg);
	clipEdgeY(triangles, count, 0.999, cmpLess);
	clipEdgeY(triangles, count, -0.999, cmpLarg);

	return count;
}

struct Camera{
	float focal_length;
	fvec3 pos;
	fvec2 rot;	//Yaw, pitch. rot.x ist die Rotation um die Y-Achse weil... uhh ja
};

typedef fvec3 (*vertexShaderFunction)(fvec3&, float*)noexcept;

//TODO sollte noch beachten, dass renderbuffers nicht unbedingt so viele attributebuffer zur verfügung stellt wie es vertex attribute gibt
//TODO Man sollte die Rotationsmatrix übergeben oder einfach in Camera speichern, damit die nicht immer wieder hier neu berechnet wird
//TODO Backface + Frontfaceculling sollten ifs sein, die man wärend der runtime ändern kann
void rasterize(RenderBuffers& renderBuffers, Triangle* tris, float* attributes, BYTE attributesCount, DWORD startIdx, DWORD endIdx, Camera& cam, vertexShaderFunction* vertexShader, WORD vertexShaderCount)noexcept{
#ifdef PERFORMANCE_ANALYZER
	_perfAnalyzer.totalTriangles += endIdx - startIdx;
#endif
	float rotm[3][3];
	float sin_rotx = sin(cam.rot.x);
	float cos_rotx = cos(cam.rot.x);
	float sin_roty = sin(cam.rot.y);
	float cos_roty = cos(cam.rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty;		rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty;	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;
	const float aspect_ratio = (float)renderBuffers.height/renderBuffers.width;
	Triangle buffer[32];	//Speichert Dreiecke die durch das clipping entstehen könnten
	const DWORD totalAttributesCount = attributesCount*3;
	float attributesBuffer[32*totalAttributesCount];
    for(DWORD i=startIdx; i < endIdx; ++i){
		const DWORD currentAttributeLocation = i*totalAttributesCount;
		Triangle& tri = buffer[0];
    	tri = tris[i];
		for(WORD j=0; j < vertexShaderCount; ++j){
			tri.points[0] = vertexShader[j](tri.points[0], &attributes[currentAttributeLocation]);
			tri.points[1] = vertexShader[j](tri.points[1], &attributes[currentAttributeLocation+attributesCount]);
			tri.points[2] = vertexShader[j](tri.points[2], &attributes[currentAttributeLocation+attributesCount*2]);
		}
    	for(BYTE j=0; j < 3; ++j){
			fvec3 d = {tri.points[j].x-cam.pos.x, tri.points[j].y-cam.pos.y, tri.points[j].z-cam.pos.z};
			fvec3 v = mulVec3Mat3x3(d, rotm);
    	    tri.points[j].x = v.x;
    	    tri.points[j].y = v.y;
    	    tri.points[j].z = v.z;
    	}
    	fvec3 l01 = {tri.points[1].x-tri.points[0].x, tri.points[1].y-tri.points[0].y, tri.points[1].z-tri.points[0].z};
    	fvec3 l02 = {tri.points[2].x-tri.points[0].x, tri.points[2].y-tri.points[0].y, tri.points[2].z-tri.points[0].z};
    	fvec3 normal = cross(l01, l02);
#ifdef EARLYZCULLING
		if(tri.points[0].z < 0 && tri.points[1].z < 0 && tri.points[2].z < 0) continue;
#endif
#ifdef CULLBACKFACES
    	if(dot(tri.points[0], normal) <= 0) continue;
#endif
#ifdef CULLFRONTFACES
    	if(dot(tri.points[0], normal) >= 0) continue;
#endif
		for(int j=0; j < attributesCount*3; ++j) attributesBuffer[j] = attributes[currentAttributeLocation+j];
    	BYTE count = clipping(renderBuffers, attributesBuffer, attributesCount, buffer);
    	for(BYTE j=0; j < count; ++j){
    		fvec3 pt1 = buffer[j].points[0];
			fvec3 pt2 = buffer[j].points[1];
			fvec3 pt3 = buffer[j].points[2];
    		buffer[j].points[0].x = pt1.x*(cam.focal_length/pt1.z)*aspect_ratio;
    		buffer[j].points[1].x = pt2.x*(cam.focal_length/pt2.z)*aspect_ratio;
    		buffer[j].points[2].x = pt3.x*(cam.focal_length/pt3.z)*aspect_ratio;
			buffer[j].points[0].y = pt1.y*(cam.focal_length/pt1.z);
			buffer[j].points[1].y = pt2.y*(cam.focal_length/pt2.z);
			buffer[j].points[2].y = pt3.y*(cam.focal_length/pt3.z);
    		buffer[j].points[0].z = pt1.z;
			buffer[j].points[1].z = pt2.z;
			buffer[j].points[2].z = pt3.z;
			#ifdef NEWTRIANGLEDRAWINGALGORITHM
			drawTriangleFilledNew(renderBuffers, attributesBuffer+j*totalAttributesCount, attributesCount, buffer[j]);
			#else
			drawTriangleFilledOld(renderBuffers, attributesBuffer+j*totalAttributesCount, attributesCount, buffer[j]);
			#endif
#ifdef PERFORMANCE_ANALYZER
    		_perfAnalyzer.drawnTriangles += 1;
#endif
    	}
    }
    return;
}

void rasterizeOutline(RenderBuffers& renderBuffers, Triangle* tris, float* attributes, BYTE attributesCount, DWORD startIdx, DWORD endIdx, Camera& cam, vertexShaderFunction* vertexShader, WORD vertexShaderCount)noexcept{
#ifdef PERFORMANCE_ANALYZER
	_perfAnalyzer.totalTriangles += endIdx - startIdx;
#endif
	float rotm[3][3];
	float sin_rotx = sin(cam.rot.x);
	float cos_rotx = cos(cam.rot.x);
	float sin_roty = sin(cam.rot.y);
	float cos_roty = cos(cam.rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty;		rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty;	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;
	Triangle buffer[32];	//Speichert Dreiecke die durch das clipping entstehen könnten
    for(DWORD i=startIdx; i < endIdx; ++i){
		Triangle& tri = buffer[0];
    	tri = tris[i];
		for(WORD j=0; j < vertexShaderCount; ++j){
			tri.points[0] = vertexShader[j](tri.points[0], &attributes[i*attributesCount*3]);
			tri.points[1] = vertexShader[j](tri.points[1], &attributes[i*attributesCount*3+attributesCount]);
			tri.points[2] = vertexShader[j](tri.points[2], &attributes[i*attributesCount*3+attributesCount*2]);
		}
    	for(BYTE j=0; j < 3; ++j){
			fvec3 d = {tri.points[j].x-cam.pos.x, tri.points[j].y-cam.pos.y, tri.points[j].z-cam.pos.z};
			fvec3 v = mulVec3Mat3x3(d, rotm);
    	    tri.points[j].x = v.x;
    	    tri.points[j].y = v.y;
    	    tri.points[j].z = v.z;
    	}
    	fvec3 l01 = {tri.points[1].x-tri.points[0].x, tri.points[1].y-tri.points[0].y, tri.points[1].z-tri.points[0].z};
    	fvec3 l02 = {tri.points[2].x-tri.points[0].x, tri.points[2].y-tri.points[0].y, tri.points[2].z-tri.points[0].z};
    	fvec3 normal = cross(l01, l02);
#ifdef EARLYZCULLING
		if(tri.points[0].z < 0 && tri.points[1].z < 0 && tri.points[2].z < 0) continue;
#endif
#ifdef CULLBACKFACES
    	if(dot(tri.points[0], normal) <= 0) continue;
#endif
#ifdef CULLFRONTFACES
    	if(dot(tri.points[0], normal) >= 0) continue;
#endif
    	BYTE count = clipping(renderBuffers, nullptr, 0, buffer);
		const float aspect_ratio = (float)renderBuffers.height/renderBuffers.width;
    	for(BYTE j=0; j < count; ++j){
    		fvec3 pt1 = buffer[j].points[0];
			fvec3 pt2 = buffer[j].points[1]; 
			fvec3 pt3 = buffer[j].points[2];
    		buffer[j].points[0].x = pt1.x*(cam.focal_length/pt1.z)*aspect_ratio;
    		buffer[j].points[1].x = pt2.x*(cam.focal_length/pt2.z)*aspect_ratio;
    		buffer[j].points[2].x = pt3.x*(cam.focal_length/pt3.z)*aspect_ratio;
			buffer[j].points[0].y = pt1.y*(cam.focal_length/pt1.z);
			buffer[j].points[1].y = pt2.y*(cam.focal_length/pt2.z);
			buffer[j].points[2].y = pt3.y*(cam.focal_length/pt3.z);
    		buffer[j].points[0].z = pt1.z;
			buffer[j].points[1].z = pt2.z;
			buffer[j].points[2].z = pt3.z;
			drawTriangleOutline(renderBuffers, buffer[j]);
#ifdef PERFORMANCE_ANALYZER
    		_perfAnalyzer.drawnTriangles += 1;
#endif
    	}
    }
    return;
}

void rasterizeShadowMap(Depthbuffer& depthbuffer, Triangle* tris, DWORD startIdx, DWORD endIdx, Camera& cam)noexcept{
	float rotm[3][3];
	float sin_rotx = sin(cam.rot.x);
	float cos_rotx = cos(cam.rot.x);
	float sin_roty = sin(cam.rot.y);
	float cos_roty = cos(cam.rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty;		rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty;	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;
	Triangle buffer[32];	//Speichert Dreiecke die durch das clipping entstehen könnten
    for(DWORD i=startIdx; i < endIdx; ++i){
		Triangle& tri = buffer[0];
    	tri = tris[i];
    	for(BYTE j=0; j < 3; ++j){
			fvec3 d = {tri.points[j].x-cam.pos.x, tri.points[j].y-cam.pos.y, tri.points[j].z-cam.pos.z};
			fvec3 v = mulVec3Mat3x3(d, rotm);
    	    tri.points[j] = v;
    	}
    	fvec3 l01 = {tri.points[1].x-tri.points[0].x, tri.points[1].y-tri.points[0].y, tri.points[1].z-tri.points[0].z};
    	fvec3 l02 = {tri.points[2].x-tri.points[0].x, tri.points[2].y-tri.points[0].y, tri.points[2].z-tri.points[0].z};
    	fvec3 normal = cross(l01, l02);
#ifdef CULLBACKFACESSHADOW
    	if(dot(tri.points[0], normal) <= 0) continue;
#endif
#ifdef CULLFRONTFACESSHADOW
    	if(dot(tri.points[0], normal) >= 0) continue;
#endif
		tri.points[0].x *= INVCLIPPINGREGIONSIZE;
    	tri.points[1].x *= INVCLIPPINGREGIONSIZE;
    	tri.points[2].x *= INVCLIPPINGREGIONSIZE;
		tri.points[0].y *= INVCLIPPINGREGIONSIZE;
		tri.points[1].y *= INVCLIPPINGREGIONSIZE;
		tri.points[2].y *= INVCLIPPINGREGIONSIZE;
    	BYTE count = clippingOrthographic(buffer);
    	for(BYTE j=0; j < count; ++j){
			#ifdef NEWTRIANGLEDRAWINGALGORITHM	//TODO
			#endif
			drawTriangleFilledDepthOnly(depthbuffer, buffer[j]);
    	}
    }
    return;
}

struct ColorPoint{
	fvec3 pos;
	DWORD color;
};

void drawPoints(RenderBuffers& renderBuffers, ColorPoint* points, DWORD startIdx, DWORD endIdx, WORD radius, Camera& cam, float(*pointScalingFunction)(WORD, float)noexcept)noexcept{
	float rotm[3][3];
	float sin_rotx = sin(cam.rot.x);
	float cos_rotx = cos(cam.rot.x);
	float sin_roty = sin(cam.rot.y);
	float cos_roty = cos(cam.rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty;		rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty;	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;
	const float aspect_ratio = (float)renderBuffers.height/renderBuffers.width;
	const DWORD totalBufferWidth = renderBuffers.width*renderBuffers.height;
    for(DWORD i=startIdx; i < endIdx; ++i){
		fvec3 d = {points[i].pos.x-cam.pos.x, points[i].pos.y-cam.pos.y, points[i].pos.z-cam.pos.z};
		fvec3 v = mulVec3Mat3x3(d, rotm);
		if(v.z <= 1) continue;
		DWORD x = (((v.x*(cam.focal_length/v.z)*aspect_ratio)*0.5)+0.5)*renderBuffers.width;
		DWORD y = ((v.y*(cam.focal_length/v.z)*0.5)+0.5)*renderBuffers.height;
		const float r = pointScalingFunction(radius, v.z);
		for(SWORD j=-r; j < r; ++j){
			for(SWORD k=-r; k < r; ++k){
				if(k*k+j*j < r*r){
					DWORD dx = k+x;
					DWORD dy = j+y;
					if(dx >= renderBuffers.width || dy >= renderBuffers.height) continue;
					DWORD idx = dy*renderBuffers.width+dx;
					#ifdef POINTSTESTDEPTH
					if(renderBuffers.depthBuffer[idx] < v.z*DEPTH_DIVISOR) continue;
					#endif
					renderBuffers.frameBuffer[idx] = points[i].color;
				}
			}
		}
    }
    return;
}

void drawPointsDepth(RenderBuffers& renderBuffers, ColorPoint* points, DWORD startIdx, DWORD endIdx, WORD radius, Camera& cam, float(*pointScalingFunction)(WORD, float)noexcept)noexcept{
	float rotm[3][3];
	float sin_rotx = sin(cam.rot.x);
	float cos_rotx = cos(cam.rot.x);
	float sin_roty = sin(cam.rot.y);
	float cos_roty = cos(cam.rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty;		rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty;	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;
	const float aspect_ratio = (float)renderBuffers.height/renderBuffers.width;
	const DWORD totalBufferWidth = renderBuffers.width*renderBuffers.height;
    for(DWORD i=startIdx; i < endIdx; ++i){
		fvec3 d = {points[i].pos.x-cam.pos.x, points[i].pos.y-cam.pos.y, points[i].pos.z-cam.pos.z};
		fvec3 v = mulVec3Mat3x3(d, rotm);
		if(v.z <= 1) continue;
		DWORD x = (((v.x*(cam.focal_length/v.z)*aspect_ratio)*0.5)+0.5)*renderBuffers.width;
		DWORD y = ((v.y*(cam.focal_length/v.z)*0.5)+0.5)*renderBuffers.height;
		const float r = pointScalingFunction(radius, v.z);
		for(SWORD j=-r; j < r; ++j){
			for(SWORD k=-r; k < r; ++k){
				if(k*k+j*j < r*r){
					DWORD dx = k+x;
					DWORD dy = j+y;
					if(dx >= renderBuffers.width || dy >= renderBuffers.height) continue;
					DWORD idx = dy*renderBuffers.width+dx;
					if(renderBuffers.depthBuffer[idx] < v.z*DEPTH_DIVISOR) continue;
					renderBuffers.frameBuffer[idx] = points[i].color;
					renderBuffers.depthBuffer[idx] = v.z*DEPTH_DIVISOR;
				}
			}
		}
    }
    return;
}

//TODO alle weiteren Funktionen sollten in eine andere Datei, da diese ja nichts mehr direkt mit dem rendering zu tun haben

ErrCode splitString(const std::string& string, DWORD& value0, DWORD& value1, DWORD& value2)noexcept{
	std::string buffer[3];
	BYTE idx = 0;
	for(size_t i=0; i < string.size(); ++i){
		if(string[i] == '/'){
			idx++;
			if(idx == 3) return ERR_MODEL_BAD_FORMAT;
			continue;
		};
		buffer[idx] += string[i];
	}
	if(idx < 2) return ERR_MODEL_BAD_FORMAT;
	//TODO Exeptions abfangen
	value0 = (buffer[0].size() < 1) ? 0 : std::stoul(buffer[0].c_str())-1;
	value1 = (buffer[1].size() < 1) ? 0 : std::stoul(buffer[1].c_str())-1;
	value2 = (buffer[2].size() < 1) ? 0 : std::stoul(buffer[2].c_str())-1;
	return ERR_SUCCESS;
}

//TODO unterstützt nur Flächen die aus Dreiecken bestehen
//TODO man sollte übergeben können in welche location die Attribute gespeichert werden
ErrCode readObj(const char* filename, Triangle* storage, float* attributesBuffer, BYTE attributesCount, DWORD* count, float x, float y, float z, float scale=1)noexcept{
	std::fstream file; file.open(filename, std::ios::in);
	if(!file.is_open()) return ERR_MODEL_NOT_FOUND;
	if(attributesCount != 6) return ERR_MODEL_BAD_FORMAT;	//TODO neue Fehlermeldung
	std::string word;
	std::vector<fvec3> points;
	std::vector<fvec3> normals;
	std::vector<fvec2> uvs;
	DWORD current_count = *count;
	DWORD tri_count = 0;
	while(file >> word){	//Lese Datei Wort für Wort
		//TODO ifs sollten mit case ersetzt werden
		if(word[0] == 'o' && word.size() == 1){
			file >> word;
			std::cout << "Filename:               " << word << std::endl;
		}
		if(word[0] == 'v' && word.size() == 1){
			file >> word;
			fvec3 point;
			point.x = (std::atof(word.c_str())+x)*scale;
			file >> word;
			point.y = (std::atof(word.c_str())+y)*scale;
			file >> word;
			point.z = (std::atof(word.c_str())+z)*scale;
			points.push_back(point);
		}
		if(word[0] == 'v' && word[1] == 'n' && word.size() == 2){
			file >> word;
			fvec3 normal;
			normal.x = std::atof(word.c_str());
			file >> word;
			normal.y = std::atof(word.c_str());
			file >> word;
			normal.z = std::atof(word.c_str());
			normals.push_back(normal);
		}
		if(word[0] == 'v' && word[1] == 't' && word.size() == 2){
			file >> word;
			fvec2 uv;
			uv.x = std::atof(word.c_str());
			file >> word;
			uv.y = std::atof(word.c_str());
			uvs.push_back(uv);
		}
		if(word[0] == 'f' && word.size() == 1){
			DWORD pt_order[3];
			DWORD uv_order[3];
			DWORD normal_order[3];

			//Lese Punkt/Texture/Normal
			file >> word;
			if(splitString(word, pt_order[0], uv_order[0], normal_order[0]) != ERR_SUCCESS) return ERR_MODEL_BAD_FORMAT;
			file >> word;
			if(splitString(word, pt_order[1], uv_order[1], normal_order[1]) != ERR_SUCCESS) return ERR_MODEL_BAD_FORMAT;
			file >> word;
			if(splitString(word, pt_order[2], uv_order[2], normal_order[2]) != ERR_SUCCESS) return ERR_MODEL_BAD_FORMAT;

			storage[current_count+tri_count].points[0] = points[pt_order[0]];
			storage[current_count+tri_count].points[1] = points[pt_order[1]];
			storage[current_count+tri_count].points[2] = points[pt_order[2]];

			attributesBuffer[(current_count+tri_count)*attributesCount*3] = uvs[uv_order[0]].x;
			attributesBuffer[(current_count+tri_count)*attributesCount*3+1] = uvs[uv_order[0]].y;
			attributesBuffer[(current_count+tri_count)*attributesCount*3+2] = normals[normal_order[0]].x;
			attributesBuffer[(current_count+tri_count)*attributesCount*3+3] = normals[normal_order[0]].y;
			attributesBuffer[(current_count+tri_count)*attributesCount*3+4] = normals[normal_order[0]].z;
			attributesBuffer[(current_count+tri_count)*attributesCount*3+5] = uvs[uv_order[1]].x;
			attributesBuffer[(current_count+tri_count)*attributesCount*3+6] = uvs[uv_order[1]].y;
			attributesBuffer[(current_count+tri_count)*attributesCount*3+7] = normals[normal_order[1]].x;
			attributesBuffer[(current_count+tri_count)*attributesCount*3+8] = normals[normal_order[1]].y;
			attributesBuffer[(current_count+tri_count)*attributesCount*3+9] = normals[normal_order[1]].z;
			attributesBuffer[(current_count+tri_count)*attributesCount*3+10] = uvs[uv_order[2]].x;
			attributesBuffer[(current_count+tri_count)*attributesCount*3+11] = uvs[uv_order[2]].y;
			attributesBuffer[(current_count+tri_count)*attributesCount*3+12] = normals[normal_order[2]].x;
			attributesBuffer[(current_count+tri_count)*attributesCount*3+13] = normals[normal_order[2]].y;
			attributesBuffer[(current_count+tri_count)*attributesCount*3+14] = normals[normal_order[2]].z;

			++tri_count;
		}
	}
	*count += tri_count;
	std::cout << "Punkte gelesen:         " << points.size() << std::endl;
	std::cout << "UV-Koordinaten gelesen: " << uvs.size() << std::endl;
	std::cout << "Dreiecke gelesen:       " << tri_count << std::endl;
	std::cout << "Dreiecke insgesamt:     " << *count << '\n' << std::endl;
	return ERR_SUCCESS;
}

//Gibt das Keyword der obj/mtl Zeile als Zahlenwert zurück, für z.b. die Verwendung in einem switch case
//TODO Hashfunktion ist nun ja... billig, aber tut es fürs erste
constexpr WORD hashKeywords(const char* string)noexcept{
	size_t size = strlen(string);
	WORD out = 7;
	for(WORD i=0; i < size; ++i){
		out *= 31;
		out += string[i];
	}
	return out;
}

enum OBJKEYWORD{
	OBJ_V = hashKeywords("v"),
	OBJ_VT = hashKeywords("vt"),
	OBJ_VN = hashKeywords("vn"),
	OBJ_F = hashKeywords("f"),
	OBJ_S = hashKeywords("s"),
	OBJ_O = hashKeywords("o"),
	OBJ_G = hashKeywords("g"),
	OBJ_MTLLIB = hashKeywords("mtllib"),
	OBJ_USEMTL = hashKeywords("usemtl"),
	OBJ_COMMENT = hashKeywords("#"),
	OBJ_L = hashKeywords("l")
};

enum MTLKEYWORD{	//TODO hier fehlen noch ein paar
	MTL_NEWMTL = hashKeywords("newmtl"),
	MTL_MAP_KD = hashKeywords("map_Kd"),		//Diffuse
	MTL_MAP_BUMP = hashKeywords("map_Bump"),	//Normal
	MTL_MAP_KA = hashKeywords("map_Ka"),		//AO
	MTL_MAP_KS = hashKeywords("map_Ks"),		//Specular
	MTL_MAP_D = hashKeywords("map_d"),			//TODO Keine Ahnung eine MTL File hat das als map_Kd genutzt
	MTL_COMMENT = hashKeywords("#"),
	MTL_KA = hashKeywords("Ka"),
	MTL_KD = hashKeywords("Kd"),
	MTL_KS = hashKeywords("Ks"),
	MTL_KE = hashKeywords("Ke"),
	MTL_NI = hashKeywords("Ni"),
	MTL_NS = hashKeywords("Ns"),
	MTL_D = hashKeywords("d"),
	MTL_ILLUM = hashKeywords("illum")
};

//Liest ein Wort aus einer Datei wie der << Operator aber gibt zusätzlich zurück, ob \n oder eof gefunden wurde
bool readWord(std::fstream& file, std::string& buffer)noexcept{
	char c;
	buffer.clear();
	bool newline = false;
	c = file.peek();
	while(c == ' ' || c == '\n'){
		file.get();
		c = file.peek();
	}
	while(1){
		c = file.get();
		if(c == ' ') break;
		if(c == '\n'){
			newline = true;
			break;
		}
		if(file.eof()) return true;
		buffer += c;
	}
	while(1){
		c = file.peek();
		if(c == ' '){
			file.get();
			continue;
		}
		if(c == '\n'){
			newline = true;
			file.get();
			continue;
		}
		if(file.eof()) return true;
		break;
	}
	return newline;
}

//Liest so lange Daten in den Buffer, bis ein \n oder eof gefunden wurde
bool readLine(std::fstream& file, std::string& buffer)noexcept{
	buffer.clear();
	int c = file.get();
	while(1){
		if(file.eof()) return true;
		if(c == '\n') return true;
		buffer += c;
	}
}

//Ließt die obj Datei weiter ein bis zum nächsten \n und parsed die Linie basierend auf dem obj keyword, schriebt die Daten in den outData buffer
//und gibt evtl. Fehler zurück
ErrCode parseObjLine(OBJKEYWORD key, std::fstream& file, void* outData)noexcept{
	switch(key){
		case OBJ_VN:
		case OBJ_V:{
			std::string buffer;
			float* data = (float*)outData;
			if(readWord(file, buffer)) return ERR_MODEL_BAD_FORMAT;
			data[0] = std::atof(buffer.c_str());
			if(readWord(file, buffer)) return ERR_MODEL_BAD_FORMAT;
			data[1] = std::atof(buffer.c_str());
			if(!readWord(file, buffer)) return ERR_MODEL_BAD_FORMAT;
			data[2] = std::atof(buffer.c_str());
			break;
		}
		case OBJ_VT:{
			std::string buffer;
			float* data = (float*)outData;
			if(readWord(file, buffer)) return ERR_MODEL_BAD_FORMAT;
			data[0] = std::atof(buffer.c_str());
			if(!readWord(file, buffer)) return ERR_MODEL_BAD_FORMAT;
			data[1] = std::atof(buffer.c_str());
			break;
		}
		case OBJ_S:{	//TODO muss noch implementiert werden
			std::string buffer;
			if(!readWord(file, buffer)) return ERR_MODEL_BAD_FORMAT;
			break;
		}
		case OBJ_F:{
			std::string buffer;
			DWORD* data = (DWORD*)outData;
			if(readWord(file, buffer)) return ERR_MODEL_BAD_FORMAT;
			if(splitString(buffer, data[0], data[1], data[2]) != ERR_SUCCESS) return ERR_MODEL_BAD_FORMAT;
			if(readWord(file, buffer)) return ERR_MODEL_BAD_FORMAT;
			if(splitString(buffer, data[3], data[4], data[5]) != ERR_SUCCESS) return ERR_MODEL_BAD_FORMAT;
			if(!readWord(file, buffer)) return ERR_MODEL_BAD_FORMAT;
			if(splitString(buffer, data[6], data[7], data[8]) != ERR_SUCCESS) return ERR_MODEL_BAD_FORMAT;
			break;
		}
		case OBJ_O:
		case OBJ_G:
		case OBJ_MTLLIB:
		case OBJ_USEMTL:{
			std::string buffer;
			BYTE* data = (BYTE*)outData;
			if(!readWord(file, buffer)) return ERR_MODEL_BAD_FORMAT;
			for(size_t i=0; i < buffer.size(); ++i){
				data[i] = buffer[i];
			}
			data[buffer.size()] = '\0';
			break;
		}
		case OBJ_L:		//TODO muss noch implementiert werden
		case OBJ_COMMENT:{
			std::string buffer;
			BYTE* data = (BYTE*)outData;
			DWORD idx = 0;
			while(!readWord(file, buffer)){
				for(size_t i=0; i < buffer.size(); ++i) data[idx++] = buffer[i];
			}
			for(size_t i=0; i < buffer.size(); ++i) data[idx++] = buffer[i];
			data[idx] = '\0';
			break;
		}
		default: return ERR_MODEL_BAD_FORMAT;
	}
	return ERR_SUCCESS;
}

//Ließt die mtl Datei weiter ein bis zum nächsten \n und parsed die Linie basierend auf dem obj keyword, schriebt die daten in den outData buffer
//und gibt evtl. Fehler zurück
//TODO hier fehlen noch ein paar
ErrCode parseMtlLine(MTLKEYWORD key, std::fstream& file, void* outData)noexcept{
	switch(key){
		case MTL_KA:
		case MTL_KD:
		case MTL_KS:
		case MTL_KE:{
			std::string buffer;
			float* data = (float*)outData;
			if(readWord(file, buffer)) return ERR_MODEL_BAD_FORMAT;
			data[0] = std::atof(buffer.c_str());
			if(readWord(file, buffer)) return ERR_MODEL_BAD_FORMAT;
			data[1] = std::atof(buffer.c_str());
			if(!readWord(file, buffer)) return ERR_MODEL_BAD_FORMAT;
			data[2] = std::atof(buffer.c_str());
			break;
		}
		case MTL_D:
		case MTL_NI:
		case MTL_NS:
		case MTL_ILLUM:
		case MTL_NEWMTL:{
			std::string buffer;
			BYTE* data = (BYTE*)outData;
			if(!readWord(file, buffer)) return ERR_MODEL_BAD_FORMAT;
			for(size_t i=0; i < buffer.size(); ++i){
				data[i] = buffer[i];
			}
			data[buffer.size()] = '\0';
			break;
		}
		case MTL_MAP_BUMP:{
			break;
		}
		case MTL_MAP_D:
		case MTL_MAP_KD:
		case MTL_MAP_KS:
		case MTL_MAP_KA:
		case MTL_COMMENT:{
			std::string buffer;
			BYTE* data = (BYTE*)outData;
			DWORD idx = 0;
			while(!readWord(file, buffer)){
				for(size_t i=0; i < buffer.size(); ++i) data[idx++] = buffer[i];
				data[idx++] = ' ';	//TODO EINFACH NUR FALSCH, DAS IST NUR HIER WEIL ICH ZU FAUL WAR MIR EINE BESSERE LÖSUNG FÜR DAS PFAD PROBLEM ZU ÜBERLEGEN
				//Problem ist ein Pfad mit einem Leerzeichen, statt readWord sollte man eine Funktion schreiben die alles bis zum \n einliest, duh
			}
			for(size_t i=0; i < buffer.size(); ++i) data[idx++] = buffer[i];
			data[idx] = '\0';
			break;
		}
		default: return ERR_MODEL_BAD_FORMAT;
	}
	return ERR_SUCCESS;
}

//TODO noch nicht alle Keywords werden beachtet
//TODO Theoretisch noch nicht optimal implementiert, da jede Zeile erst in einen Buffer gelesen wird und dieser dann erst geparsed
//TODO Liest aktuell die Texturen in feste Einträge im Texture Array des Materials
ErrCode loadMtl(const char* filename, Material* materials, DWORD& materialCount)noexcept{
	std::fstream file;
	file.open(filename, std::ios::in);
	if(!file.is_open()) return ERR_MATERIAL_NOT_FOUND;
	std::string word;
	void* data[120];			//TODO Manche Texturenpfade könnten länger sein
	DWORD lineNumber = 0;
	while(file >> word){
		lineNumber++;
		MTLKEYWORD key = (MTLKEYWORD)hashKeywords(word.c_str());
		switch(key){
			case MTL_NEWMTL:{
				if(parseMtlLine(key, file, data) != ERR_SUCCESS) return ErrCheck(ERR_MATERIAL_BAD_FORMAT, std::string(word + " in Zeile: " + longToString(lineNumber)).c_str());
				materialCount++;
				materials[materialCount-1].name = std::string((char*)data);
				break;
			}
			case MTL_MAP_BUMP:{
				break;
			}
			case MTL_MAP_D:		//Kein Plan ob das richtig ist
			case MTL_MAP_KS:
			case MTL_MAP_KD:{
				BYTE textureIdx = 0;
				switch(key){
					case MTL_MAP_KS:
						textureIdx = 1;
						break;
				}
				if(parseMtlLine(key, file, data) != ERR_SUCCESS) return ErrCheck(ERR_MATERIAL_BAD_FORMAT, std::string(word + " in Zeile: " + longToString(lineNumber)).c_str());
				std::string textureFile = std::string((char*)data);
				size_t index = 0;
				while(true){
					index = textureFile.find("\\", index);
					if(index == std::string::npos) break;
					textureFile.replace(index, 2, "/");
					index += 2;
				}
				if(materials[materialCount-1].textures[textureIdx].data == nullptr) destroyImage(materials[materialCount-1].textures[textureIdx]);
				if(textureFile.size() < 4) return ErrCheck(ERR_MATERIAL_BAD_FORMAT, std::string(word + " in Zeile " + longToString(lineNumber)).c_str());
				std::string fileEnding = textureFile.substr(textureFile.size()-4, 4);
				if(fileEnding == ".tex"){
					if(ErrCheck(loadImage(textureFile.c_str(), materials[materialCount-1].textures[textureIdx]), "Texture laden") != ERR_SUCCESS) return ErrCheck(ERR_MATERIAL_BAD_FORMAT, std::string(word + " in Zeile: " + longToString(lineNumber)).c_str());
				}
				else if(fileEnding == ".png"){
					if(ErrCheck(loadPng(textureFile.c_str(), materials[materialCount-1].textures[textureIdx]), "Texture laden") != ERR_SUCCESS) return ErrCheck(ERR_MATERIAL_BAD_FORMAT, std::string(word + " in Zeile: " + longToString(lineNumber)).c_str());
				}
				flipImageVertically(materials[materialCount-1].textures[textureIdx]);	//TODO warum nur ist das nötig?
				materials[materialCount-1].textureCount = 1;
				//TODO Das ist nur eine Übergangslösung und sollte besser implementiert werden
				DWORD redBuffer[256]{0};
				DWORD greenBuffer[256]{0};
				DWORD blueBuffer[256]{0};
				DWORD redMax = 0;
				BYTE redIdx = 0;
				DWORD greenMax = 0;
				BYTE greenIdx = 0;
				DWORD blueMax = 0;
				BYTE blueIdx = 0;
				for(DWORD i=0; i < materials[materialCount-1].textures[textureIdx].width*materials[materialCount-1].textures[textureIdx].height; ++i){
					redBuffer[R(materials[materialCount-1].textures[textureIdx].data[i])] += 1;
					greenBuffer[G(materials[materialCount-1].textures[textureIdx].data[i])] += 1;
					blueBuffer[B(materials[materialCount-1].textures[textureIdx].data[i])] += 1;
				}
				for(WORD i=0; i < 256; ++i){
					if(redBuffer[i] > redMax){
						redMax = redBuffer[i];
						redIdx = i;
					}
					if(greenBuffer[i] > greenMax){
						greenMax = greenBuffer[i];
						greenIdx = i;
					}
					if(blueBuffer[i] > blueMax){
						blueMax = blueBuffer[i];
						blueIdx = i;
					}
				}
				materials[materialCount-1].baseColor = RGBA(redIdx, greenIdx, blueIdx);
				break;
			}
			case MTL_KS:
			case MTL_KD:{
				BYTE textureIdx = 0;
				switch(key){
					case MTL_KS:
						textureIdx = 1;
						break;
				}
				if(parseMtlLine(key, file, data) != ERR_SUCCESS) return ErrCheck(ERR_MATERIAL_BAD_FORMAT, std::string(word + " in Zeile: " + longToString(lineNumber)).c_str());
				float* values = (float*)data;
				if(materials[materialCount-1].textures[textureIdx].data != nullptr) break;
				if(ErrCheck(createBlankImage(materials[materialCount-1].textures[textureIdx], 4, 4, RGBA(values[0]*255, values[1]*255, values[2]*255)), "Texture laden") != ERR_SUCCESS) return ErrCheck(ERR_MATERIAL_BAD_FORMAT, std::string(word + " in Zeile: " + longToString(lineNumber)).c_str());
				materials[materialCount-1].textureCount = 1;
				materials[materialCount-1].baseColor = RGBA(values[0]*255, values[1]*255, values[2]*255);
				break;
			}
			case MTL_NS:	//TODO müssen alle noch implementiert werden
			case MTL_MAP_KA:
			case MTL_KA:
			case MTL_KE:
			case MTL_NI:
			case MTL_D:
			case MTL_ILLUM:
			case MTL_COMMENT:{
				if(parseMtlLine(key, file, data) != ERR_SUCCESS) return ErrCheck(ERR_MATERIAL_BAD_FORMAT, std::string(word + " in Zeile: " + longToString(lineNumber)).c_str());
				break;
			}
			default: return ErrCheck(ERR_MATERIAL_BAD_FORMAT, std::string(word + " Unbekanntes MTL Keyword in Zeile: " + longToString(lineNumber)).c_str());
		}
	}
	return ERR_SUCCESS;
}

//Speichert die Modelle der .obj Datei und alle .mtl Materialen falls es diese noch nicht gibt 
//TODO man sollte übergeben können in welche location die Attribute gespeichert werden
//TODO unterstützt nur Flächen die aus Dreiecken bestehen
//TODO Theoretisch noch nicht optimal implementiert, da jede Zeile erst in einen Buffer gelesen wird und dieser dann erst geparsed
ErrCode loadObj(const char* filename, TriangleModel* models, DWORD& modelCount, Material* materials, DWORD& materialCount, BYTE attributeCount, float x, float y, float z, float scaleX=1, float scaleY=1, float scaleZ=1)noexcept{
	std::fstream file;
	file.open(filename, std::ios::in);
	if(!file.is_open()) return ERR_MODEL_NOT_FOUND;
	if(models[0].attributesCount < 5) return ERR_GENERIC_ERROR;		//TODO Neue Fehlermeldung
	std::string word;
	std::vector<fvec3> points;
	std::vector<fvec3> normals;
	std::vector<fvec2> uvs;
	void* data[80];
	bool hasMaterial = false;
	DWORD lineNumber = 0;
	BYTE windingOrder[] = {0, 1, 2};
	if(((sign(scaleX)+sign(scaleY)+sign(scaleZ))%2) == 0){
		windingOrder[0] = 2;
		windingOrder[2] = 0;
	}
	while(file >> word){
		lineNumber++;
		OBJKEYWORD key = (OBJKEYWORD)hashKeywords(word.c_str());
		switch(key){
			case OBJ_O:{
				if(parseObjLine(key, file, data) != ERR_SUCCESS) return ErrCheck(ERR_MODEL_BAD_FORMAT, std::string(word + " in Zeile: " + longToString(lineNumber)).c_str());
				break;
			}
			case OBJ_V:{
				if(parseObjLine(key, file, data) != ERR_SUCCESS) return ErrCheck(ERR_MODEL_BAD_FORMAT, std::string(word + " in Zeile: " + longToString(lineNumber)).c_str());
				points.push_back({((float*)data)[0]*scaleX, ((float*)data)[1]*scaleY, ((float*)data)[2]*scaleZ});
				break;
			}
			case OBJ_VN:{
				if(parseObjLine(key, file, data) != ERR_SUCCESS) return ErrCheck(ERR_MODEL_BAD_FORMAT, std::string(word + " in Zeile: " + longToString(lineNumber)).c_str());
				normals.push_back({((float*)data)[0], ((float*)data)[1], ((float*)data)[2]});
				break;
			}
			case OBJ_VT:{
				if(parseObjLine(key, file, data) != ERR_SUCCESS) return ErrCheck(ERR_MODEL_BAD_FORMAT, std::string(word + " in Zeile: " + longToString(lineNumber)).c_str());
				uvs.push_back({((float*)data)[0], ((float*)data)[1]});
				break;
			}
			case OBJ_F:{
				DWORD pt_order[3];
				DWORD uv_order[3];
				DWORD normal_order[3];

				if(models[modelCount-1].triangleCount >= models[modelCount-1].triangleCapacity) increaseTriangleCapacity(models[modelCount-1], 100);	//TODO 100
				
				//Lese Punkt/Texture/Normal
				if(parseObjLine(key, file, data) != ERR_SUCCESS) return ErrCheck(ERR_MODEL_BAD_FORMAT, std::string(word + " in Zeile: " + longToString(lineNumber)).c_str());
				pt_order[0] = ((DWORD*)data)[0]; uv_order[0] = ((DWORD*)data)[1]; normal_order[0] = ((DWORD*)data)[2];
				pt_order[1] = ((DWORD*)data)[3]; uv_order[1] = ((DWORD*)data)[4]; normal_order[1] = ((DWORD*)data)[5];
				pt_order[2] = ((DWORD*)data)[6]; uv_order[2] = ((DWORD*)data)[7]; normal_order[2] = ((DWORD*)data)[8];

				if(modelCount == 0) return ErrCheck(ERR_MODEL_BAD_FORMAT, "f modelCount == 0 aka keine usemtl Zeile vor einer f Zeile");
				DWORD modelIdx = modelCount-1;
				DWORD triangleIdx = models[modelIdx].triangleCount;

				models[modelIdx].triangles[triangleIdx].points[0] = points[pt_order[windingOrder[0]]];
				models[modelIdx].triangles[triangleIdx].points[1] = points[pt_order[windingOrder[1]]];
				models[modelIdx].triangles[triangleIdx].points[2] = points[pt_order[windingOrder[2]]];

				DWORD attributeBaseIdx = triangleIdx*models[modelIdx].attributesCount*3;
				if(uvs.size() > 0){
					models[modelIdx].attributesBuffer[attributeBaseIdx] = uvs[uv_order[windingOrder[0]]].x;
					models[modelIdx].attributesBuffer[attributeBaseIdx+1] = uvs[uv_order[windingOrder[0]]].y;
				}
				models[modelIdx].attributesBuffer[attributeBaseIdx+2] = normals[normal_order[windingOrder[0]]].x*-negSign(scaleX);
				models[modelIdx].attributesBuffer[attributeBaseIdx+3] = normals[normal_order[windingOrder[0]]].y*-negSign(scaleY);	//TODO Warum müssen die negativ sein?
				models[modelIdx].attributesBuffer[attributeBaseIdx+4] = normals[normal_order[windingOrder[0]]].z*-negSign(scaleZ);
				attributeBaseIdx += models[modelIdx].attributesCount;
				if(uvs.size() > 0){
					models[modelIdx].attributesBuffer[attributeBaseIdx] = uvs[uv_order[windingOrder[1]]].x;
					models[modelIdx].attributesBuffer[attributeBaseIdx+1] = uvs[uv_order[windingOrder[1]]].y;
				}
				models[modelIdx].attributesBuffer[attributeBaseIdx+2] = normals[normal_order[windingOrder[1]]].x*-negSign(scaleX);
				models[modelIdx].attributesBuffer[attributeBaseIdx+3] = normals[normal_order[windingOrder[1]]].y*-negSign(scaleY);
				models[modelIdx].attributesBuffer[attributeBaseIdx+4] = normals[normal_order[windingOrder[1]]].z*-negSign(scaleZ);
				attributeBaseIdx += models[modelIdx].attributesCount;
				if(uvs.size() > 0){
					models[modelIdx].attributesBuffer[attributeBaseIdx] = uvs[uv_order[windingOrder[2]]].x;
					models[modelIdx].attributesBuffer[attributeBaseIdx+1] = uvs[uv_order[windingOrder[2]]].y;
				}
				models[modelIdx].attributesBuffer[attributeBaseIdx+2] = normals[normal_order[windingOrder[2]]].x*-negSign(scaleX);
				models[modelIdx].attributesBuffer[attributeBaseIdx+3] = normals[normal_order[windingOrder[2]]].y*-negSign(scaleY);
				models[modelIdx].attributesBuffer[attributeBaseIdx+4] = normals[normal_order[windingOrder[2]]].z*-negSign(scaleZ);

				models[modelIdx].triangleCount++;
				break;
			}
			case OBJ_MTLLIB:{
				if(parseObjLine(key, file, data) != ERR_SUCCESS) return ErrCheck(ERR_MODEL_BAD_FORMAT, std::string(word + " in Zeile: " + longToString(lineNumber)).c_str());
				size_t lastSlash = std::string(filename).find_last_of("/");
				std::string path = std::string(filename).substr(0, lastSlash+1);
				std::string mtlFile = path + std::string((char*)data);
				ErrCode err = loadMtl(mtlFile.c_str(), materials, materialCount);
				if(err == ERR_MATERIAL_NOT_FOUND){
					ErrCheck(err, std::string(word + " Material Bibliothek nicht gefunden... fahre ohne fort... in Zeile: " + longToString(lineNumber)).c_str());
					break;
				}
				if(ErrCheck(err, "Mtl Datei laden") != ERR_SUCCESS) return ErrCheck(ERR_MODEL_BAD_FORMAT, std::string(word + " in Zeile: " + longToString(lineNumber)).c_str());
				hasMaterial = true;
				break;
			}
			case OBJ_USEMTL:{
				modelCount++;
				if(!hasMaterial){
					if(parseObjLine(key, file, data) != ERR_SUCCESS) return ErrCheck(ERR_MODEL_BAD_FORMAT, std::string(word + " in Zeile: " + longToString(lineNumber)).c_str());
					ErrCheck(ERR_MODEL_BAD_FORMAT, std::string(word + " Material angefordert, Datei hat aber keins in Zeile: " + longToString(lineNumber)).c_str());
					break;
				}
				if(parseObjLine(key, file, data) != ERR_SUCCESS) return ErrCheck(ERR_MODEL_BAD_FORMAT, std::string(word + " in Zeile: " + longToString(lineNumber)).c_str());
				DWORD i=0;
				for(;i < materialCount; ++i){
					if(strcmp((char*)data, materials[i].name.c_str()) == 0){
						models[modelCount-1].material = &materials[i];
						break;
					}
				}
				if(i == materialCount) return ErrCheck(ERR_MODEL_BAD_FORMAT, std::string(word + " Material nicht gefunden in Zeile: " + longToString(lineNumber)).c_str());
				break;
			}
			case OBJ_G:			//TODO müssen alle noch implementiert werden
			case OBJ_S:
			case OBJ_L:
			case OBJ_COMMENT:{
				if(parseObjLine(key, file, data) != ERR_SUCCESS) return ErrCheck(ERR_MODEL_BAD_FORMAT, std::string(word + " in Zeile: " + longToString(lineNumber)).c_str());
				break;
			}
			default: return ErrCheck(ERR_MODEL_BAD_FORMAT, std::string(word + " Unbekanntes OBJ Keyword in Zeile: " + longToString(lineNumber)).c_str());
		}
	}
	return ERR_SUCCESS;
}

//TODO Sollte auch in eine eigene Datei
struct SDF{
	fvec3 pos;
	fvec3 size;	//Die Größe des SDF
	DWORD dx;	//Die interne Auflösung, also dx*dy*dz
	DWORD dy;
	DWORD dz;
	float* data = nullptr;	//Daten erst über X, dann Y, dann Z
	BYTE* ids = nullptr;	//ID für jeden Datenpunkt
	DWORD* color = nullptr;	//Farbe für jeden Datenpunkt
};

ErrCode createSDF(SDF& sdf, fvec3 pos, fvec3 size, DWORD dx, DWORD dy, DWORD dz)noexcept{
	sdf.pos = pos;
	sdf.size = size;
	sdf.dx = dx;
	sdf.dy = dy;
	sdf.dz = dz;
	sdf.data = alloc<float>(dx*dy*dz, "SDFdata");
	sdf.ids = alloc<BYTE>(dx*dy*dz, "SDFids");
	sdf.color = alloc<DWORD>(dx*dy*dz, "SDFcolors");
	if(sdf.data == nullptr) return ERR_BAD_ALLOC;
	if(sdf.ids == nullptr) return ERR_BAD_ALLOC;
	if(sdf.color == nullptr) return ERR_BAD_ALLOC;
	return ERR_SUCCESS;
}

void destroySDF(SDF& sdf)noexcept{
	dealloc(sdf.data);
	dealloc(sdf.ids);
	dealloc(sdf.color);
}

void setSDFValues(SDF& sdf, float value)noexcept{
	for(DWORD i=0; i < sdf.dx*sdf.dy*sdf.dz; ++i) sdf.data[i] = value;
}

void calculateSDFBounds(SDF& sdf, const TriangleModel* models, DWORD modelCount, const fvec3& extend = {10, 10, 10})noexcept{
	fvec3 minPos = {FLOAT_MAX, FLOAT_MAX, FLOAT_MAX};
	fvec3 maxPos = {FLOAT_MIN, FLOAT_MIN, FLOAT_MIN};
	std::numeric_limits<float>::min();
	for(DWORD i=0; i < modelCount; ++i){
		for(DWORD j=0; j < models[i].triangleCount; ++j){
			for(BYTE k=0; k < 3; ++k){
				minPos.x = min(models[i].triangles[j].points[k].x, minPos.x);
				minPos.y = min(models[i].triangles[j].points[k].y, minPos.y);
				minPos.z = min(models[i].triangles[j].points[k].z, minPos.z);
				maxPos.x = max(models[i].triangles[j].points[k].x, maxPos.x);
				maxPos.y = max(models[i].triangles[j].points[k].y, maxPos.y);
				maxPos.z = max(models[i].triangles[j].points[k].z, maxPos.z);
			}
		}
	}
	sdf.pos = {minPos.x - extend.x, minPos.y - extend.y, minPos.z - extend.z};
	sdf.size = {maxPos.x - minPos.x + extend.x*2, maxPos.y - minPos.y + extend.y*2, maxPos.z - minPos.z + extend.z*2};
}

float pointToTriangleDistance(const fvec3& p, Triangle& tri){
	fvec3 p21 = {tri.points[2].x - tri.points[1].x, tri.points[2].y - tri.points[1].y, tri.points[2].z - tri.points[1].z};
	fvec3 p20 = {tri.points[2].x - tri.points[0].x, tri.points[2].y - tri.points[0].y, tri.points[2].z - tri.points[0].z};
	fvec3 normal = normalize(cross(p21, p20));
	fvec3 diff = {p.x - tri.points[0].x, p.y - tri.points[0].y, p.z - tri.points[0].z};
	float dist = dot(normal, diff);
	fvec3 projP = {p.x - normal.x*dist, p.y - normal.y*dist, p.z - normal.z*dist};
	fvec3 v0 = {tri.points[1].x - tri.points[0].x, tri.points[1].y - tri.points[0].y, tri.points[1].z - tri.points[0].z};
    fvec3 v1 = {tri.points[2].x - tri.points[0].x, tri.points[2].y - tri.points[0].y, tri.points[2].z - tri.points[0].z};
    fvec3 v2 = {projP.x - tri.points[0].x, projP.y - tri.points[0].y, projP.z - tri.points[0].z};

    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d11 = dot(v1, v1);
    float d20 = dot(v2, v0);
    float d21 = dot(v2, v1);
    float denom = 1.f/(d00 * d11 - d01 * d01);
    float v = (d11 * d20 - d01 * d21)*denom;
    float w = (d00 * d21 - d01 * d20)*denom;

    if(w >= 0 && v >= 0 && w + v <= 1) return fabs(dist);

	fvec3 p1 = closestPointOnLineSegment(p, tri.points[0], tri.points[1]);
	fvec3 p2 = closestPointOnLineSegment(p, tri.points[0], tri.points[2]);
	fvec3 p3 = closestPointOnLineSegment(p, tri.points[1], tri.points[2]);
	float dist1 = length({p1.x - p.x, p1.y - p.y, p1.z - p.z});
	float dist2 = length({p2.x - p.x, p2.y - p.y, p2.z - p.z});
	float dist3 = length({p3.x - p.x, p3.y - p.y, p3.z - p.z});
	return min(dist3, min(dist2, dist1));
}

void calculateSDFFromMesh(SDF& sdf, const TriangleModel& model, BYTE id)noexcept{
    float dx = sdf.size.x/sdf.dx;
	float dy = sdf.size.y/sdf.dy;
	float dz = sdf.size.z/sdf.dz;
	BYTE* marked = alloc<BYTE>(sdf.dx*sdf.dy*sdf.dz, "SDF-Gen-MarkedArray");
	for(DWORD i=0; i < sdf.dx*sdf.dy*sdf.dz; ++i) marked[i] = 0;
	uivec3* bfsBuffer = alloc<uivec3>(sdf.dx*sdf.dy*sdf.dz*4, "SDF-Gen-BFSBuffer");		//TODO Solle ein Vektor werden oder so
	DWORD startIdx = 0;
	DWORD endIdx = 0;
	for(DWORD i=0; i < model.triangleCount; ++i){
		fvec3 point1 = model.triangles[i].points[0];
		fvec3 point2 = model.triangles[i].points[1];
		fvec3 point3 = model.triangles[i].points[2];
		fvec3 minExtend = {min(point1.x, min(point2.x, point3.x)), min(point1.y, min(point2.y, point3.y)), min(point1.z, min(point2.z, point3.z))};
		fvec3 maxExtend = {max(point1.x, max(point2.x, point3.x)), max(point1.y, max(point2.y, point3.y)), max(point1.z, max(point2.z, point3.z))};
		//TODO Könnte in ein DWORD transformiert werden, damit < 0 nicht mehr getestet werden muss
		int minX = clamp((int)(((minExtend.x-sdf.pos.x)*sdf.dx)/sdf.size.x)-1, 0, sdf.dx-1);
		int maxX = clamp((int)(((maxExtend.x-sdf.pos.x)*sdf.dx)/sdf.size.x)+1, 0, sdf.dx-1);
		int minY = clamp((int)(((minExtend.y-sdf.pos.y)*sdf.dy)/sdf.size.y)-1, 0, sdf.dy-1);
		int maxY = clamp((int)(((maxExtend.y-sdf.pos.y)*sdf.dy)/sdf.size.y)+1, 0, sdf.dy-1);
		int minZ = clamp((int)(((minExtend.z-sdf.pos.z)*sdf.dz)/sdf.size.z)-1, 0, sdf.dz-1);
		int maxZ = clamp((int)(((maxExtend.z-sdf.pos.z)*sdf.dz)/sdf.size.z)+1, 0, sdf.dz-1);
		fvec3 pos;
		for(int z=minZ; z <= maxZ; ++z){
			pos.z = sdf.pos.z + z * dz;
			for(int y=minY; y <= maxY; ++y){
				pos.y = sdf.pos.y + y * dy;
				for(int x=minX; x <= maxX; ++x){
					pos.x = sdf.pos.x + x * dx;
					DWORD idx = z*sdf.dy*sdf.dx+y*sdf.dx+x;
					float dst = pointToTriangleDistance(pos, model.triangles[i]);
            		if(dst < sdf.data[idx]){
						sdf.data[idx] = dst;
						sdf.color[idx] = model.material->baseColor;		//TODO material könnte ein nullptr sein
						sdf.ids[idx] = id;
					}
					if(marked[idx] == 0) bfsBuffer[endIdx++] = {(DWORD)x, (DWORD)y, (DWORD)z};
					marked[idx] = 1;
				}
			}
		}
	}
	int dxDir[26] = {1, -1, 0, 0, 0, 0,  1, -1,  1, -1,  1, -1,  0, 0,  0, 0,  1, -1,  1, -1,  1, -1,  1, -1, 1, -1};
	int dyDir[26] = {0, 0, 1, -1, 0, 0,  1, -1, -1,  1,  0,  0,  1, -1,  1, -1,  1, -1,  1, -1,  0,  0, -1,  1, 1, -1};
	int dzDir[26] = {0, 0, 0, 0, 1, -1,  0,  0,  0,  0,  1, -1,  1,  1, -1, -1,  1,  1, -1, -1,  1,  1, -1, -1, 1, -1};
	float distances[26] = {
		dx, dx, dy, dy, dz, dz,
		sqrt(dx*dx + dy*dy), sqrt(dx*dx + dy*dy),
		sqrt(dx*dx + dz*dz), sqrt(dx*dx + dz*dz),
		sqrt(dy*dy + dz*dz), sqrt(dy*dy + dz*dz),
		sqrt(dx*dx + dy*dy), sqrt(dx*dx + dy*dy),
		sqrt(dx*dx + dz*dz), sqrt(dx*dx + dz*dz),
		sqrt(dx*dx + dy*dy + dz*dz), sqrt(dx*dx + dy*dy + dz*dz),
		sqrt(dx*dx + dy*dy + dz*dz), sqrt(dx*dx + dy*dy + dz*dz),
		sqrt(dx*dx + dy*dy + dz*dz), sqrt(dx*dx + dy*dy + dz*dz),
		sqrt(dx*dx + dy*dy + dz*dz), sqrt(dx*dx + dy*dy + dz*dz),
		sqrt(dx*dx + dy*dy + dz*dz), sqrt(dx*dx + dy*dy + dz*dz)
	};
	while(startIdx < endIdx){
		uivec3 point = bfsBuffer[startIdx++];
		for(int i=0; i < 26; ++i){
			DWORD x = point.x + dxDir[i];
            DWORD y = point.y + dyDir[i];
            DWORD z = point.z + dzDir[i];
			if(x >= sdf.dx || y >= sdf.dy || z >= sdf.dz) continue;
			DWORD curIdx = point.z*sdf.dy*sdf.dx+point.y*sdf.dx+point.x;
			DWORD newIdx = z*sdf.dy*sdf.dx+y*sdf.dx+x;
			if(marked[newIdx] == 1) continue;
			float dist = sdf.data[curIdx] + distances[i];
			if(dist < sdf.data[newIdx]){
				sdf.data[newIdx] = dist;
				sdf.color[newIdx] = sdf.color[curIdx];
				sdf.ids[newIdx] = sdf.ids[curIdx];
				bfsBuffer[endIdx++] = {x, y, z};
			}
		}
	}
	dealloc(marked);
	dealloc(bfsBuffer);
}

void calculateSDFFromMeshOld(SDF& sdf, const TriangleModel& model)noexcept{
    float dx = sdf.size.x/sdf.dx;
    float dy = sdf.size.y/sdf.dy;
    float dz = sdf.size.z/sdf.dz;
    fvec3 pos = sdf.pos;
    DWORD idx = 0;
    for(DWORD i=0; i < sdf.dz; ++i){
        pos.z = sdf.pos.z + i * dz;
        for(DWORD j=0; j < sdf.dy; ++j){
            pos.y = sdf.pos.y + j * dy;
            for(DWORD k=0; k < sdf.dx; ++k){
                pos.x = sdf.pos.x + k * dx;
				for(DWORD l=0; l < model.triangleCount; ++l){
                    float dst = pointToTriangleDistance(pos, model.triangles[l]);
                    if(dst < sdf.data[idx]) sdf.data[idx] = dst;
                }
                idx++;
            }
        }
    }
}

float getSDFDistanceFromPosition(SDF& sdf, const fvec3& position)noexcept{
	DWORD x = (DWORD)((position.x-sdf.pos.x)*sdf.dx)/sdf.size.x;
	DWORD y = (DWORD)((position.y-sdf.pos.y)*sdf.dy)/sdf.size.y;
	DWORD z = (DWORD)((position.z-sdf.pos.z)*sdf.dz)/sdf.size.z;
	if(x >= sdf.dx || y >= sdf.dy || z >= sdf.dz) return -1;
	return sdf.data[z*sdf.dy*sdf.dx+y*sdf.dx+x];
}

DWORD getSDFColorFromPosition(SDF& sdf, const fvec3& position)noexcept{
	DWORD x = (DWORD)((position.x-sdf.pos.x)*sdf.dx)/sdf.size.x;
	DWORD y = (DWORD)((position.y-sdf.pos.y)*sdf.dy)/sdf.size.y;
	DWORD z = (DWORD)((position.z-sdf.pos.z)*sdf.dz)/sdf.size.z;
	if(x >= sdf.dx || y >= sdf.dy || z >= sdf.dz) return RGBA(0, 0, 0);
	return sdf.color[z*sdf.dy*sdf.dx+y*sdf.dx+x];
}

constexpr DWORD colorPicker(BYTE id)noexcept{
	switch(id%14){
		case 0: return RGBA(128, 128, 128);
		case 1: return RGBA(255, 0, 0);
		case 2: return RGBA(0, 255, 0);
		case 3: return RGBA(0, 0, 255);
		case 4: return RGBA(255, 255, 0);
		case 5: return RGBA(255, 0, 255);
		case 6: return RGBA(0, 255, 255);
		case 7: return RGBA(255, 255, 255);
		case 8: return RGBA(128, 0, 0);
		case 9: return RGBA(0, 128, 0);
		case 10: return RGBA(0, 0, 128);
		case 11: return RGBA(128, 128, 0);
		case 12: return RGBA(128, 0, 128);
		case 13: return RGBA(0, 128, 128);
	}
	return RGBA(128, 128, 128);
}

void drawSDF(RenderBuffers& renderBuffers, SDF& sdf, WORD radius, float maxVal, Camera& cam, float (*pointScalingFunction)(WORD, float) noexcept)noexcept{
	ColorPoint* pts = alloc<ColorPoint>(sdf.dx*sdf.dy*sdf.dz, "Draw-SDF-ColorpointBuffer");
	DWORD count = 0;
	float dx = sdf.size.x/sdf.dx;
	float dy = sdf.size.y/sdf.dy;
	float dz = sdf.size.z/sdf.dz;
	fvec3 pos;
	for(int z=0; z < sdf.dz; ++z){
		pos.z = sdf.pos.z + z * dz;
		for(int y=0; y < sdf.dy; ++y){
			pos.y = sdf.pos.y + y * dy;
			for(int x=0; x < sdf.dx; ++x){
				pos.x = sdf.pos.x + x * dx;
				DWORD idx = z*sdf.dy*sdf.dx+y*sdf.dx+x;
				float value = sdf.data[idx];
				if(value <= maxVal){
					pts[count].pos = pos;
					// pts[count].color = colorPicker(sdf.ids[idx]);
					pts[count].color = sdf.color[idx];
					count++;
				}
			}
		}
	}
	for(DWORD i=0; i < renderBuffers.width*renderBuffers.height; ++i) renderBuffers.depthBuffer[i] = FLOAT_MAX;
	drawPointsDepth(renderBuffers, pts, 0, count, radius, cam, pointScalingFunction);
	dealloc(pts);
}
