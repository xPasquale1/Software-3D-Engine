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
	TODO aktuell gibt es kein far clipping plane, daher wird nur ein teil der depth buffer auflösung genutzt
	vllt kann man kein clipping machen, aber eine max. weite und daher auch auflösung festlegen
	TODO Bilder müssen um 90° nach rechts gedreht werden damit die uv Koordinaten stimmen...
	TODO Alle Dreiecke sollten in einem Kontainer-System gespeichert werden, es sollte schnell sein die Daten zu
	finden (hashing/array) es sollte aber auch schnell gehen diese wieder zu löschen (Datenpackete Objektweiße speichern,
	da Dreiecke eigentlich nie einzeln eingelesen werden)
	TODO Erste Beleuchtungsmodelle überlegen und implementieren
*/

static bool _running = true;
static camera _cam = {1., {0, 0, -20}, {0, 0}};

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

//#define THREADING
#define THREADCOUNT 8
#define SPEED 0.05

int ERR_CODE = SUCCESS;

#define OFFSET 100
#define SAMPLES 2

inline fvec3 reflect(fvec3 i, fvec3 n){
	i.x -= 2 * dot(i, n) * n.x;
	i.y -= 2 * dot(i, n) * n.y;
	i.z -= 2 * dot(i, n) * n.z;
	return i;
}

void SSAO(){
	uint buffer_width = _window_width/_pixel_size;
	uint buffer_height = _window_height/_pixel_size;
	for(uint y=OFFSET; y < buffer_height-OFFSET; ++y){
		for(uint x=OFFSET; x < buffer_width-OFFSET; ++x){

			float depth = _depth_buffer[y*buffer_width+x];
			float count = 0;
			for(int i=0; i < SAMPLES; ++i){
                //Generiere Testpunkte basierend auf den aktuellen Normalenvektoren
                float u = R(_normal_buffer[i]/128.-1);
                float v = G(_normal_buffer[i]/128.-1);
                fvec2 sample_point = {u*(rand()%201-100)/100.f, v*(rand()%201-100)/100.f};
//                sample_point.x /= depth; sample_point.y /= depth;
                uint idx = (int)(sample_point.y+y)*buffer_width+(int)(sample_point.x+x);
                if(idx < 0 || idx >= buffer_width*buffer_height) continue;
                float cur_depth = _depth_buffer[idx];
                if(depth > cur_depth) ++count;
			}
			count /= SAMPLES;
			uint color = _pixels[y*buffer_width+x];
			_pixels[y*buffer_width+x] = RGBA((1.f-count)*R(color), (1.f-count)*G(color), (1.f-count)*B(color));
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
		std::cerr << "Konnte keinen Speicher für die statischen Dreiecke allokieren!" << std::endl;
		return -1;
	}
	uint triangle_count = 0;

	ERR_CODE = load_texture("textures/blank.tex", _default_texture);
	if(ERR_CODE != SUCCESS){
		std::cerr << "Konnte default texture nicht laden!" << std::endl;
		return -1;
	}

//	create_cube(triangles, triangle_count, -5, -5, 10, 10, 10, 10);
	ERR_CODE = read_obj("objects/SSAO_test.obj", triangles, &triangle_count, 0, 20, 0, 2);
		if(ERR_CODE != SUCCESS){
		std::cerr << "Konnte Modell nicht laden!" << std::endl;
		return -1;
	}

	SetCursorPos(_window_width/2, _window_height/2);

	//TODO dynamisch buttons hinzufügen und entfernen
	Button settingButtons[5];
	settingButtons[0].size = {105, 15};
	settingButtons[0].pos = {(int)_window_width/(int)_pixel_size-settingButtons[0].size.x-10, settingButtons[0].size.y+10};
	settingButtons[0].hover_color = RGBA(120, 120, 255, 255); settingButtons[0].state = BUTTON_VISIBLE | BUTTON_CAN_HOVER;
	settingButtons[0].event = setWireframeMode; settingButtons[0].text = "WIREFRAME";
	settingButtons[1].size = {105, 15};
	settingButtons[1].pos = {(int)_window_width/(int)_pixel_size-settingButtons[0].size.x-10, (settingButtons[0].size.y+10)*2};
	settingButtons[1].hover_color = RGBA(120, 120, 255, 255); settingButtons[1].state = BUTTON_VISIBLE | BUTTON_CAN_HOVER;
	settingButtons[1].event = setShadedMode; settingButtons[1].text = "SHADED";
	settingButtons[2].size = {105, 15};
	settingButtons[2].pos = {(int)_window_width/(int)_pixel_size-settingButtons[0].size.x-10, (settingButtons[0].size.y+10)*3};
	settingButtons[2].hover_color = RGBA(120, 120, 255, 255); settingButtons[2].state = BUTTON_VISIBLE | BUTTON_CAN_HOVER;
	settingButtons[2].event = setDepthMode; settingButtons[2].text = "DEPTH";
	settingButtons[3].size = {105, 15};
	settingButtons[3].pos = {(int)_window_width/(int)_pixel_size-settingButtons[0].size.x-10, (settingButtons[0].size.y+10)*4};
	settingButtons[3].hover_color = RGBA(120, 120, 255, 255); settingButtons[3].state = BUTTON_VISIBLE | BUTTON_CAN_HOVER;
	settingButtons[3].event = setNormalMode; settingButtons[3].text = "NORMALS";
	settingsMenu.buttons = settingButtons;
	settingsMenu.button_count = 4;

	while(_running){
		getMessages(window);
		_perfAnalyzer.reset();
		clear_window();

#ifdef PERFORMANCE_ANALYZER
	_perfAnalyzer.start_timer(0);
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
	    SSAO();
#else
		rasterize(triangles, 0, triangle_count, &_cam);
		SSAO();
#endif
#ifdef PERFORMANCE_ANALYZER
    _perfAnalyzer.record_data(0);
#endif

		uint buffer_width = _window_width/_pixel_size;
		uint buffer_height = _window_height/_pixel_size;
    	switch(_render_mode){
    	case NORMAL_MODE:{
    		for(uint i=0; i < buffer_width*buffer_height; ++i){
    			_pixels[i] = _normal_buffer[i];
    		}
    		break;
    	}
    	case DEPTH_MODE:{
    		for(uint i=0; i < buffer_width*buffer_height; ++i){
    			uint depth_color = _depth_buffer[i]/10000.;
    			_pixels[i] = RGBA(depth_color, depth_color, depth_color);
    		}
    		break;
    	}
    	default: break;
    	}

    	update(_perfAnalyzer.get_avg_data(0)+1);
		draw_int(5, 5, 8/_pixel_size, _perfAnalyzer.get_avg_data(0), RGBA(130, 130, 130, 255));
		draw_int(5, 55/_pixel_size, 8/_pixel_size, _perfAnalyzer.get_avg_data(1), RGBA(130, 130, 130, 255));
		draw_int(5, 105/_pixel_size, 8/_pixel_size, _perfAnalyzer.get_avg_data(2), RGBA(130, 130, 130, 255));
		draw_int(5, 155/_pixel_size, 8/_pixel_size, _perfAnalyzer.total_triangles, RGBA(130, 130, 130, 255));
		draw_int(5, 205/_pixel_size, 8/_pixel_size, _perfAnalyzer.drawn_triangles, RGBA(130, 130, 130, 255));
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
		_cam.pos.x -= W(_keyboard)*sin_rotx*SPEED*dt;
		_cam.pos.z += W(_keyboard)*cos_rotx*SPEED*dt;
		_cam.pos.x += S(_keyboard)*sin_rotx*SPEED*dt;
		_cam.pos.z -= S(_keyboard)*cos_rotx*SPEED*dt;
		_cam.pos.x += D(_keyboard)*cos_rotx*SPEED*dt;
		_cam.pos.z += D(_keyboard)*sin_rotx*SPEED*dt;
		_cam.pos.x -= A(_keyboard)*cos_rotx*SPEED*dt;
		_cam.pos.z -= A(_keyboard)*sin_rotx*SPEED*dt;
		_cam.pos.y -= SPACE(_keyboard)*SPEED*dt;
		_cam.pos.y += SHIFT(_keyboard)*SPEED*dt;
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
				std::cerr << "Konnte keinen Speicher für pixel oder depth buffer allokieren!" << std::endl;
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
		_mouse.pos.x = (m_pos.x - w_pos.left)/_pixel_size;
		_mouse.pos.y = ((m_pos.y - w_pos.top)-31)/_pixel_size;	//TODO Größe der Titlebar bestimmen
		if(!checkMenuState(settingsMenu, MENU_OPEN)){
			_cam.rot.x -= ((float)m_pos.x-(_window_width/2+w_pos.left)) * 0.001;
			_cam.rot.y += ((float)m_pos.y-(_window_height/2+w_pos.top)) * 0.001;
			SetCursorPos(_window_width/2+w_pos.left, _window_height/2+w_pos.top);
		}
		return 0L;
	}
	case WM_LBUTTONDOWN:{
		_mouse.button |= MOUSE_LMB;
		break;
	}
	case WM_LBUTTONUP:{
		_mouse.button &= ~MOUSE_LMB;
		break;
	}
	case WM_RBUTTONDOWN:{
		_mouse.button |= MOUSE_RMB;
		break;
	}
	case WM_RBUTTONUP:{
		_mouse.button &= ~MOUSE_RMB;
		break;
	}
	case WM_KEYDOWN:{
		switch(wParam){
		case 0x57:	//W
			_keyboard.button |= KEYBOARD_W;
			break;
		case 0x53:	//S
			_keyboard.button |= KEYBOARD_S;
			break;
		case 0x44:	//D
			_keyboard.button |= KEYBOARD_D;
			break;
		case 0x41:	//A
			_keyboard.button |= KEYBOARD_A;
			break;
		case VK_SPACE:
			_keyboard.button |= KEYBOARD_SPACE;
			break;
		case VK_SHIFT:
			_keyboard.button |= KEYBOARD_SHIFT;
			break;
		case VK_ESCAPE:
			_keyboard.button |= KEYBOARD_ESC;
			if(!checkMenuState(settingsMenu, MENU_OPEN_TOGGLE)){
				settingsMenu.state |= MENU_OPEN_TOGGLE;	//Setze toggle bit
				settingsMenu.state ^= MENU_OPEN;		//änder offen bit
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
			_keyboard.button &= ~KEYBOARD_W;
			break;
		case 0x53:	//S
			_keyboard.button &= ~KEYBOARD_S;
			break;
		case 0x44:	//D
			_keyboard.button &= ~KEYBOARD_D;
			break;
		case 0x41:	//A
			_keyboard.button &= ~KEYBOARD_A;
			break;
		case VK_SPACE:
			_keyboard.button &= ~KEYBOARD_SPACE;
			break;
		case VK_SHIFT:
			_keyboard.button &= ~KEYBOARD_SHIFT;
			break;
		case VK_ESCAPE:
			_keyboard.button &= ~KEYBOARD_ESC;
			settingsMenu.state &= ~MENU_OPEN_TOGGLE;	//Setze toggle bit zurück
			break;
		}
		return 0L;
	}
    }
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
