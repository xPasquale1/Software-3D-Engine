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
	TODO Erste per pixel Beleuchtungsmodelle überlegen und implementieren
	TODO Multithreading muss noch korrekt implementiert werden mit locks auf die buffers, "faire" aufteilung,...
	TODO Weitere Buffer an das Fenster anfügen um zusätzliche Vertex Attribute per pixel interpoliert zu speichern, sollte also irgendwie mit Triangle.attributes
	zusammenhängen... Erst mal überlegen wie das am besten gehen würde tho
*/

static bool _running = true;
// static camera _cam = {1., {110, -25, -80}, {0, 0.25}};
static camera _cam = {1., {0, 0, -80}, {0, 0}};

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
#define SPEED 0.1

Window* window = nullptr;
Font* font = nullptr;

//TODO Nutze mehrere threads falls THREADING
void glight(){
	fvec3 light_dir = {2, -2, 1};
	normalize(light_dir);
	DWORD buffer_width = window->windowWidth/window->pixelSize;
	DWORD buffer_height = window->windowHeight/window->pixelSize;
	for(DWORD y=0; y < buffer_height; ++y){
		for(DWORD x=0; x < buffer_width; ++x){

			DWORD color = window->pixels[y*buffer_width+x];
			// DWORD normal = _normal_buffer[y*buffer_width+x];
			fvec3 n;
			// n.x = (R(normal)/127.5)-1;
			// n.y = (G(normal)/127.5)-1;
			// n.z = (B(normal)/127.5)-1;
			float occlusion = (dot(light_dir, n)+1)/2.f;
			window->pixels[y*buffer_width+x] = RGBA(R(color)*occlusion, G(color)*occlusion, B(color)*occlusion);

		}
	}
}

INT WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow){
	if(ErrCheck(initApp(), "App initialisieren") != SUCCESS) return -1;

	font = new Font;
	if(ErrCheck(loadFont("fonts/ascii.tex", *font, {82, 83}), "Font laden") != SUCCESS) return -1;
	font->font_size = 21;

	if(ErrCheck(createWindow(hInstance, 1000, 1000, 250, 0, 2, window, "3D!!!", mainWindowProc), "Fenster erstellen") != SUCCESS) return -1;

	triangle* triangles = new(std::nothrow) triangle[1100000];
	if(!triangles){
		ErrCheck(BAD_ALLOC, "Konnte keinen Speicher für die statischen Dreiecke allokieren!");
		return -1;
	}
	DWORD triangle_count = 0;

	Image image;
	if(ErrCheck(loadImage("textures/low_poly_winter.tex", image), "Image laden") != SUCCESS) return -1;
	// _default_texture = new uint[17*17+2];
	// _default_texture[0] = 17; _default_texture[1] = 17;
	// for(int i=2; i < 17*17+2; ++i){
	// 	_default_texture[i] = (i%2)*0xFFFFFFFF;
	// }

	if(ErrCheck(readObj("objects/low_poly_winter.obj", triangles, &triangle_count, 50, 0, 0, 2), "Modell laden") != SUCCESS) return -1;

	// triangles[0].points[2] = {-10, 10, 0};
	// triangles[0].points[1] = {0, -10, 0};
	// triangles[0].points[0] = {10, 10, 0};
	// triangles[0].points[0] = {-10, -10, 0};
	// triangles[0].points[1] = {-10, 10, 0};
	// triangles[0].points[2] = {10, -10, 0};
	// triangles[0].colors[0] = RGBA(255, 0, 0);
	// triangles[0].colors[1] = RGBA(0, 255, 0);
	// triangles[0].colors[2] = RGBA(0, 0, 255);
	// triangles[0].uv[0] = {0, 0};
	// triangles[0].uv[1] = {0, 1};
	// triangles[0].uv[2] = {1, 0};
	// triangles[1].points[0] = {10, -10, 0};
	// triangles[1].points[1] = {-10, 10, 0};
	// triangles[1].points[2] = {10, 10, 0};
	// triangles[1].colors[2] = RGBA(255, 0, 0);
	// triangles[1].colors[1] = RGBA(0, 255, 0);
	// triangles[1].colors[0] = RGBA(0, 0, 255);
	// triangles[1].uv[0] = {1, 0};
	// triangles[1].uv[1] = {0, 1};
	// triangles[1].uv[2] = {1, 1};
	// triangle_count = 2;

	triangle_count = 0;
	createCube(triangles, triangle_count, 0, 0, 0, 20, 20, 20);
	SetCursorPos(window->windowWidth/2, window->windowHeight/2);

	//TODO dynamisch buttons hinzufügen und entfernen
	Button settingButtons[5];
	// settingButtons[0].size = {105, 15};
	// settingButtons[0].pos = {(int)window->windowWidth/window->pixelSize-settingButtons[0].size.x-10, settingButtons[0].size.y+10};
	// settingButtons[0].event = setWireframeMode; settingButtons[0].text = "WIREFRAME";
	// settingButtons[1].size = {105, 15};
	// settingButtons[1].pos = {(int)window->windowWidth/window->pixelSize-settingButtons[0].size.x-10, (settingButtons[0].size.y+10)*2};
	// settingButtons[1].event = setShadedMode; settingButtons[1].text = "SHADED";
	// settingButtons[2].size = {105, 15};
	// settingButtons[2].pos = {(int)window->windowWidth/window->pixelSize-settingButtons[0].size.x-10, (settingButtons[0].size.y+10)*3};
	// settingButtons[2].event = setDepthMode; settingButtons[2].text = "DEPTH";
	// settingButtons[3].size = {105, 15};
	// settingButtons[3].pos = {(int)window->windowWidth/window->pixelSize-settingButtons[0].size.x-10, (settingButtons[0].size.y+10)*4};
	// settingButtons[3].event = setNormalMode; settingButtons[3].text = "NORMALS";
	// settingsMenu.buttons = settingButtons;
	settingsMenu.button_count = 0;

	while(_running){
		getMessages(window);
		reset(_perfAnalyzer);
		clearWindow(window);

#ifdef PERFORMANCE_ANALYZER
		startTimer(_perfAnalyzer, 0);
#endif
#ifdef THREADING
		uint t_count = triangle_count/THREADCOUNT;
	    std::vector<std::thread> threads;
	    for(int i=0; i < THREADCOUNT-1; ++i){
	        threads.push_back(std::thread(rasterize, triangles, t_count*i, t_count*(i+1), &_cam));
	    }
	    threads.push_back(std::thread(rasterize, triangles, t_count*(THREADCOUNT-1), triangle_count, &_cam));

	    for(auto& thread : threads){
	        thread.join();
	    }
#else
		rasterize(window, triangles, 0, triangle_count, &_cam);
		// glight();
#endif
#ifdef PERFORMANCE_ANALYZER
    	recordData(_perfAnalyzer, 0);
#endif

    	update(getAvgData(_perfAnalyzer, 0)+1);
		std::string val = floatToString(getAvgData(_perfAnalyzer, 0), 4);
		drawFontString(window, *font, val.c_str(), 5, 5);
		val = floatToString(getAvgData(_perfAnalyzer, 1), 4);
		drawFontString(window, *font, val.c_str(), 5, font->font_size+10);
		val = floatToString(getAvgData(_perfAnalyzer, 2), 4);
		drawFontString(window, *font, val.c_str(), 5, font->font_size*2+15);
		val = longToString(_perfAnalyzer.totalTriangles);
		drawFontString(window, *font, val.c_str(), 5, font->font_size*3+20);
		val = longToString(_perfAnalyzer.drawnTriangles);
		drawFontString(window, *font, val.c_str(), 5, font->font_size*4+25);
		val = longToString(_perfAnalyzer.pixelsDrawn);
		drawFontString(window, *font, val.c_str(), 5, font->font_size*5+30);
		val = longToString(_perfAnalyzer.pixelsCulled);
		drawFontString(window, *font, val.c_str(), 5, font->font_size*6+35);
		drawWindow(window);
	}

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
