#include <iostream>
#include <chrono>
#include <math.h>
#include <thread>
#include <vector>
#include "window.h"
#include "font.h"
#include "gui.h"

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
*/

static bool _running = true;
static camera _cam = {1., {110, -25, -80}, {0, 0.25}};

LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void update(float dt);

Menu settingsMenu;
//Menü Funktionen
ErrCode setWireframeMode(void){
	_render_mode = WIREFRAME_MODE;
	return SUCCESS;
}
ErrCode setShadedMode(void){
	_render_mode = SHADED_MODE;
	return SUCCESS;
}
ErrCode setDepthMode(void){
	_render_mode = DEPTH_MODE;
	return SUCCESS;
}
ErrCode setNormalMode(void){
	_render_mode = NORMAL_MODE;
	return SUCCESS;
}

// #define THREADING
#define THREADCOUNT 8
#define SPEED 0.05

GLOBALVAR ErrCode ERR_CODE = SUCCESS;

//TODO Nutze mehrere threads falls THREADING
void glight(){
	fvec3 light_dir = {2, -2, 1};
	normalize(light_dir);
	uint buffer_width = _window_width/_pixel_size;
	uint buffer_height = _window_height/_pixel_size;
	for(uint y=0; y < buffer_height; ++y){
		for(uint x=0; x < buffer_width; ++x){

			uint color = _pixels[y*buffer_width+x];
			uint normal = _normal_buffer[y*buffer_width+x];
			fvec3 n;
			n.x = (R(normal)/127.5)-1;
			n.y = (G(normal)/127.5)-1;
			n.z = (B(normal)/127.5)-1;
			float occlusion = (dot(light_dir, n)+1)/2.f;
			_pixels[y*buffer_width+x] = RGBA(R(color)*occlusion, G(color)*occlusion, B(color)*occlusion);

		}
	}
}

INT WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow){
	HWND window = getWindow(hInstance, "Window", WindowProc);
	if(!window){
		std::cerr << "Konnte Fenster nicht erstellen!" << std::endl;
		return -1;
	}

	triangle* triangles = new(std::nothrow) triangle[1100000];
	if(!triangles){
		ErrCheck(BAD_ALLOC, "Konnte keinen Speicher für die statischen Dreiecke allokieren!");
		return -1;
	}
	uint triangle_count = 0;

	ERR_CODE = load_texture("textures/low_poly_winter.tex", _default_texture);
	if(ErrCheck(ERR_CODE, "Fehler beim laden der default Texture!")){
		return -1;
	}
	// _default_texture = new uint[17*17+2];
	// _default_texture[0] = 17; _default_texture[1] = 17;
	// for(int i=2; i < 17*17+2; ++i){
	// 	_default_texture[i] = (i%2)*0xFFFFFFFF;
	// }

	ERR_CODE = read_obj("objects/low_poly_winter.obj", triangles, &triangle_count, 50, 0, 0, 2);
	if(ErrCheck(ERR_CODE, "Fehler beim Laden des default Modells!")){
		return -1;
	}
	for(uint i=0; i < triangle_count; ++i){
		uint color = _getDebugColor(_debugColorIdx++);
		triangles[i].colors[0] = color;
		triangles[i].colors[1] = color;
		triangles[i].colors[2] = color;
	}

	// triangles[0].point[2] = {-10, 10, 0};
	// triangles[0].point[1] = {0, -10, 0};
	// triangles[0].point[0] = {10, 10, 0};
	// triangles[0].point[0] = {-10, -10, 0};
	// triangles[0].point[1] = {-10, 10, 0};
	// triangles[0].point[2] = {10, -10, 0};
	// triangles[0].colors[0] = RGBA(255, 0, 0);
	// triangles[0].colors[1] = RGBA(0, 255, 0);
	// triangles[0].colors[2] = RGBA(0, 0, 255);
	// triangles[0].uv[0] = {0, 0};
	// triangles[0].uv[1] = {0, 1};
	// triangles[0].uv[2] = {1, 0};
	// triangles[1].point[0] = {10, -10, 0};
	// triangles[1].point[1] = {-10, 10, 0};
	// triangles[1].point[2] = {10, 10, 0};
	// triangles[1].colors[2] = RGBA(255, 0, 0);
	// triangles[1].colors[1] = RGBA(0, 255, 0);
	// triangles[1].colors[0] = RGBA(0, 0, 255);
	// triangles[1].uv[0] = {1, 0};
	// triangles[1].uv[1] = {0, 1};
	// triangles[1].uv[2] = {1, 1};
	// triangle_count = 2;
	create_cube(triangles, triangle_count, 0, 0, 0, 10, 10, 10);

	SetCursorPos(_window_width/2, _window_height/2);

	//TODO dynamisch buttons hinzufügen und entfernen
	Button settingButtons[5];
	settingButtons[0].size = {105, 15};
	settingButtons[0].pos = {(int)_window_width/(int)_pixel_size-settingButtons[0].size.x-10, settingButtons[0].size.y+10};
	settingButtons[0].event = setWireframeMode; settingButtons[0].text = "WIREFRAME";
	settingButtons[1].size = {105, 15};
	settingButtons[1].pos = {(int)_window_width/(int)_pixel_size-settingButtons[0].size.x-10, (settingButtons[0].size.y+10)*2};
	settingButtons[1].event = setShadedMode; settingButtons[1].text = "SHADED";
	settingButtons[2].size = {105, 15};
	settingButtons[2].pos = {(int)_window_width/(int)_pixel_size-settingButtons[0].size.x-10, (settingButtons[0].size.y+10)*3};
	settingButtons[2].event = setDepthMode; settingButtons[2].text = "DEPTH";
	settingButtons[3].size = {105, 15};
	settingButtons[3].pos = {(int)_window_width/(int)_pixel_size-settingButtons[0].size.x-10, (settingButtons[0].size.y+10)*4};
	settingButtons[3].event = setNormalMode; settingButtons[3].text = "NORMALS";
	settingsMenu.buttons = settingButtons;
	settingsMenu.button_count = 4;

	while(_running){
		getMessages(window);
		reset(_perfAnalyzer);
		clear_window();

#ifdef PERFORMANCE_ANALYZER
	start_timer(_perfAnalyzer, 0);
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
		rasterize(triangles, 0, triangle_count, &_cam);
		glight();
#endif
#ifdef PERFORMANCE_ANALYZER
    record_data(_perfAnalyzer, 0);
#endif

    	draw_buffer();

    	update(get_avg_data(_perfAnalyzer, 0)+1);
		draw_int(5, 5, 8/_pixel_size, get_avg_data(_perfAnalyzer, 0), RGBA(130, 130, 130, 255));
		draw_int(5, 55/_pixel_size, 8/_pixel_size, get_avg_data(_perfAnalyzer, 1), RGBA(130, 130, 130, 255));
		draw_int(5, 105/_pixel_size, 8/_pixel_size, get_avg_data(_perfAnalyzer, 2), RGBA(130, 130, 130, 255));
		draw_int(5, 155/_pixel_size, 8/_pixel_size, _perfAnalyzer.total_triangles, RGBA(130, 130, 130, 255));
		draw_int(5, 205/_pixel_size, 8/_pixel_size, _perfAnalyzer.drawn_triangles, RGBA(130, 130, 130, 255));
		draw_int(5, 255/_pixel_size, 8/_pixel_size, _pixels_drawn, RGBA(130, 130, 130, 255));
		draw_int(5, 305/_pixel_size, 8/_pixel_size, _pixels_not_drawn, RGBA(130, 130, 130, 255));
		draw(window);
	}

	DestroyWindow(window);
	delete[] triangles;
	delete[] _pixels;
	delete[] _depth_buffer;
	delete[] _normal_buffer;
	delete[] _default_texture;

	return 0;
}

void update(float dt){
	float sin_rotx = sin(_cam.rot.x);
	float cos_rotx = cos(_cam.rot.x);
	if(!checkMenuState(settingsMenu, MENU_OPEN)){
		_cam.pos.x -= getgKey(KEY_W)*sin_rotx*SPEED*dt;
		_cam.pos.z += getgKey(KEY_W)*cos_rotx*SPEED*dt;
		_cam.pos.x += getgKey(KEY_S)*sin_rotx*SPEED*dt;
		_cam.pos.z -= getgKey(KEY_S)*cos_rotx*SPEED*dt;
		_cam.pos.x += getgKey(KEY_D)*cos_rotx*SPEED*dt;
		_cam.pos.z += getgKey(KEY_D)*sin_rotx*SPEED*dt;
		_cam.pos.x -= getgKey(KEY_A)*cos_rotx*SPEED*dt;
		_cam.pos.z -= getgKey(KEY_A)*sin_rotx*SPEED*dt;
		_cam.pos.y -= getgKey(KEY_SPACE)*SPEED*dt;
		_cam.pos.y += getgKey(KEY_SHIFT)*SPEED*dt;
	}else{
		updateMenu(settingsMenu, _mouse);
	}
}

LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch(uMsg){
    case WM_SIZE:{
    	_window_width = LOWORD(lParam);
    	_window_height = HIWORD(lParam);
    	uint buffer_width = _window_width/_pixel_size;
    	uint buffer_height = _window_height/_pixel_size;
    	if(buffer_width > 0 && buffer_height > 0){
			delete[] _pixels;
			delete[] _depth_buffer;
			delete[] _normal_buffer;
			_pixels = new(std::nothrow) uint[buffer_width*buffer_height];
			_depth_buffer = new(std::nothrow) uint[buffer_width*buffer_height];
			_normal_buffer = new(std::nothrow) uint[buffer_width*buffer_height];
			if(!_pixels || !_depth_buffer){
				std::cerr << "Konnte keinen Speicher f�r pixel oder depth buffer allokieren!" << std::endl;
				buffer_width = 0;
				buffer_height = 0;
			}
			_bitmapInfo.bmiHeader.biWidth = buffer_width;
			_bitmapInfo.bmiHeader.biHeight = buffer_height;
    	}
    	return 0L;
    }
    case WM_DESTROY:
    case WM_CLOSE:{
    	_running = false;
    	return 0L;
    }
	case WM_MOUSEMOVE:{
		tagPOINT m_pos;
		tagRECT w_pos;
		GetCursorPos(&m_pos);
		GetWindowRect(hwnd, &w_pos);
		POINT point = {0, 0};
		ClientToScreen(hwnd, &point);
//		_mouse.pos.x = GET_X_LPARAM(lParam);	//TODO das sollte alles neu geschrieben werden
//		_mouse.pos.y = GET_Y_LPARAM(lParam);
		_mouse.pos.x = (m_pos.x - w_pos.left)/_pixel_size;
		_mouse.pos.y = ((m_pos.y - w_pos.top)-(point.y-w_pos.top))/_pixel_size;
		if(!checkMenuState(settingsMenu, MENU_OPEN)){
			_cam.rot.x -= ((float)m_pos.x-(_window_width/2+w_pos.left)) * 0.001;
			_cam.rot.y += ((float)m_pos.y-(_window_height/2+w_pos.top)) * 0.001;
			SetCursorPos(_window_width/2+w_pos.left, _window_height/2+w_pos.top);
		}
		return 0L;
	}
	case WM_LBUTTONDOWN:{
		setgMouse(MOUSE_LMB);
		break;
	}
	case WM_LBUTTONUP:{
		resetgMouse(MOUSE_LMB);
		break;
	}
	case WM_RBUTTONDOWN:{
		setgMouse(MOUSE_RMB);
		break;
	}
	case WM_RBUTTONUP:{
		resetgMouse(MOUSE_RMB);
		break;
	}
	case WM_KEYDOWN:{
		switch(wParam){
		case 0x57:	//W
			setgKey(KEY_W);
			break;
		case 0x53:	//S
			setgKey(KEY_S);
			break;
		case 0x44:	//D
			setgKey(KEY_D);
			break;
		case 0x41:	//A
			setgKey(KEY_A);
			break;
		case VK_SPACE:
			setgKey(KEY_SPACE);
			break;
		case VK_SHIFT:
			setgKey(KEY_SHIFT);
			break;
		case VK_ESCAPE:
			setgKey(KEY_ESC);
			if(!checkMenuState(settingsMenu, MENU_OPEN_TOGGLE)){
				settingsMenu.state |= MENU_OPEN_TOGGLE;	//Setze toggle bit
				settingsMenu.state ^= MENU_OPEN;		//�nder offen bit
				tagRECT w_pos;
				GetWindowRect(hwnd, &w_pos);
				if(!checkMenuState(settingsMenu, MENU_OPEN)) SetCursorPos(_window_width/2+w_pos.left, _window_height/2+w_pos.top);
			}
			break;
		}
		return 0L;
	}
	case WM_KEYUP:{
		switch(wParam){
		case 0x57:	//W
			resetgKey(KEY_W);
			break;
		case 0x53:	//S
			resetgKey(KEY_S);
			break;
		case 0x44:	//D
			resetgKey(KEY_D);
			break;
		case 0x41:	//A
			resetgKey(KEY_A);
			break;
		case VK_SPACE:
			resetgKey(KEY_SPACE);
			break;
		case VK_SHIFT:
			resetgKey(KEY_SHIFT);
			break;
		case VK_ESCAPE:
			resetgKey(KEY_ESC);
			settingsMenu.state &= ~MENU_OPEN_TOGGLE;	//Setze toggle bit zur�ck
			break;
		}
		return 0L;
	}
    }
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
