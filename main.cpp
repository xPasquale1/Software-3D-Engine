#include <iostream>
#include <chrono>
#include <math.h>
#include <thread>
#include <vector>
#include "graphics/window.h"

/*	TODO programm crashed falls die clipping region gleich/größer wie der Bildschirm ist, wahrscheinlich schreibt
	der rasterizer ausserhalb des pixel arrays
	TODO aktuell gibt es kein far clipping plane, daher wird nur ein teil der depth buffer Auflösung genutzt
	vllt kann man kein clipping machen, aber eine max. weite und daher auch Auflösung festlegen (clippe einfach alle fragmente die zu groß sind? aufwand größer?)
	TODO Bilder müssen um 90° nach rechts gedreht werden damit die uv Koordinaten stimmen...
	TODO Alle Dreiecke sollten in einem Kontainer-System gespeichert werden, es sollte schnell sein die Daten zu
	finden (hashing/array) es sollte aber auch schnell gehen diese wieder zu löschen (Datenpackete Objektweiße speichern,
	da Dreiecke eigentlich nie einzeln eingelesen werden)
	TODO Shadow mapping oder ähnliches
	TODO Multithreading muss noch korrekt implementiert werden mit locks auf die buffers, "faire" aufteilung,... und die Threads vllt wieder verwenden lol
*/

#define PIXELSIZE 2

static bool _running = true;
static camera _cam = {1., {0, -10, -30}, {0, 0}};

LRESULT mainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void update(float dt);

Menu settingsMenu;
//Menü Funktionen
// ErrCode setWireframeMode(void){
// 	_render_mode = WIREFRAME_MODE;
// 	return SUCCESS;
// }
// ErrCode setShadedMode(void){
// 	_render_mode = SHADED_MODE;
// 	return SUCCESS;
// }
// ErrCode setDepthMode(void){
// 	_render_mode = DEPTH_MODE;
// 	return SUCCESS;
// }
// ErrCode setNormalMode(void){
// 	_render_mode = NORMAL_MODE;
// 	return SUCCESS;
// }

// #define THREADING
#define THREADCOUNT 8
#define SPEED 0.25

Window* window = nullptr;
Font* font = nullptr;

//TODO mehrere Threads falls THREADING
inline void textureShader(Window* window, Image& image){
	DWORD bufferWidth = window->windowWidth/window->pixelSize;
	DWORD bufferHeight = window->windowHeight/window->pixelSize;
	fvec3 light_dir = {2, 2, 1};
	normalize(light_dir);
	for(DWORD i=0; i < bufferWidth*bufferHeight; ++i){
		if(window->fragmentFlag[i] == 0) continue;
		window->fragmentFlag[i] = 0;
		fvec3 n;
		n.x = window->attributeBuffers[1][i].x;
		n.y = window->attributeBuffers[1][i].y;
		n.z = window->attributeBuffers[1][i].z;
		float uvx = window->attributeBuffers[0][i].x;
		float uvy = window->attributeBuffers[0][i].y;
		float occlusion = (dot(light_dir, n)+1)*0.5f;
		DWORD color = texture2D(image, uvx, uvy);
		// window->pixels[i] = RGBA(R(color)*occlusion, G(color)*occlusion, B(color)*occlusion);
		window->pixels[i] = color;
	}
}

INT WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow){
	if(ErrCheck(initApp(), "App initialisieren") != SUCCESS) return -1;

	if(ErrCheck(createWindow(hInstance, 1000, 1000, 250, 0, PIXELSIZE, window, "3D!!!", mainWindowProc), "Fenster erstellen") != SUCCESS) return -1;
	if(ErrCheck(assignAttributeBuffers(window, 2), "AttributeBuffer hinzufügen") != SUCCESS) return -1;

	if(ErrCheck(createFont(font), "Font erstellen") != SUCCESS) return -1;
	if(ErrCheck(loadFont("fonts/asciiOutlined.tex", *font, {82, 83}), "Font laden") != SUCCESS) return -1;
	font->font_size = 42/window->pixelSize;

	//TODO beides sollte in ein Struct gepackt werden und nur zusammen allokiert/deallokiert werden
	// Triangle* triangles = new(std::nothrow) Triangle[300000];
	TriangleModel* models = new(std::nothrow) TriangleModel[500];
	DWORD modelCount = 0;
	Material* materials = new(std::nothrow) Material[50];
	DWORD materialCount = 0;
	if(!models || !materials){
		ErrCheck(BAD_ALLOC, "Konnte keinen Speicher für Modelle/Materials allokieren!");
		return -1;
	}
	// DWORD triangle_count = 0;

	Image texture;
	if(ErrCheck(loadImage("textures/basic.tex", texture), "Image laden") != SUCCESS) return -1;

	// if(ErrCheck(readObj("objects/sponza.obj", triangles, 2, &triangle_count, 0, 0, 0, -1.5), "Modell laden") != SUCCESS) return -1;
	if(ErrCheck(loadObj("objects/sponza_stripped.obj", models, modelCount, materials, materialCount, 2, 0, 0, 0, -1.5), "Modell laden") != SUCCESS) return -1;

	RECT rect;
	GetWindowRect(window->handle, &rect);
	SetCursorPos(window->windowWidth/2+rect.left, window->windowHeight/2+rect.top);

	//TODO dynamisch buttons hinzufügen und entfernen
	Button settingsButtons[5];
	// settingsButtons[0].size = {105, 15};
	// settingsButtons[0].pos = {(int)window->windowWidth/window->pixelSize-settingsButtons[0].size.x-10, settingsButtons[0].size.y+10};
	// settingsButtons[0].event = setWireframeMode; settingsButtons[0].text = "WIREFRAME";
	// settingsButtons[1].size = {105, 15};
	// settingsButtons[1].pos = {(int)window->windowWidth/window->pixelSize-settingsButtons[0].size.x-10, (settingsButtons[0].size.y+10)*2};
	// settingsButtons[1].event = setShadedMode; settingsButtons[1].text = "SHADED";
	// settingsButtons[2].size = {105, 15};
	// settingsButtons[2].pos = {(int)window->windowWidth/window->pixelSize-settingsButtons[0].size.x-10, (settingsButtons[0].size.y+10)*3};
	// settingsButtons[2].event = setDepthMode; settingsButtons[2].text = "DEPTH";
	// settingsButtons[3].size = {105, 15};
	// settingsButtons[3].pos = {(int)window->windowWidth/window->pixelSize-settingsButtons[0].size.x-10, (settingsButtons[0].size.y+10)*4};
	// settingsButtons[3].event = setNormalMode; settingsButtons[3].text = "NORMALS";
	// settingsMenu.buttons = settingsButtons;
	settingsMenu.button_count = 0;

	while(_running){
		getMessages(window);
		resetData(_perfAnalyzer);
		clearWindow(window);

#ifdef PERFORMANCE_ANALYZER
		resetTimer(_perfAnalyzer.timer[0]);
#endif
#ifdef THREADING
		DWORD t_count = triangle_count/THREADCOUNT;
	    std::vector<std::thread> threads;
	    for(int i=0; i < THREADCOUNT-1; ++i){
	        threads.push_back(std::thread(rasterize, window, triangles, t_count*i, t_count*(i+1), std::ref(_cam)));
	    }
	    threads.push_back(std::thread(rasterize, window, triangles, t_count*(THREADCOUNT-1), triangle_count, std::ref(_cam)));

	    for(auto& thread : threads){
	        thread.join();
	    }
#else
		for(DWORD i=0; i < modelCount; ++i){
			rasterize(window, models[i].triangles, 0, models[i].triangleCount, _cam);
			textureShader(window, models[i].material->textures[0]);
		}
#endif
#ifdef PERFORMANCE_ANALYZER
    	recordData(_perfAnalyzer, 0);
#endif

    	update(getAvgData(_perfAnalyzer, 0)+1);
		
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

		drawWindow(window);
	}

	// delete[] triangles;
	destroyFont(font);
	destroyWindow(window);
	destroyApp();

	return 0;
}

void update(float dt){
	float sin_rotx = sin(_cam.rot.x);
	float cos_rotx = cos(_cam.rot.x);
	if(!getMenuFlag(settingsMenu, MENU_OPEN)){
		_cam.pos.x -= getButton(keyboard, KEY_W)*sin_rotx*SPEED*dt;
		_cam.pos.z += getButton(keyboard, KEY_W)*cos_rotx*SPEED*dt;
		_cam.pos.x += getButton(keyboard, KEY_S)*sin_rotx*SPEED*dt;
		_cam.pos.z -= getButton(keyboard, KEY_S)*cos_rotx*SPEED*dt;
		_cam.pos.x += getButton(keyboard, KEY_D)*cos_rotx*SPEED*dt;
		_cam.pos.z += getButton(keyboard, KEY_D)*sin_rotx*SPEED*dt;
		_cam.pos.x -= getButton(keyboard, KEY_A)*cos_rotx*SPEED*dt;
		_cam.pos.z -= getButton(keyboard, KEY_A)*sin_rotx*SPEED*dt;
		_cam.pos.y -= getButton(keyboard, KEY_SPACE)*SPEED*dt;
		_cam.pos.y += getButton(keyboard, KEY_SHIFT)*SPEED*dt;
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
					_cam.rot.x -= ((float)m_pos.x-(window->windowWidth/2+w_pos.left)) * 0.001;
					_cam.rot.y += ((float)m_pos.y-(window->windowHeight/2+w_pos.top)) * 0.001;
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
