#pragma once

#include <windows.h>
#include <windowsx.h>
#include <fstream>
#include <d2d1.h>

#include "util.h"
#include "math.h"

//TODO namespace?
//TODO triangle struct sollte weg... lieber alles per pointer übergeben, dann kann man auch sauber punkte, attribute,... trennen und viel wichtiger die Dreiecke
//haben nicht immer einen pointer der vllt nicht gebraucht wird, da es keine attribute gibt

#define INVALIDHANDLEERRORS
#define MAXVERTEXATTRIBUTES 32

#define WINDOWFLAGSTYPE BYTE
enum WINDOWFLAG : WINDOWFLAGSTYPE{
	WINDOW_NONE = 0,
	WINDOW_CLOSE = 1,
	WINDOW_RESIZE = 2
};
//Hat viele Attribute die man auch über die win api abrufen könnte, aber diese extra zu speichern macht alles übersichtlicher
struct Window{
	HWND handle;									//Fensterhandle
	WORD windowWidth = 800;							//Fensterbreite
	WORD windowHeight = 800;						//Fensterhöhe
	DWORD* pixels;									//Pixelarray
	DWORD* depth;									//Depthbuffer
	WORD pixelSize = 1;								//Größe der Pixel in Bildschirmpixeln
	WINDOWFLAG flags = WINDOW_NONE;					//Fensterflags
	ID2D1HwndRenderTarget* renderTarget = nullptr;	//Direct 2D Rendertarget
	std::string windowClassName;					//Ja, jedes Fenster hat seine eigene Klasse... GROSSES TODO
	BYTE attributeBuffersCount = 0;					//Anzahl der Vertex-Attribute Buffer
	//TODO float dafür zu verwenden ist ja eigentlich doof, da die Zahlen eh nur zwischen 0-1 sind...
	float* attributeBuffers[MAXVERTEXATTRIBUTES];	//Buffer für die interpolierten Vertex-Attribute
};

#define APPLICATIONFLAGSTYPE BYTE
enum APPLICATIONFLAG : APPLICATIONFLAGSTYPE{
	APP_RUNNING=1,
	APP_HAS_DEVICE=2
};
#define MAX_WINDOW_COUNT 10
struct Application{
	APPLICATIONFLAG flags = APP_RUNNING;		//Applikationsflags
	ID2D1Factory* factory = nullptr;			//Direct2D Factory
}; static Application app;

inline bool getAppFlag(APPLICATIONFLAG flag){return(app.flags & flag);}
inline void setAppFlag(APPLICATIONFLAG flag){app.flags = (APPLICATIONFLAG)(app.flags | flag);}
inline void resetAppFlag(APPLICATIONFLAG flag){app.flags = (APPLICATIONFLAG)(app.flags & ~flag);}

inline ErrCode initApp(){
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &app.factory);
	if(hr){
		std::cerr << hr << std::endl;
		return APP_INIT;
	}
	return SUCCESS;
}

//TODO wirft das errors?
inline ErrCode destroyApp(){
	app.factory->Release();
	return SUCCESS;
}

typedef LRESULT (*window_callback_function)(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK default_window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//Setzt bei Erfolg den Parameter window zu einem gültigen window, auf welches man dann zugreifen kann
//window muss ein nullptr sein/kein vorhandenes window!
ErrCode createWindow(HINSTANCE hInstance, LONG windowWidth, LONG windowHeight, LONG x, LONG y, WORD pixelSize, Window*& window, const char* name = "Window", window_callback_function callback = default_window_callback, HWND parentWindow = NULL){
	//Erstelle Fenster Klasse
	if(window != nullptr) return CREATE_WINDOW;

	window = new Window;

	WNDCLASS window_class = {};
	window_class.hInstance = hInstance;
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	std::string className = "Window-Class" + std::to_string((unsigned long long)window);	//TODO meh...
	window_class.lpszClassName = className.c_str();
	window_class.lpfnWndProc = callback;

	window->windowClassName = className;
	//Registriere Fenster Klasse
	if(!RegisterClass(&window_class)){
		std::cerr << "Register-Class: " << GetLastError() << std::endl;
		return CREATE_WINDOW;
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
	window->handle = CreateWindow(window_class.lpszClassName, name, WS_VISIBLE | WS_OVERLAPPEDWINDOW, x, y, w, h, parentWindow, NULL, hInstance, NULL);
	if(window->handle == NULL){
		std::cerr << "Create-Window: "<< GetLastError() << std::endl;
		return CREATE_WINDOW;
	}

	//TODO idk ob das so ok ist, win32 doku sagt nicht viel darüber aus... aber angeblich ist USERDATA
	//genau für sowas gedacht und es sollte auch nie überschrieben werden...
	SetWindowLongPtr(window->handle, GWLP_USERDATA, (LONG_PTR)window);

	D2D1_RENDER_TARGET_PROPERTIES properties = {};
	properties.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
	properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	properties.dpiX = 96;
	properties.dpiY = 96;
	properties.usage = D2D1_RENDER_TARGET_USAGE_NONE;
	properties.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
	D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProperties = {};
	hwndProperties.hwnd = window->handle;
	hwndProperties.pixelSize = D2D1::SizeU(windowWidth, windowHeight);
	hwndProperties.presentOptions = D2D1_PRESENT_OPTIONS_IMMEDIATELY;
    HRESULT hr = app.factory->CreateHwndRenderTarget(properties, hwndProperties, &window->renderTarget);
    if(hr){
		std::cerr << "Create-Rendertarget: "<< hr << std::endl;
		return INIT_RENDER_TARGET;
    }

	WORD buffer_width = windowWidth/pixelSize;
	WORD buffer_height = windowHeight/pixelSize;
	window->pixelSize = pixelSize;
	window->pixels = new DWORD[buffer_width*buffer_height];
	window->depth = new DWORD[buffer_width*buffer_height];
	window->windowWidth = windowWidth;
	window->windowHeight = windowHeight;
	return SUCCESS;
}

//Zerstört das Fenster und alle allokierten Ressourcen mit diesem
ErrCode destroyWindow(Window*& window){
	#ifdef INVALIDHANDLEERRORS
	if(window == nullptr) return WINDOW_NOT_FOUND;
	#endif
	for(BYTE i=0; i < window->attributeBuffersCount; ++i) delete[] window->attributeBuffers[i];
	if(!UnregisterClassA(window->windowClassName.c_str(), NULL)){
		std::cerr << GetLastError() << std::endl;
		return GENERIC_ERROR;
	}
	DestroyWindow(window->handle);
	delete[] window->pixels;
	delete[] window->depth;
	delete window;
	window = nullptr;
	return SUCCESS;
}

inline ErrCode setWindowFlag(Window* window, WINDOWFLAG state){
	#ifdef INVALIDHANDLEERRORS
	if(window == nullptr) return WINDOW_NOT_FOUND;
	#endif
	window->flags = (WINDOWFLAG)(window->flags | state);
	return SUCCESS;
}
inline ErrCode resetWindowFlag(Window* window, WINDOWFLAG state){
	#ifdef INVALIDHANDLEERRORS
	if(window == nullptr) return WINDOW_NOT_FOUND;
	#endif
	window->flags = (WINDOWFLAG)(window->flags & ~state);
	return SUCCESS;
}
inline bool getWindowFlag(Window* window, WINDOWFLAG state){
	#ifdef INVALIDHANDLEERRORS
	if(window == nullptr) return false;
	#endif
	return (window->flags & state);
}

//TODO Sollte ERRCODE zurückgeben und WINDOWFLAG als Referenzparameter übergeben bekommen
//Gibt den nächsten Zustand des Fensters zurück und löscht diesen anschließend, Anwendung z.B. while(state = getNextWindowState() != WINDOW_NONE)...
inline WINDOWFLAG getNextWindowState(Window* window){
	#ifdef INVALIDHANDLEERRORS
	if(window == nullptr) return WINDOW_NONE;
	#endif
	WINDOWFLAG flag = (WINDOWFLAG)(window->flags & -window->flags);
	window->flags = (WINDOWFLAG)(window->flags & ~flag);
	return flag;
}

inline ErrCode resizeWindow(Window* window, WORD width, WORD height, WORD pixel_size){
	#ifdef INVALIDHANDLEERRORS
	if(window == nullptr) return WINDOW_NOT_FOUND;
	#endif
	window->windowWidth = width;
	window->windowHeight = height;
	window->pixelSize = pixel_size;
	delete[] window->pixels;
	delete[] window->depth;
	WORD bufferWidth = width/pixel_size;
	WORD bufferHeight = height/pixel_size;
	window->pixels = new(std::nothrow) DWORD[bufferWidth*bufferHeight];
	if(!window->pixels) return BAD_ALLOC;
	window->depth = new(std::nothrow) DWORD[bufferWidth*bufferHeight];
	if(!window->depth) return BAD_ALLOC;
	window->renderTarget->Resize({width, height});
	for(BYTE i=0; i < window->attributeBuffersCount; ++i){
		delete[] window->attributeBuffers[i];
		window->attributeBuffers[i] = new(std::nothrow) float[bufferWidth*bufferHeight];
		if(!window->attributeBuffers[i]) return BAD_ALLOC;
	}
	return SUCCESS;
}

//Weißt dem Fenster bufferCount viele Vertex-Attribute-Buffer zu und löscht bestehende
//TODO 255 sind zu viele, sollte Fehler werfen, falls man mehr wie so 32 erstellen will
ErrCode assignAttributeBuffers(Window* window, BYTE bufferCount){
	#ifdef INVALIDHANDLEERRORS
	if(window == nullptr) return WINDOW_NOT_FOUND;
	#endif
	if(bufferCount > 32) return BAD_ALLOC;	//TODO neuer Fehlercode
	for(BYTE i=0; i < window->attributeBuffersCount; ++i) delete[] window->attributeBuffers[i];
	DWORD bufferWidth = window->windowWidth/window->pixelSize;
	DWORD bufferHeight = window->windowHeight/window->pixelSize;
	for(BYTE i=0; i < bufferCount; ++i){
		window->attributeBuffers[i] = new(std::nothrow) float[bufferWidth*bufferHeight];
		if(!window->attributeBuffers[i]) return BAD_ALLOC;
	}
	window->attributeBuffersCount = bufferCount;
	return SUCCESS;
}

//TODO anstatt solch eine komplexe Funktion in createWindow rein zu geben, könnte man seine eigene schreiben mit Window* und uMsg,... als Parameter
//und diese default funktion ruft diese dann optional nur auf
LRESULT CALLBACK default_window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if(window == nullptr) return DefWindowProc(hwnd, uMsg, wParam, lParam);	//TODO das ist ein Fehler, wie melden aber?
	switch(uMsg){
		case WM_DESTROY:{
			ErrCheck(setWindowFlag(window, WINDOW_CLOSE), "setze close Fensterstatus");
			break;
		}
		case WM_SIZE:{
			UINT width = LOWORD(lParam);
			UINT height = HIWORD(lParam);
			if(!width || !height) break;
			ErrCheck(setWindowFlag(window, WINDOW_RESIZE), "setzte resize Fensterstatus");
			ErrCheck(resizeWindow(window, width, height, 1), "Fenster skalieren");
			break;
		}
		case WM_LBUTTONDOWN:{
			if(!getButton(mouse, MOUSE_LMB)){

			};
			setButton(mouse, MOUSE_LMB);
			break;
		}
		case WM_LBUTTONUP:{
			resetButton(mouse, MOUSE_LMB);
			break;
		}
		case WM_RBUTTONDOWN:{
			if(!getButton(mouse, MOUSE_RMB)){

			};
			setButton(mouse, MOUSE_RMB);
			break;
		}
		case WM_RBUTTONUP:{
			resetButton(mouse, MOUSE_RMB);
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

inline void getMessages(Window* window)noexcept{
	MSG msg;
	while(PeekMessage(&msg, window->handle, 0, 0, PM_REMOVE)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

inline constexpr DWORD RGBA(BYTE r, BYTE g, BYTE b, BYTE a=255){return DWORD(b|g<<8|r<<16|a<<24);}
inline constexpr BYTE A(DWORD color){return BYTE(color>>24);}
inline constexpr BYTE R(DWORD color){return BYTE(color>>16);}
inline constexpr BYTE G(DWORD color){return BYTE(color>>8);}
inline constexpr BYTE B(DWORD color){return BYTE(color);}

inline ErrCode clearWindow(Window* window)noexcept{
	#ifdef INVALIDHANDLEERRORS
	if(window == nullptr) return WINDOW_NOT_FOUND;
	#endif
	WORD buffer_width = window->windowWidth/window->pixelSize;
	WORD buffer_height = window->windowHeight/window->pixelSize;
	for(WORD y=0; y < buffer_height; ++y){
		for(WORD x=0; x < buffer_width; ++x){
			window->pixels[y*buffer_width+x] = RGBA(0, 0, 0);
			window->depth[y*buffer_width+x] = 0xFFFFFFFF;
		}
	}
	return SUCCESS;
}

//TODO bitmap kann man bestimmt auch im Window speichern und auf dieser dann rummalen. anstatt diese immer neu zu erzeugen
//Muss dann halt auch immer geupdated werden, wenn das Window skaliert wird,...
inline ErrCode drawWindow(Window* window)noexcept{
	#ifdef INVALIDHANDLEERRORS
	if(window == nullptr) return WINDOW_NOT_FOUND;
	#endif
	if(window->windowWidth == 0 || window->windowHeight == 0) return GENERIC_ERROR;
	WORD buffer_width = window->windowWidth/window->pixelSize;
	WORD buffer_height = window->windowHeight/window->pixelSize;
	ID2D1Bitmap* bitmap;
	D2D1_BITMAP_PROPERTIES properties = {};
	properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	properties.dpiX = 96;
	properties.dpiY = 96;
	window->renderTarget->BeginDraw();
	HRESULT hr = window->renderTarget->CreateBitmap({buffer_width, buffer_height}, window->pixels, buffer_width*4, properties, &bitmap);
	if(hr){
		std::cerr << hr << std::endl;
		exit(-2);
	}
	window->renderTarget->DrawBitmap(bitmap, D2D1::RectF(0, 0, window->windowWidth, window->windowHeight), 1, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, D2D1::RectF(0, 0, buffer_width, buffer_height));
	bitmap->Release();
	window->renderTarget->EndDraw();
	return SUCCESS;
}

inline ErrCode drawRectangle(Window* window, DWORD x, DWORD y, DWORD dx, DWORD dy, DWORD color)noexcept{
	#ifdef INVALIDHANDLEERRORS
	if(window == nullptr) return WINDOW_NOT_FOUND;
	#endif
	WORD buffer_width = window->windowWidth/window->pixelSize;
	for(WORD i=y; i < y+dy; ++i){
		for(WORD j=x; j < x+dx; ++j){
			window->pixels[i*buffer_width+j] = color;
		}
	}
	return SUCCESS;
}

inline ErrCode drawLine(Window* window, WORD start_x, WORD start_y, WORD end_x, WORD end_y, DWORD color)noexcept{
	#ifdef INVALIDHANDLEERRORS
	if(window == nullptr) return WINDOW_NOT_FOUND;
	#endif
	WORD bufferWidth = window->windowWidth/window->pixelSize;
	int dx = end_x-start_x;
	int dy = end_y-start_y;
	int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
	float xinc = dx/(float)steps;
	float yinc = dy/(float)steps;
	float x = start_x+0.5f;
	float y = start_y+0.5f;
	for(int i = 0; i <= steps; ++i){
		DWORD idx = (int)y*bufferWidth+(int)x;
		if(idx >= 0 && idx < bufferWidth*window->windowHeight/window->pixelSize) window->pixels[idx] = color;	//TODO remove
		// window->pixels[(int)y*bufferWidth+(int)x] = color;
		x += xinc;
		y += yinc;
	}
	return SUCCESS;
}

struct Image{
	DWORD* data = nullptr;
	WORD width = 0;		//x-Dimension
	WORD height = 0;	//y-Dimension
};

Image _default_texture;

ErrCode loadImage(const char* name, Image& image){
	std::fstream file; file.open(name, std::ios::in);
	if(!file.is_open()) return FILE_NOT_FOUND;
	//Lese Breite und Höhe
	std::string word;
	file >> word;
	image.width = std::atoi(word.c_str());
	file >> word;
	image.height = std::atoi(word.c_str());
	int pos = file.tellg();
	image.data = new(std::nothrow) DWORD[image.width*image.height];
	if(!image.data) return BAD_ALLOC;
	file.close();
	file.open(name, std::ios::in | std::ios::binary);
	if(!file.is_open()) return FILE_NOT_FOUND;
	file.seekg(pos);
	char val[4];
	file.read(&val[0], 1);	//überspringe letztes Leerzeichen
	for(DWORD i=0; i < image.width*image.height; ++i){
		file.read(&val[0], 1);
		file.read(&val[1], 1);
		file.read(&val[2], 1);
		file.read(&val[3], 1);
		image.data[i] = RGBA(val[0], val[1], val[2], val[3]);
	}
	file.close();
	return SUCCESS;
}

void destroyImage(Image& image){
	delete[] image.data;
	image.data = nullptr;
}

//x und y von 0 - 1, keine Tests auf Überlauf!
inline DWORD getImage(Image& image, float x, float y){
	DWORD ry = y*image.height;
	DWORD rx = x*(image.width-1);
	return image.data[ry*image.width+rx];
}

//Kopiert das gesamte Image in den angegebenen Bereich von start_x bis end_x und start_y bis end_y
//TODO Kopiere nicht das gesamte Image, sondern auch das sollte man angeben können
//TODO up-/downscaling methoden wie nearest, bilinear,...
ErrCode copyImageToWindow(Window* window, Image& image, WORD start_x, WORD start_y, WORD end_x, WORD end_y){
	#ifdef INVALIDHANDLEERRORS
	if(window == nullptr) return WINDOW_NOT_FOUND;
	#endif
	WORD buffer_width = window->windowWidth/window->pixelSize;
	for(int y=start_y; y < end_y; ++y){
		WORD ry = (float)(y-start_y)/(end_y-start_y)*image.height;
		for(int x=start_x; x < end_x; ++x){
			WORD rx = (float)(x-start_x)/(end_x-start_x)*(image.width-1);
			DWORD color = image.data[ry*image.width+rx];
			if(A(color) > 0) window->pixels[y*buffer_width+x] = color;
		}
	}
	return SUCCESS;
}

//Funktion testet ob jeder pixel im gültigen Fensterbereich liegt! idx ist der window index
//TODO ist das wirklich nötig eine ganze extra Funktion dafür zu machen?
ErrCode copyImageToWindowSave(Window* window, Image& image, WORD start_x, WORD start_y, WORD end_x, WORD end_y){
	#ifdef INVALIDHANDLEERRORS
	if(window == nullptr) return WINDOW_NOT_FOUND;
	#endif
	WORD buffer_width = window->windowWidth/window->pixelSize;
	WORD buffer_height = window->windowHeight/window->pixelSize;
	for(int y=start_y; y < end_y; ++y){
		if(y < 0 || y >= (int)buffer_height) continue;
		WORD ry = (float)(y-start_y)/(end_y-start_y)*image.height;
		for(int x=start_x; x < end_x; ++x){
			if(x < 0 || x >= (int)buffer_width) continue;
			WORD rx = (float)(x-start_x)/(end_x-start_x)*(image.width-1);
			DWORD color = image.data[ry*image.width+rx];
			if(A(color) > 0) window->pixels[y*buffer_width+x] = color;
		}
	}
	return SUCCESS;
}

struct Font{
	Image image;
	ivec2 char_size;		//Größe eines Symbols im Image
	WORD font_size = 12;	//Größe der Symbole in Pixel
	BYTE char_sizes[96];	//Größe der Symbole in x-Richtung
};

//Allokiert eine Font auf dem Heap
ErrCode createFont(Font*& font){
	font = new(std::nothrow) Font;
	if(!font) return BAD_ALLOC;
	return SUCCESS;
}

//Zerstört eine im Heap allokierte Font und alle weiteren allokierten Elemente
void destroyFont(Font*& font){
	destroyImage(font->image);
	delete font;
	font = nullptr;
}

ErrCode loadFont(const char* path, Font& font, ivec2 char_size){
	ErrCode code;
	font.char_size = char_size;
	if((code = ErrCheck(loadImage(path, font.image), "font image laden")) != SUCCESS){
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
	return SUCCESS;
}

//Gibts zurück wie viele Pixel der Text unter der gegebenen Font benötigt
//TODO font könnte nullptr sein -> INVALIDHANDLEERRORS?
WORD getStringFontSize(Font& font, std::string& text){
	float div = (float)font.char_size.y/font.font_size;
	WORD offset = 0;
	for(size_t i=0; i < text.size(); ++i){
		BYTE idx = (text[i]-32);
		offset += font.char_sizes[idx]/div;
	}
	return offset;
}

//Gibt zurück wie breit das Symbol war das gezeichnet wurde
//TODO Errors? übergebe symbol größe als Referenz Parameter
DWORD drawFontChar(Window* window, Font& font, char symbol, DWORD start_x, DWORD start_y){
	#ifdef INVALIDHANDLEERRORS
	if(window == nullptr) return 0;
	#endif
	DWORD idx = (symbol-32);
	float div = (float)font.char_size.y/font.font_size;
	DWORD end_x = start_x+font.char_sizes[idx]/div;
	DWORD end_y = start_y+font.font_size;
	DWORD x_offset = (idx%16)*font.char_size.x;
	DWORD y_offset = (idx/16)*font.char_size.y;
	DWORD buffer_width = window->windowWidth/window->pixelSize;
	for(DWORD y=start_y; y < end_y; ++y){
		float scaled_y = (float)(y-start_y)/(end_y-start_y);
		for(DWORD x=start_x; x < end_x; ++x){
			DWORD ry = scaled_y*font.char_size.y;
			DWORD rx = (float)(x-start_x)/(end_x-start_x)*(font.char_sizes[idx]-1);
			DWORD color = font.image.data[(ry+y_offset)*font.image.width+rx+x_offset];
			if(A(color) > 0) window->pixels[y*buffer_width+x] = color;
		}
	}
	return end_x-start_x;
}

//Gibt zurück, wie breit der String zu zeichnen war, String muss \0 terminiert sein!
//TODO Errors? meh, if window wird evtl bei jedem Aufruf von drawFontChar gemacht
DWORD drawFontString(Window* window, Font& font, const char* string, DWORD start_x, DWORD start_y){
	DWORD offset = 0;
	DWORD idx = 0;
	while(string[idx] != '\0'){
		offset += drawFontChar(window, font, string[idx++], start_x+offset, start_y);
	}
	return offset;
}

ErrCode _defaultEvent(BYTE*){return SUCCESS;}
enum BUTTONFLAGS{
	BUTTON_VISIBLE=1,
	BUTTON_CAN_HOVER=2,
	BUTTON_HOVER=4,
	BUTTON_PRESSED=8,
	BUTTON_TEXT_CENTER=16,
	BUTTON_DISABLED=32
};
struct Button{
	ErrCode (*event)(BYTE*) = _defaultEvent;	//Funktionspointer zu einer Funktion die gecallt werden soll wenn der Button gedrückt wird
	std::string text;
	Image* image = nullptr;
	Image* disabled_image = nullptr;
	ivec2 pos = {0, 0};
	ivec2 repos = {0, 0};
	ivec2 size = {50, 10};
	ivec2 resize = {55, 11};
	BYTE flags = BUTTON_VISIBLE | BUTTON_CAN_HOVER | BUTTON_TEXT_CENTER;
	DWORD color = RGBA(120, 120, 120);
	DWORD hover_color = RGBA(120, 120, 255);
	DWORD textcolor = RGBA(180, 180, 180);
	DWORD disabled_color = RGBA(90, 90, 90);
	WORD textsize = 16;
	BYTE* data = nullptr;
};

void destroyButton(Button& button){
	destroyImage(*button.image);
	delete[] button.data;
}

inline constexpr void setButtonFlag(Button& button, BUTTONFLAGS flag){button.flags |= flag;}
inline constexpr void resetButtonFlag(Button& button, BUTTONFLAGS flag){button.flags &= ~flag;}
inline constexpr bool getButtonFlag(Button& button, BUTTONFLAGS flag){return (button.flags & flag);}
//TODO kann bestimmt besser geschrieben werden... und ErrCheck aufs Event sollte mit einem BUTTONSTATE entschieden werden
inline void buttonsClicked(Button* buttons, WORD button_count){
	for(WORD i=0; i < button_count; ++i){
		Button& b = buttons[i];
		if(!getButtonFlag(b, BUTTON_VISIBLE) || getButtonFlag(b, BUTTON_DISABLED)) continue;
		ivec2 delta = {mouse.pos.x - b.pos.x, mouse.pos.y - b.pos.y};
		if(delta.x >= 0 && delta.x <= b.size.x && delta.y >= 0 && delta.y <= b.size.y){
			if(getButtonFlag(b, BUTTON_CAN_HOVER)) b.flags |= BUTTON_HOVER;
			if(getButton(mouse, MOUSE_LMB) && !getButtonFlag(b, BUTTON_PRESSED)){
				ErrCheck(b.event(b.data));
				b.flags |= BUTTON_PRESSED;
			}
			else if(!getButton(mouse, MOUSE_LMB)) b.flags &= ~BUTTON_PRESSED;
		}else if(getButtonFlag(b, BUTTON_CAN_HOVER)){
			b.flags &= ~BUTTON_HOVER;
		}
	}
}

//TODO meh, if window wird in jedem draw gemacht
inline void drawButtons(Window* window, Font& font, Button* buttons, WORD button_count){
	for(WORD i=0; i < button_count; ++i){
		Button& b = buttons[i];
		if(!getButtonFlag(b, BUTTON_VISIBLE)) continue;
		if(getButtonFlag(b, BUTTON_DISABLED)){
			if(b.disabled_image == nullptr)
				drawRectangle(window, b.pos.x, b.pos.y, b.size.x, b.size.y, b.disabled_color);
			else
				copyImageToWindow(window, *b.disabled_image, b.pos.x, b.pos.y, b.pos.x+b.size.x, b.pos.y+b.size.y);
		}else if(b.image == nullptr){
			if(getButtonFlag(b, BUTTON_CAN_HOVER) && getButtonFlag(b, BUTTON_HOVER))
				drawRectangle(window, b.pos.x, b.pos.y, b.size.x, b.size.y, b.hover_color);
			else
				drawRectangle(window, b.pos.x, b.pos.y, b.size.x, b.size.y, b.color);
		}else{
			if(getButtonFlag(b, BUTTON_CAN_HOVER) && getButtonFlag(b, BUTTON_HOVER))
				copyImageToWindow(window, *b.image, b.repos.x, b.repos.y, b.repos.x+b.resize.x, b.repos.y+b.resize.y);
			else
				copyImageToWindow(window, *b.image, b.pos.x, b.pos.y, b.pos.x+b.size.x, b.pos.y+b.size.y);
		}
		if(getButtonFlag(b, BUTTON_TEXT_CENTER)){
			DWORD offset = 0;
			WORD tmp_font_size = font.font_size;
			font.font_size = b.textsize;
			WORD str_size = getStringFontSize(font, b.text);
			for(size_t i=0; i < b.text.size(); ++i){
				offset += drawFontChar(window, font, b.text[i], b.pos.x+offset+b.size.x/2-str_size/2, b.pos.y+b.size.y/2-b.textsize/2);
			}
			font.font_size = tmp_font_size;
		}else{
			DWORD offset = 0;
			WORD tmp_font_size = font.font_size;
			font.font_size = b.textsize;
			for(size_t i=0; i < b.text.size(); ++i){
				offset += drawFontChar(window, font, b.text[i], b.pos.x+offset, b.pos.y+b.size.y/2-b.textsize/2);
			}
			font.font_size = tmp_font_size;
		}
	}
}

inline void updateButtons(Window* window, Font& font, Button* buttons, WORD button_count){
	buttonsClicked(buttons, button_count);
	drawButtons(window, font, buttons, button_count);
}

struct Label{
	std::string text;
	ivec2 pos = {0, 0};
	DWORD textcolor = RGBA(180, 180, 180);
	WORD text_size = 2;
};

enum MENUFLAGS{
	MENU_OPEN=1,
	MENU_OPEN_TOGGLE=2
};
#define MAX_BUTTONS 10
#define MAX_STRINGS 20
#define MAX_IMAGES 5
struct Menu{
	Image* images[MAX_IMAGES];	//Sind für die Buttons
	BYTE image_count = 0;
	Button buttons[MAX_BUTTONS];
	BYTE button_count = 0;
	BYTE flags = MENU_OPEN;		//Bits: offen, toggle bit für offen, Rest ungenutzt
	ivec2 pos = {};				//TODO Position in Bildschirmpixelkoordinaten
	Label labels[MAX_STRINGS];
	BYTE label_count = 0;
};

void destroyMenu(Menu& menu){
	for(WORD i=0; i < menu.image_count; ++i){
		destroyImage(*menu.images[i]);
	}
}

inline constexpr void setMenuFlag(Menu& menu, MENUFLAGS flag){menu.flags |= flag;}
inline constexpr void resetMenuFlag(Menu& menu, MENUFLAGS flag){menu.flags &= ~flag;}
inline constexpr bool getMenuFlag(Menu& menu, MENUFLAGS flag){return (menu.flags&flag);}

inline void updateMenu(Window* window, Menu& menu, Font& font){
	if(getMenuFlag(menu, MENU_OPEN)){
		updateButtons(window, font, menu.buttons, menu.button_count);
		for(WORD i=0; i < menu.label_count; ++i){
			Label& label = menu.labels[i];
			DWORD offset = 0;
			for(size_t j=0; j < label.text.size(); ++j){
				WORD tmp = font.font_size;
				font.font_size = label.text_size;
				offset += drawFontChar(window, font, label.text[j], label.pos.x+offset, label.pos.y);
				font.font_size = tmp;
			}
		}
	}
}

//------------------------------ Für 3D und "erweiterte" Grafiken ------------------------------

#define CULLBACKFACES
#define DEPTH_DIVISOR 10000.f

struct triangle{
	fvec3 points[3];
};

struct TriangleAttribute{
	fvec4 attributes[3];		//1 fec4 Attribut pro Punkt
};

//Anzahl der Attribute sind dann die Anzahl der Dreiecke
struct VertexAttributesInfo{
	DWORD index;											//Wird vom rasterizer verwendet, nicht verändern
	BYTE attributesCount = 0;								//Anzahl der Attribute
	TriangleAttribute* attributes[MAXVERTEXATTRIBUTES];		//Die Attribute als fvec4 arrays in 3er Vektoren aufgeteilt, da es ja 3 Punkte gibt
};

//Einfacher Texture lookup; u, v von 0 - 1
//TODO aktuell extra Funktion das nicht das modulo wrapping macht
inline DWORD texture2D(Image& image, float u, float v)noexcept{
	int u1 = u*(image.width-1);
	int v1 = v*(image.height-1);
	int idx = u1*image.width+v1;
	return image.data[idx%(image.width*image.height)];
}

//Zeichnet nur die Umrandung eines Dreiecks
inline void drawTriangleOutline(Window* window, triangle& tri)noexcept{
	DWORD buffer_width = window->windowWidth/window->pixelSize;
	DWORD buffer_height = window->windowHeight/window->pixelSize;
	fvec3 pt0 = tri.points[0]; fvec3 pt1 = tri.points[1]; fvec3 pt2 = tri.points[2];
	pt0.x = ((pt0.x*0.5f)+0.5f)*buffer_width; pt1.x = ((pt1.x*0.5f)+0.5)*buffer_width; pt2.x = ((pt2.x*0.5f)+0.5)*buffer_width;
	pt0.y = ((pt0.y*0.5f)+0.5)*buffer_height; pt1.y = ((pt1.y*0.5f)+0.5)*buffer_height; pt2.y = ((pt2.y*0.5f)+0.5)*buffer_height;
	drawLine(window, pt0.x, pt0.y, pt1.x, pt1.y, RGBA(255, 255, 255, 255));
	drawLine(window, pt0.x, pt0.y, pt2.x, pt2.y, RGBA(255, 255, 255, 255));
	drawLine(window, pt1.x, pt1.y, pt2.x, pt2.y, RGBA(255, 255, 255, 255));
}

//TODO man kann den Anfang der "scanline" berechnen anstatt einer bounding box
//TODO es sollten vertex attribute übergeben werden, die dann alle interpoliert werden, so machen es auch moderne grafikkarten
inline void drawTriangleFilledOld(Window* window, triangle& tri, fvec3& normal, VertexAttributesInfo& attributesInfo)noexcept{
	DWORD buffer_width = window->windowWidth/window->pixelSize;
	DWORD buffer_height = window->windowHeight/window->pixelSize;
	fvec3 pt0 = tri.points[0]; fvec3 pt1 = tri.points[1]; fvec3 pt2 = tri.points[2];
	pt0.x = ((pt0.x*0.5f)+0.5f)*buffer_width; pt1.x = ((pt1.x*0.5f)+0.5f)*buffer_width; pt2.x = ((pt2.x*0.5f)+0.5f)*buffer_width;
	pt0.y = ((pt0.y*0.5f)+0.5f)*buffer_height; pt1.y = ((pt1.y*0.5f)+0.5f)*buffer_height; pt2.y = ((pt2.y*0.5f)+0.5f)*buffer_height;

	DWORD ymin = min(pt0.y, min(pt1.y, pt2.y));
	DWORD ymax = max(pt0.y, max(pt1.y, pt2.y));
	DWORD xmin = min(pt0.x, min(pt1.x, pt2.x));
	DWORD xmax = max(pt0.x, max(pt1.x, pt2.x));

	fvec2 vs1 = {pt1.x - pt0.x, pt1.y - pt0.y};
	fvec2 vs2 = {pt2.x - pt0.x, pt2.y - pt0.y};
	float div = cross(vs1, vs2);

	//Berechne uv/z vor
	// float uv0x = tri.uv[0].x/tri.points[0].z; float uv1x = tri.uv[1].x/tri.points[1].z; float uv2x = tri.uv[2].x/tri.points[2].z;
	// float uv0y = tri.uv[0].y/tri.points[0].z; float uv1y = tri.uv[1].y/tri.points[1].z; float uv2y = tri.uv[2].y/tri.points[2].z;
	float uv0x = attributesInfo.attributes[0][attributesInfo.index].attributes[0].x/tri.points[0].z;
	float uv0y = attributesInfo.attributes[0][attributesInfo.index].attributes[0].y/tri.points[0].z;
	float uv1x = attributesInfo.attributes[0][attributesInfo.index].attributes[1].x/tri.points[1].z;
	float uv1y = attributesInfo.attributes[0][attributesInfo.index].attributes[1].y/tri.points[1].z;
	float uv2x = attributesInfo.attributes[0][attributesInfo.index].attributes[2].x/tri.points[2].z;
	float uv2y = attributesInfo.attributes[0][attributesInfo.index].attributes[2].y/tri.points[2].z;

	//Normalenvektor
	// DWORD normalColor = RGBA(127.5f*(1+normal.x), 127.5f*(1+normal.y), 127.5f*(1+normal.z));

	//Berechne u und v initial und inkrementiere dann nur noch entsprechend
	fvec2 q = {xmin - pt0.x, ymin - pt0.y};
	float u = cross(q, vs2)/div; float v = cross(vs1, q)/div;
	float deltaX_u = (pt2.y - pt0.y)/div; float deltaX_v = (pt1.y - pt0.y)/div;
	float deltaY_u = (pt2.x - pt0.x)/div; float deltaY_v = (pt1.x - pt0.x)/div;
	//Berechne 1/pt<n>.z vor
	float pt0_z_inv = 1.f/pt0.z; float pt1_z_inv = 1.f/pt1.z; float pt2_z_inv = 1.f/pt2.z;
	for(DWORD y = ymin; y <= ymax; ++y){
		float tmp_u = u; float tmp_v = v;
		bool wasIn = false;
		for(DWORD x = xmin; x <= xmax; ++x){
			//w -> pt0, u -> pt1, v -> pt2
			if((v >= 0)&&(u >= 0)&&(u + v <= 1)){
				wasIn = true;
				float w = 1.f-u-v;
				DWORD idx = y*buffer_width+x;
				float depth = 1.f/(w*pt0_z_inv + u*pt1_z_inv + v*pt2_z_inv);	//TODO Iterativ lösbar?
				//TODO depth buffer endlich eine Range geben damit eine erwartete Genauigkeit erfasst werden kann
				float inc_depth = depth*DEPTH_DIVISOR;
				if(inc_depth <= window->depth[idx]){
					#ifdef PERFORMANCE_ANALYZER
					_perfAnalyzer.pixelsDrawn++;
					#endif
					window->depth[idx] = (DWORD)inc_depth;
					float s = (w*uv0x + u*uv1x + v*uv2x);
					float t = (w*uv0y + u*uv1y + v*uv2y);
					s *= depth; t *= depth;
					//TODO dynamisch texturen lesen können/materials hinzufügen
					window->pixels[idx] = texture2D(_default_texture, s, t);
					// _normal_buffer[idx] = normalColor;
					// window->pixels[idx] = RGBA(192, 192, 192); //TODO remove
				}
				#ifdef PERFORMANCE_ANALYZER
				else _perfAnalyzer.pixelsCulled++;
				#endif
			}
			else if(wasIn) break;
	        u += deltaX_u; v -= deltaX_v;
		}
		u = tmp_u; v = tmp_v;
		u -= deltaY_u; v += deltaY_v;
	}
}

struct plane{
	fvec3 pos;
	fvec3 normal;
};

//Berechnet Kollisionspunkt zwischen einem Plane und 2 punkten (die eine linie bilden)
//Gibt zurück, ob eine Kollision stattgefunden hat und den Kollisionspunkt cp, sowie t als skalares Parameter (zwischen 0 - 1, start*(1-t)+end*t)
inline bool rayPlaneIntersection(plane& p, fvec3& start, fvec3& end, float& t, fvec3& cp)noexcept{
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

inline __attribute__((always_inline)) void removeTriangleFromBuffer(triangle* buffer, byte& count, byte& temp_count, byte& i)noexcept{
	buffer[i] = buffer[temp_count-1];
	--i; --temp_count; --count;
	return;
}

inline void clipPlane(plane& p, triangle* buffer, byte& count, VertexAttributesInfo& attributesInfo)noexcept{
	byte tmp_off = count;		//Offset wo das aktuelle neue Dreieck hinzugefügt werden soll
	byte offset = count;		//Originaler Offset der neuen Dreiecke
	byte temp_count = count;	//Index des letzten originalen Dreiecks
	for(byte i=0; i < temp_count; ++i){
		fvec3 in_v[3]; fvec3 out_v[3];	//TODO Speicher die Indexe die Innerhalb/Ausserhalb liegen
		byte in = 0; byte out = 0;
		for(int j=0; j < 3; ++j){
			fvec3 vec = buffer[i].points[j]; vec.x -= p.pos.x; vec.y -= p.pos.y; vec.z -= p.pos.z;
			float dist = dot(vec, p.normal);
			if(dist < 0.){
				out_v[out] = buffer[i].points[j];
				++out;
			}else{
				in_v[in] = buffer[i].points[j];
				++in;
			}
		}
		switch(in){
			case 0:{	//Dreieck liegt komplett ausserhalb, es muss entfernt werden
				removeTriangleFromBuffer(buffer, count, temp_count, i);
				break;
			}
			case 1:{	//Das aktuelle Dreieck kann einfach geändert werden
				float t;
				rayPlaneIntersection(p, in_v[0], out_v[0], t, buffer[i].points[1]);
				// buffer[i].uv[1] = out_uv[0];
				rayPlaneIntersection(p, in_v[0], out_v[1], t, buffer[i].points[2]);
				// buffer[i].uv[2] = out_uv[1];
				buffer[i].points[0] = in_v[0];
				// buffer[i].uv[0] = in_uv[0];
				break;
			}
			case 2:{	//2 neue Dreiecke müssen hinzugefügt werden und das aktuelle entfernt
				removeTriangleFromBuffer(buffer, count, temp_count, i);
				fvec3 dir = in_v[0];
				dir.x -= out_v[0].x; dir.y -= out_v[0].y; dir.z -= out_v[0].z;
				// fvec2 end_uv_tmp = out_uv[0];
				float t;
				rayPlaneIntersection(p, in_v[0], out_v[0], t, buffer[tmp_off].points[2]);
				// buffer[tmp_off].uv[2] = out_uv[0];
				buffer[tmp_off].points[0] = in_v[0];
				// buffer[tmp_off].uv[0] = in_uv[0];
				buffer[tmp_off].points[1] = in_v[1];
				// buffer[tmp_off].uv[1] = in_uv[1];
				dir = in_v[1];
				dir.x -= out_v[0].x; dir.y -= out_v[0].y; dir.z -= out_v[0].z;
				// out_uv[0] = end_uv_tmp;
				rayPlaneIntersection(p, in_v[1], out_v[0], t, buffer[tmp_off+1].points[2]);
				// buffer[tmp_off+1].uv[2] = out_uv[0];
				buffer[tmp_off+1].points[0] = buffer[tmp_off].points[2];
				// buffer[tmp_off+1].uv[0] = buffer[tmp_off].uv[2];
				buffer[tmp_off+1].points[1] = in_v[1];
				// buffer[tmp_off+1].uv[1] = in_uv[1];
				tmp_off += 2;
				count += 2;
				break;
			}
		}
	}
	for(int i=0; i < count-temp_count; ++i){
		buffer[temp_count+i] = buffer[offset+i];
	}
	return;
}

#define XMIN -1.01f
#define XMAX 1.01f
#define YMIN -1.01f
#define YMAX 1.01f

//TODO kann bestimmt um einiges optimiert werden
inline BYTE clipping(Window* window, triangle* buffer, VertexAttributesInfo& attributesInfo)noexcept{
	BYTE count = 1;
	float aspect_ratio = (float)window->windowHeight/window->windowWidth;

	plane pz = {}; pz.normal = {0, 0, 1}; pz.pos = {0, 0, 0};
	// normalize(pz.normal);
	clipPlane(pz, buffer, count, attributesInfo);

	plane px = {}; px.normal = {XMIN*aspect_ratio, 0, 1};
	normalize(px.normal);
	clipPlane(px, buffer, count, attributesInfo);

	plane pnx = {}; pnx.normal = {XMAX*aspect_ratio, 0, 1};
	normalize(pnx.normal);
	clipPlane(pnx, buffer, count, attributesInfo);

	plane py = {}; py.normal = {0, YMIN, 1};
	normalize(py.normal);
	clipPlane(py, buffer, count, attributesInfo);

	plane pny = {}; pny.normal = {0, YMAX, 1};
	normalize(pny.normal);
	clipPlane(pny, buffer, count, attributesInfo);

	return count;
}

struct camera{
	float focal_length;
	fvec3 pos;
	fvec2 rot;	//Yaw, pitch. rot.x ist die Rotation um die Y-Achse weil... uhh ja
};

inline void rasterize(Window* window, triangle* tris, DWORD startIdx, DWORD endIdx, VertexAttributesInfo& attributesInfo, camera* cam)noexcept{
#ifdef PERFORMANCE_ANALYZER
	_perfAnalyzer.totalTriangles += endIdx - startIdx;
#endif
	float rotm[3][3];
	float aspect_ratio = (float)window->windowWidth/window->windowHeight;
	float sin_rotx = sin(cam->rot.x);
	float cos_rotx = cos(cam->rot.x);
	float sin_roty = sin(cam->rot.y);
	float cos_roty = cos(cam->rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty;		rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty;	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;
	triangle buffer[32];	//Speichert Dreiecke die durch das clipping entstehen
    for(DWORD i=startIdx; i < endIdx; ++i){
    	triangle tri = tris[i];
    	//TODO kann man auch im Dreieck speichern
    	fvec3 l1 = {tri.points[1].x-tri.points[0].x, tri.points[1].y-tri.points[0].y, tri.points[1].z-tri.points[0].z};
    	fvec3 l2 = {tri.points[2].x-tri.points[0].x, tri.points[2].y-tri.points[0].y, tri.points[2].z-tri.points[0].z};
    	fvec3 world_normal = cross(l1, l2);
    	normalize(world_normal);
    	for(int j=0; j < 3; ++j){
    		float d[3];
    		d[0] = (tri.points[j].x-cam->pos.x);
    		d[1] = (tri.points[j].y-cam->pos.y);
    		d[2] = (tri.points[j].z-cam->pos.z);
    		float v[3]{0};
    	    for(WORD a=0; a < 3; ++a){
    	        for(WORD b=0; b < 3; ++b){
    	        	v[a] += (rotm[a][b]*d[b]);
    	        }
    	    }
    	    tri.points[j].x = v[0];
    	    tri.points[j].y = v[1];
    	    tri.points[j].z = v[2];
    	}
    	fvec3 l01 = {tri.points[1].x-tri.points[0].x, tri.points[1].y-tri.points[0].y, tri.points[1].z-tri.points[0].z};
    	fvec3 l02 = {tri.points[2].x-tri.points[0].x, tri.points[2].y-tri.points[0].y, tri.points[2].z-tri.points[0].z};
    	fvec3 normal = cross(l01, l02);
    	normalize(normal);
#ifdef CULLBACKFACES
    	if(dot(tri.points[0], normal) > 0) continue;
#endif
    	buffer[0] = tri;
    	BYTE count = clipping(window, buffer, attributesInfo);
		attributesInfo.index = i;		//TODO falsch und tatsächlich muss man ja auch den buffer mit den vertex attributen anpassen... ugh
    	for(byte j=0; j < count; ++j){
    		fvec3 pt1 = buffer[j].points[0]; fvec3 pt2 = buffer[j].points[1]; fvec3 pt3 = buffer[j].points[2];
    		buffer[j].points[0].x = pt1.x*(cam->focal_length/pt1.z)/aspect_ratio; buffer[j].points[0].y = pt1.y*(cam->focal_length/pt1.z);
    		buffer[j].points[1].x = pt2.x*(cam->focal_length/pt2.z)/aspect_ratio; buffer[j].points[1].y = pt2.y*(cam->focal_length/pt2.z);
    		buffer[j].points[2].x = pt3.x*(cam->focal_length/pt3.z)/aspect_ratio; buffer[j].points[2].y = pt3.y*(cam->focal_length/pt3.z);
    		buffer[j].points[0].z = pt1.z; buffer[j].points[1].z = pt2.z; buffer[j].points[2].z = pt3.z;
    		drawTriangleFilledOld(window, buffer[j], world_normal, attributesInfo);
			// drawTriangleOutline(window, buffer[j]);
#ifdef PERFORMANCE_ANALYZER
    		_perfAnalyzer.drawnTriangles += 1;
#endif
    	}
    }
    return;
}

//TODO sollte Kontainer übergeben bekommen, die das Objekt speichern und nicht ein ganzes Dreiecks array
//TODO man sollte übergeben können, ob uvs und normalen genutzt werden und auch in welche location die gespeichert werden
ErrCode readObj(const char* filename, triangle* storage, DWORD* count, VertexAttributesInfo& attributesInfo, float x, float y, float z, float scale=1){
	std::fstream file; file.open(filename, std::ios::in);
	if(!file.is_open()) throw std::runtime_error("Konnte Datei nicht öffnen!");
	std::string word;
	std::vector<fvec3> points;
	std::vector<fvec3> normals;
	std::vector<fvec2> uvs;
	DWORD current_count = *count;
	DWORD tri_count = 0;
	while(file >> word){	//Lese Datei Wort für Wort
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
			//Lese nur die Eckpunkte
			DWORD pt_order[4];
			DWORD uv_order[4];
			DWORD c = 0;
			DWORD c1 = 0;
			std::string val;

			//Lese Punkt/Texture Koordinate
			file >> word; c = 0;
			//Suche alle Zahlen die den Punkt beschreiben
			for(size_t i=0; i < word.size(); ++i){
				if(word[i] >= '0' && word[i] <= '9') ++c;
				else break;
			} val.clear();
			for(DWORD i=0; i < c; ++i){
				val += word[i];
			} pt_order[0] = std::atoi(val.c_str())-1;
			//Suche alle Zahlen die die Texturkoordinaten bestimmen
			c1 = c+1; c = 0;
			for(size_t i=c1; i < word.size(); ++i){
				if(word[i] >= '0' && word[i] <= '9') ++c;
				else break;
			} val.clear();
			for(DWORD i=c1; i < c+c1; ++i){
				val += word[i];
			} uv_order[0] = std::atoi(val.c_str())-1;

			//Lese Punkt/Texture Koordinate
			file >> word; c = 0;
			//Suche alle Zahlen die den Punkt beschreiben
			for(size_t i=0; i < word.size(); ++i){
				if(word[i] >= '0' && word[i] <= '9') ++c;
				else break;
			} val.clear();
			for(DWORD i=0; i < c; ++i){
				val += word[i];
			} pt_order[1] = std::atoi(val.c_str())-1;
			//Suche alle Zahlen die die Texturkoordinaten bestimmen
			c1 = c+1; c = 0;
			for(size_t i=c1; i < word.size(); ++i){
				if(word[i] >= '0' && word[i] <= '9') ++c;
				else break;
			} val.clear();
			for(DWORD i=c1; i < c+c1; ++i){
				val += word[i];
			} uv_order[1] = std::atoi(val.c_str())-1;

			//Lese Punkt/Texture Koordinate
			file >> word; c = 0;
			//Suche alle Zahlen die den Punkt beschreiben
			for(size_t i=0; i < word.size(); ++i){
				if(word[i] >= '0' && word[i] <= '9') ++c;
				else break;
			} val.clear();
			for(DWORD i=0; i < c; ++i){
				val += word[i];
			} pt_order[2] = std::atoi(val.c_str())-1;
			//Suche alle Zahlen die die Texturkoordinaten bestimmen
			c1 = c+1; c = 0;
			for(size_t i=c1; i < word.size(); ++i){
				if(word[i] >= '0' && word[i] <= '9') ++c;
				else break;
			} val.clear();
			for(DWORD i=c1; i < c+c1; ++i){
				val += word[i];
			} uv_order[2] = std::atoi(val.c_str())-1;

			storage[tri_count+current_count].points[0] = points[pt_order[0]];
			storage[tri_count+current_count].points[1] = points[pt_order[1]];
			storage[tri_count+current_count].points[2] = points[pt_order[2]];
			attributesInfo.attributes[0][tri_count+current_count].attributes[0].x = uvs[uv_order[0]].x;
			attributesInfo.attributes[0][tri_count+current_count].attributes[0].y = uvs[uv_order[0]].y;
			attributesInfo.attributes[0][tri_count+current_count].attributes[1].x = uvs[uv_order[1]].x;
			attributesInfo.attributes[0][tri_count+current_count].attributes[1].y = uvs[uv_order[1]].y;
			attributesInfo.attributes[0][tri_count+current_count].attributes[2].x = uvs[uv_order[2]].x;
			attributesInfo.attributes[0][tri_count+current_count].attributes[2].y = uvs[uv_order[2]].y;
			// float x = (normals[pt_order[0]].x + normals[pt_order[1]].x + normals[pt_order[2]].x)/3.;	//TODO
			// float y = (normals[pt_order[0]].y + normals[pt_order[1]].y + normals[pt_order[2]].y)/3.;
			// float z = (normals[pt_order[0]].z + normals[pt_order[1]].x + normals[pt_order[2]].z)/3.;
			// storage[tri_count+current_count].normal = {x, y, z};
			++tri_count;
		}
	}
	*count += tri_count;
	std::cout << "Punkte gelesen:         " << points.size() << std::endl;
	std::cout << "UV-Koordinaten gelesen: " << uvs.size() << std::endl;
	std::cout << "Dreiecke gelesen:       " << tri_count << std::endl;
	std::cout << "Dreiecke insgesamt:     " << *count << '\n' << std::endl;
	return SUCCESS;
}

inline void createCube(triangle* tri, DWORD& count, float x, float y, float z, float dx, float dy, float dz){
	tri[count].points[0].x = x+dx;
	tri[count].points[0].y = y+dy;
	tri[count].points[0].z = z;
	tri[count].points[1].x = x+dx;
	tri[count].points[1].y = y;
	tri[count].points[1].z = z;
	tri[count].points[2].x = x;
	tri[count].points[2].y = y+dy;
	tri[count].points[2].z = z;
	// tri[count].uv[0] = {1, 1};
	// tri[count].uv[1] = {0, 1};
	// tri[count].uv[2] = {1, 0};

	tri[count+1].points[2].x = x;
	tri[count+1].points[2].y = y;
	tri[count+1].points[2].z = z;
	tri[count+1].points[1].x = x+dx;
	tri[count+1].points[1].y = y;
	tri[count+1].points[1].z = z;
	tri[count+1].points[0].x = x;
	tri[count+1].points[0].y = y+dy;
	tri[count+1].points[0].z = z;
	// tri[count+1].uv[2] = {0, 0};
	// tri[count+1].uv[1] = {0, 1};
	// tri[count+1].uv[0] = {1, 0};

	tri[count+2].points[2].x = x;
	tri[count+2].points[2].y = y;
	tri[count+2].points[2].z = z;
	tri[count+2].points[1].x = x;
	tri[count+2].points[1].y = y+dy;
	tri[count+2].points[1].z = z;
	tri[count+2].points[0].x = x;
	tri[count+2].points[0].y = y;
	tri[count+2].points[0].z = z+dz;
	// tri[count+2].uv[2] = {0, 1};
	// tri[count+2].uv[1] = {1, 1};
	// tri[count+2].uv[0] = {0, 0};

	tri[count+3].points[2].x = x;
	tri[count+3].points[2].y = y+dy;
	tri[count+3].points[2].z = z+dz;
	tri[count+3].points[1].x = x;
	tri[count+3].points[1].y = y;
	tri[count+3].points[1].z = z+dz;
	tri[count+3].points[0].x = x;
	tri[count+3].points[0].y = y+dy;
	tri[count+3].points[0].z = z;
	// tri[count+3].uv[2] = {1, 0};
	// tri[count+3].uv[1] = {0, 0};
	// tri[count+3].uv[0] = {1, 1};

	tri[count+4].points[0].x = x+dx;
	tri[count+4].points[0].y = y;
	tri[count+4].points[0].z = z;
	tri[count+4].points[1].x = x+dx;
	tri[count+4].points[1].y = y+dy;
	tri[count+4].points[1].z = z;
	tri[count+4].points[2].x = x+dx;
	tri[count+4].points[2].y = y;
	tri[count+4].points[2].z = z+dz;
	// tri[count+4].uv[0] = {0, 1};
	// tri[count+4].uv[1] = {1, 1};
	// tri[count+4].uv[2] = {0, 0};

	tri[count+5].points[0].x = x+dx;
	tri[count+5].points[0].y = y+dy;
	tri[count+5].points[0].z = z+dz;
	tri[count+5].points[1].x = x+dx;
	tri[count+5].points[1].y = y;
	tri[count+5].points[1].z = z+dz;
	tri[count+5].points[2].x = x+dx;
	tri[count+5].points[2].y = y+dy;
	tri[count+5].points[2].z = z;
	// tri[count+5].uv[0] = {1, 0};
	// tri[count+5].uv[1] = {0, 0};
	// tri[count+5].uv[2] = {1, 1};

	tri[count+6].points[2].x = x;
	tri[count+6].points[2].y = y;
	tri[count+6].points[2].z = z+dz;
	tri[count+6].points[1].x = x;
	tri[count+6].points[1].y = y+dy;
	tri[count+6].points[1].z = z+dz;
	tri[count+6].points[0].x = x+dx;
	tri[count+6].points[0].y = y;
	tri[count+6].points[0].z = z+dz;
	// tri[count+6].uv[2] = {0, 0};
	// tri[count+6].uv[1] = {1, 0};
	// tri[count+6].uv[0] = {0, 1};

	tri[count+7].points[0].x = x+dx;
	tri[count+7].points[0].y = y+dy;
	tri[count+7].points[0].z = z+dz;
	tri[count+7].points[1].x = x;
	tri[count+7].points[1].y = y+dy;
	tri[count+7].points[1].z = z+dz;
	tri[count+7].points[2].x = x+dx;
	tri[count+7].points[2].y = y;
	tri[count+7].points[2].z = z+dz;
	// tri[count+7].uv[0] = {1, 1};
	// tri[count+7].uv[1] = {1, 0};
	// tri[count+7].uv[2] = {0, 1};

	tri[count+8].points[2].x = x;
	tri[count+8].points[2].y = y+dy;
	tri[count+8].points[2].z = z;
	tri[count+8].points[1].x = x+dx;
	tri[count+8].points[1].y = y+dy;
	tri[count+8].points[1].z = z;
	tri[count+8].points[0].x = x;
	tri[count+8].points[0].y = y+dy;
	tri[count+8].points[0].z = z+dz;
	// tri[count+8].uv[2] = {0, 0};
	// tri[count+8].uv[1] = {1, 0};
	// tri[count+8].uv[0] = {0, 1};

	tri[count+9].points[2].x = x+dx;
	tri[count+9].points[2].y = y+dy;
	tri[count+9].points[2].z = z+dz;
	tri[count+9].points[1].x = x;
	tri[count+9].points[1].y = y+dy;
	tri[count+9].points[1].z = z+dz;
	tri[count+9].points[0].x = x+dx;
	tri[count+9].points[0].y = y+dy;
	tri[count+9].points[0].z = z;
	// tri[count+9].uv[2] = {1, 1};
	// tri[count+9].uv[1] = {0, 1};
	// tri[count+9].uv[0] = {1, 0};

	tri[count+10].points[0].x = x;
	tri[count+10].points[0].y = y;
	tri[count+10].points[0].z = z;
	tri[count+10].points[1].x = x+dx;
	tri[count+10].points[1].y = y;
	tri[count+10].points[1].z = z;
	tri[count+10].points[2].x = x;
	tri[count+10].points[2].y = y;
	tri[count+10].points[2].z = z+dz;
	// tri[count+10].uv[0] = {0, 0};
	// tri[count+10].uv[1] = {1, 0};
	// tri[count+10].uv[2] = {0, 1};

	tri[count+11].points[0].x = x+dx;
	tri[count+11].points[0].y = y;
	tri[count+11].points[0].z = z+dz;
	tri[count+11].points[1].x = x;
	tri[count+11].points[1].y = y;
	tri[count+11].points[1].z = z+dz;
	tri[count+11].points[2].x = x+dx;
	tri[count+11].points[2].y = y;
	tri[count+11].points[2].z = z;
	// tri[count+11].uv[0] = {1, 1};
	// tri[count+11].uv[1] = {0, 1};
	// tri[count+11].uv[2] = {1, 0};
	count += 12;
	return;
}
