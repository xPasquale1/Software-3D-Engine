#include <iostream>
#include <chrono>
#include <math.h>
#include <thread>
#include <vector>
#include "graphics/window.h"

/*	TODO programm crashed falls die clipping region gleich/größer wie der Bildschirm ist, wahrscheinlich schreibt
	der rasterizer ausserhalb des pixel arrays
	TODO aktuell gibt es kein far clipping plane, daher wird nur ein teil der depth buffer Auflösung genutzt
	vllt kann man kein clipping machen, aber eine max. weite und daher auch Auflösung festlegen (clippe einfach alle verticies die zu groß sind? aufwand größer?)
	TODO uhm ja das weirde ghosting und so
	TODO Shadow mapping oder ähnliches
	TODO Multithreading muss noch korrekt implementiert werden mit locks auf die buffers, "faire" aufteilung,... und die Threads vllt wieder verwenden lol
*/

#define PIXELSIZE 2

static bool _running = true;
static Camera cam = {1., {-293.917, -197.536, -18.5511}, {-1.493, 0.411999}};

LRESULT mainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void update(float dt)noexcept;

enum RENDERMODE{
	SHADED_MODE, WIREFRAME_MODE, DEPTH_MODE
};
static RENDERMODE renderMode;

Menu settingsMenu;
// Menü Funktionen
ErrCode setWireframeMode(void*)noexcept{
	renderMode = WIREFRAME_MODE;
	return SUCCESS;
}
ErrCode setShadedMode(void*)noexcept{
	renderMode = SHADED_MODE;
	return SUCCESS;
}
ErrCode setDepthMode(void*)noexcept{
	renderMode = DEPTH_MODE;
	return SUCCESS;
}

// #define THREADING
#define THREADCOUNT 8
#define SPEED 0.25

Window* window = nullptr;
Font* font = nullptr;

void textureShader(Window* window, Image& image)noexcept{
	DWORD bufferWidth = window->windowWidth/window->pixelSize;
	DWORD bufferHeight = window->windowHeight/window->pixelSize;
	fvec3 light_dir = {2, 2, 1};
	normalize(light_dir);
	for(DWORD i=0; i < bufferWidth*bufferHeight; ++i){
		if(window->fragmentFlag[i] == 0) continue;
		window->fragmentFlag[i] = 0;
		float uvx = window->attributeBuffers[0][i].x;
		float uvy = window->attributeBuffers[0][i].y;
		DWORD color = texture2D(image, uvx, uvy);
		if(A(color) == 0) continue;
		fvec3 n;
		n.x = window->attributeBuffers[1][i].x;
		n.y = window->attributeBuffers[1][i].y;
		n.z = window->attributeBuffers[1][i].z;
		float occlusion = (dot(light_dir, n)+1)*0.5f;
		window->pixels[i] = RGBA(R(color)*occlusion, G(color)*occlusion, B(color)*occlusion);
	}
}

void depthBufferShader(Window* window)noexcept{
	DWORD bufferWidth = window->windowWidth/window->pixelSize;
	DWORD bufferHeight = window->windowHeight/window->pixelSize;
	for(DWORD i=0; i < bufferWidth*bufferHeight; ++i){
		QWORD depth = window->depth[i]*255;
		BYTE color = depth/0xFFFFFF;
		window->pixels[i] = RGBA(color, color, color);
	}
}

void drawTriangleModel(Window* window, TriangleModel& model, Image& defaultTexture)noexcept{
	rasterize(window, model.triangles, 0, model.triangleCount, cam, (TRIANGLERENDERMODE)renderMode);
	if(model.material == nullptr) textureShader(window, defaultTexture);
	else textureShader(window, model.material->textures[0]);
}

void drawDepthBuffer(Window* window, TriangleModel& model, Image& defaultTexture)noexcept{
	rasterize(window, model.triangles, 0, model.triangleCount, cam);
	depthBufferShader(window);
}

INT WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow){
	if(ErrCheck(initApp(), "App initialisieren") != SUCCESS) return -1;

	if(ErrCheck(createWindow(hInstance, 1000, 1000, 250, 0, PIXELSIZE, window, "3D!!!", mainWindowProc), "Fenster erstellen") != SUCCESS) return -1;
	if(ErrCheck(assignAttributeBuffers(window, 2), "AttributeBuffer hinzufügen") != SUCCESS) return -1;

	if(ErrCheck(createFont(font), "Font erstellen") != SUCCESS) return -1;
	if(ErrCheck(loadFont("fonts/asciiOutlined.tex", *font, {82, 83}), "Font laden") != SUCCESS) return -1;
	font->font_size = 42/window->pixelSize;

	//TODO dynamisch
	TriangleModel* models = new(std::nothrow) TriangleModel[50];
	DWORD modelCount = 0;
	Material* materials = new(std::nothrow) Material[50];
	DWORD materialCount = 0;
	if(!models || !materials){
		ErrCheck(BAD_ALLOC, "Konnte keinen Speicher für Modelle/Materials allokieren!");
		return -1;
	}

	Image defaultTexture;
	if(ErrCheck(loadImage("textures/basic.tex", defaultTexture), "Default Texture laden") != SUCCESS) return -1;

	if(ErrCheck(loadObj("objects/sponza.obj", models, modelCount, materials, materialCount, 2, 0, 0, 0, -4.5), "Modell laden") != SUCCESS) return -1;
	// if(ErrCheck(loadObj("objects/terrain1_optimized.obj", models, modelCount, materials, materialCount, 2, 0, 0, 0, 10), "Modell laden") != SUCCESS) return -1;

	for(DWORD i=0; i < materialCount; ++i){
		Image& image = materials[i].textures[0];
		for(DWORD j=0; j < image.width*image.height; ++j){
			if(A(image.data[j]) > 10) image.data[j] = RGBA(R(image.data[j]), G(image.data[j]), B(image.data[j]));
			else image.data[j] = RGBA(R(image.data[j]), G(image.data[j]), B(image.data[j]), 0);
		}
	}

	RECT rect;
	GetWindowRect(window->handle, &rect);
	SetCursorPos(window->windowWidth/2+rect.left, window->windowHeight/2+rect.top);

	ivec2 buttonPos = {20/window->pixelSize, 80/window->pixelSize};
	ivec2 buttonSize = {280/window->pixelSize, 40/window->pixelSize};

	settingsMenu.buttons[0].size = buttonSize;
	settingsMenu.buttons[0].pos = buttonPos;
	settingsMenu.buttons[0].event = setWireframeMode;
	settingsMenu.buttons[0].text = "WIREFRAME";
	settingsMenu.buttons[0].textsize = 36/window->pixelSize;

	buttonPos.y += buttonSize.y+10/window->pixelSize;
	settingsMenu.buttons[1].size = buttonSize;
	settingsMenu.buttons[1].pos = buttonPos;
	settingsMenu.buttons[1].event = setShadedMode;
	settingsMenu.buttons[1].text = "SHADED";
	settingsMenu.buttons[1].textsize = 36/window->pixelSize;

	buttonPos.y += buttonSize.y+10/window->pixelSize;
	settingsMenu.buttons[2].size = buttonSize;
	settingsMenu.buttons[2].pos = buttonPos;
	settingsMenu.buttons[2].event = setDepthMode;
	settingsMenu.buttons[2].text = "DEPTH";
	settingsMenu.buttons[2].textsize = 36/window->pixelSize;
	settingsMenu.buttonCount = 3;

	while(_running){
		getMessages(window);
		resetData(_perfAnalyzer);
		clearWindow(window);

#ifdef PERFORMANCE_ANALYZER
		resetTimer(_perfAnalyzer.timer[0]);
#endif
#ifdef THREADING
#else
		switch(renderMode){
			case WIREFRAME_MODE:
			case SHADED_MODE:{
				for(DWORD i=0; i < modelCount; ++i) drawTriangleModel(window, models[i], defaultTexture);
				break;
			}
			case DEPTH_MODE:{
				for(DWORD i=0; i < modelCount; ++i) drawDepthBuffer(window, models[i], defaultTexture);
				break;
			}
		}
#endif
#ifdef PERFORMANCE_ANALYZER
    	recordData(_perfAnalyzer, 0);
#endif
		
		std::string val = floatToString(getAvgData(_perfAnalyzer, 0), 2) + "ms";
		drawFontString(window, *font, val.c_str(), 5, 2);
		val = floatToString(getAvgData(_perfAnalyzer, 1), 2) + "ms";
		drawFontString(window, *font, val.c_str(), 5, font->font_size+4);
		val = longToString(_perfAnalyzer.totalTriangles);
		drawFontString(window, *font, val.c_str(), 5, font->font_size*2+6);
		val = longToString(_perfAnalyzer.drawnTriangles);
		drawFontString(window, *font, val.c_str(), 5, font->font_size*3+8);
		val = longToString(_perfAnalyzer.pixelsDrawn);
		drawFontString(window, *font, val.c_str(), 5, font->font_size*4+10);
		val = longToString(_perfAnalyzer.pixelsCulled);
		drawFontString(window, *font, val.c_str(), 5, font->font_size*5+12);

		update(getAvgData(_perfAnalyzer, 0)+1);

		drawWindow(window);
	}

	for(DWORD i=0; i < modelCount; ++i) destroyTriangleModel(models[i]);
	for(DWORD i=0; i < materialCount; ++i) destroyMaterial(materials[i]);
	destroyFont(font);
	destroyWindow(window);
	destroyApp();

	return 0;
}

void update(float dt)noexcept{
	float sin_rotx = sin(cam.rot.x);
	float cos_rotx = cos(cam.rot.x);
	if(!getMenuFlag(settingsMenu, MENU_OPEN)){
		cam.pos.x -= getButton(keyboard, KEY_W)*sin_rotx*SPEED*dt;
		cam.pos.z += getButton(keyboard, KEY_W)*cos_rotx*SPEED*dt;
		cam.pos.x += getButton(keyboard, KEY_S)*sin_rotx*SPEED*dt;
		cam.pos.z -= getButton(keyboard, KEY_S)*cos_rotx*SPEED*dt;
		cam.pos.x += getButton(keyboard, KEY_D)*cos_rotx*SPEED*dt;
		cam.pos.z += getButton(keyboard, KEY_D)*sin_rotx*SPEED*dt;
		cam.pos.x -= getButton(keyboard, KEY_A)*cos_rotx*SPEED*dt;
		cam.pos.z -= getButton(keyboard, KEY_A)*sin_rotx*SPEED*dt;
		cam.pos.y -= getButton(keyboard, KEY_SPACE)*SPEED*dt;
		cam.pos.y += getButton(keyboard, KEY_SHIFT)*SPEED*dt;
	}else{
		updateMenu(window, settingsMenu, *font);
	}
}

LRESULT CALLBACK mainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if(window == nullptr) return DefWindowProc(hwnd, uMsg, wParam, lParam);
	switch(uMsg){
		case WM_DESTROY:{
			_running = false;
			ErrCheck(setWindowFlag(window, WINDOW_CLOSE), "setze close Fensterstatus");
			break;
		}
		case WM_SIZE:{
			UINT width = LOWORD(lParam);
			UINT height = HIWORD(lParam);
			if(!width || !height) break;
			// ErrCheck(setWindowFlag(window, WINDOW_RESIZE), "setzte resize Fensterstatus");
			ErrCheck(resizeWindow(window, width, height, PIXELSIZE), "Fenster skalieren");
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
			tagPOINT m_pos;
			tagRECT w_pos;
			GetCursorPos(&m_pos);
			GetWindowRect(hwnd, &w_pos);
			POINT point = {0, 0};
			ClientToScreen(hwnd, &point);
			mouse.pos.x = (m_pos.x - w_pos.left)/window->pixelSize;
			mouse.pos.y = ((m_pos.y - w_pos.top)-(point.y-w_pos.top))/window->pixelSize;
				if(!getMenuFlag(settingsMenu, MENU_OPEN)){
					cam.rot.x -= ((float)m_pos.x-(window->windowWidth/2+w_pos.left)) * 0.001;
					cam.rot.y += ((float)m_pos.y-(window->windowHeight/2+w_pos.top)) * 0.001;
					SetCursorPos(window->windowWidth/2+w_pos.left, window->windowHeight/2+w_pos.top);
				}
			break;
		}
		case WM_KEYDOWN:{
			switch(wParam){
			case 0x57:	//W
				setButton(keyboard, KEY_W);
				break;
			case 0x53:	//S
				setButton(keyboard, KEY_S);
				break;
			case 0x44:	//D
				setButton(keyboard, KEY_D);
				break;
			case 0x41:	//A
				setButton(keyboard, KEY_A);
				break;
			case VK_SPACE:
				setButton(keyboard, KEY_SPACE);
				break;
			case VK_SHIFT:
				setButton(keyboard, KEY_SHIFT);
				break;
			case VK_ESCAPE:
				setButton(keyboard, KEY_ESC);
				if(!getMenuFlag(settingsMenu, MENU_OPEN_TOGGLE)){
					setMenuFlag(settingsMenu, MENU_OPEN_TOGGLE);
					settingsMenu.flags ^= MENU_OPEN;			//Änder offen bit
					tagRECT w_pos;
					GetWindowRect(hwnd, &w_pos);
					if(!getMenuFlag(settingsMenu, MENU_OPEN)) SetCursorPos(window->windowWidth/2+w_pos.left, window->windowHeight/2+w_pos.top);
				}
				break;
			}
			return 0L;
		}
		case WM_KEYUP:{
			switch(wParam){
			case 0x57:	//W
				resetButton(keyboard, KEY_W);
				break;
			case 0x53:	//S
				resetButton(keyboard, KEY_S);
				break;
			case 0x44:	//D
				resetButton(keyboard, KEY_D);
				break;
			case 0x41:	//A
				resetButton(keyboard, KEY_A);
				break;
			case VK_SPACE:
				resetButton(keyboard, KEY_SPACE);
				break;
			case VK_SHIFT:
				resetButton(keyboard, KEY_SHIFT);
				break;
			case VK_ESCAPE:
				resetButton(keyboard, KEY_ESC);
				resetMenuFlag(settingsMenu, MENU_OPEN_TOGGLE);
				break;
			}
			return 0L;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
