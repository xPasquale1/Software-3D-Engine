#include <iostream>
#include <chrono>
#include <math.h>
#include <thread>
#include <vector>
#include "window.h"
#include "font.h"

/*	TODO programm crashed falls die clipping region gleich/gr��er wie der Bildschirm ist, wahrscheinlich schreibt
	der rasterizer ausserhalb des pixel arrays
	TODO aktuell gibt es kein far clipping plane, daher wird nur ein teil der depth buffer aufl�sung genutzt
	vllt kann man kein clipping machen, aber eine max. weite und daher auch aufl�sung festlegen
	TODO Bilder m�ssen um 90� nach rechts gedreht werden damit die uv Koordinaten stimmen...
	TODO Alle Dreiecke sollten in einem Kontainer-System gespeichert werden, es sollte schnell sein die Daten zu
	finden (hashing/array) es sollte aber auch schnell gehen diese wieder zu l�schen (Datenpackete Objektwei�e speichern,
	da Dreiecke eigentlich nie einzeln eingelesen werden)
	TODO Erste Beleuchtungsmodelle �berlegen und implementieren
*/

static bool _running = true;
static camera _cam = {1., {0, 0, -20}, {0, 0}};

LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void update(float dt);

//#define THREADING
#define THREADCOUNT 8
#define SPEED 0.05

int ERR_CODE = SUCCESS;

INT WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow){
	HWND window = getWindow(hInstance, "Window", WindowProc);
	if(!window){
		std::cerr << "Konnte Fenster nicht erstellen!" << std::endl;
		return -1;
	}

	triangle* triangles = new(std::nothrow) triangle[80000];
	if(!triangles){
		std::cerr << "Konnte keinen Speicher f�r die statischen Dreiecke allokieren!" << std::endl;
		return -1;
	}
	uint triangle_count = 0;

	ERR_CODE = load_texture("textures/low_poly_winter.tex", _default_texture);
	if(ERR_CODE != SUCCESS){
		std::cerr << "Konnte default texture nicht laden!" << std::endl;
		return -1;
	}

//	create_cube(triangles, triangle_count, -5, -5, 10, 10, 10, 10);
	ERR_CODE = read_obj("objects/low_poly_winter1.obj", triangles, &triangle_count, 0, 20, 0);
		if(ERR_CODE != SUCCESS){
		std::cerr << "Konnte Modell nicht laden!" << std::endl;
		return -1;
	}

	SetCursorPos(500, 500);

	while(_running){
		getMessages(window);
		update(perfAnalyzer.get_avg_data(0)+1);
		perfAnalyzer.reset();
		clear_window();

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
#endif

		draw_int(5, 5, 8/_pixel_size, perfAnalyzer.get_avg_data(0), RGBA(130, 130, 130, 255));
		draw_int(5, 55/_pixel_size, 8/_pixel_size, perfAnalyzer.get_avg_data(1), RGBA(130, 130, 130, 255));
		draw_int(5, 105/_pixel_size, 8/_pixel_size, perfAnalyzer.get_avg_data(2), RGBA(130, 130, 130, 255));
		draw_int(5, 155/_pixel_size, 8/_pixel_size, perfAnalyzer.total_triangles, RGBA(130, 130, 130, 255));
		draw_int(5, 205/_pixel_size, 8/_pixel_size, perfAnalyzer.drawn_triangles, RGBA(130, 130, 130, 255));
		draw(window);
	}

	DestroyWindow(window);
	delete[] triangles;
	delete[] _pixels;
	delete[] _depth_buffer;
	delete[] _default_texture;
	return 0;
}

void update(float dt){
	float sin_rotx = sin(_cam.rot.x);
	float cos_rotx = cos(_cam.rot.x);
	_cam.pos.x -= W(keyboard)*sin_rotx*SPEED*dt;
	_cam.pos.z += W(keyboard)*cos_rotx*SPEED*dt;
	_cam.pos.x += S(keyboard)*sin_rotx*SPEED*dt;
	_cam.pos.z -= S(keyboard)*cos_rotx*SPEED*dt;
	_cam.pos.x += D(keyboard)*cos_rotx*SPEED*dt;
	_cam.pos.z += D(keyboard)*sin_rotx*SPEED*dt;
	_cam.pos.x -= A(keyboard)*cos_rotx*SPEED*dt;
	_cam.pos.z -= A(keyboard)*sin_rotx*SPEED*dt;
	_cam.pos.y -= SPACE(keyboard)*SPEED*dt;
	_cam.pos.y += SHIFT(keyboard)*SPEED*dt;
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
			_pixels = new(std::nothrow) uint[buffer_width*buffer_height];
			_depth_buffer = new(std::nothrow) uint[buffer_width*buffer_height];
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
		tagPOINT pos;
		GetCursorPos(&pos);
		mouse.pos.x = pos.x;
		mouse.pos.y = pos.y;
		_cam.rot.x -= ((float)pos.x-500) * 0.001;
		_cam.rot.y += ((float)pos.y-500) * 0.001;
		SetCursorPos(500, 500);
		return 0L;
	}
	case WM_LBUTTONDOWN:{
		mouse.button |= 0b1000'0000;
		break;
	}
	case WM_LBUTTONUP:{
		mouse.button &= 0b0111'1111;
		break;
	}
	case WM_RBUTTONDOWN:{
		mouse.button |= 0b0100'0000;
		break;
	}
	case WM_RBUTTONUP:{
		mouse.button &= 0b1011'1111;
		break;
	}
	case WM_KEYDOWN:{
		switch(wParam){
		case 0x57:	//W
			keyboard.button |= 0b1000'0000;
			break;
		case 0x53:	//S
			keyboard.button |= 0b0010'0000;
			break;
		case 0x44:	//D
			keyboard.button |= 0b0001'0000;
			break;
		case 0x41:	//A
			keyboard.button |= 0b0100'0000;
			break;
		case VK_SPACE:
			keyboard.button |= 0b0000'0100;
			break;
		case VK_SHIFT:
			keyboard.button |= 0b0000'1000;
			break;
		}
		return 0L;
	}
	case WM_KEYUP:{
		switch(wParam){
		case 0x57:	//W
			keyboard.button &= 0b0111'1111;
			break;
		case 0x53:	//S
			keyboard.button &= 0b1101'1111;
			break;
		case 0x44:	//D
			keyboard.button &= 0b1110'1111;
			break;
		case 0x41:	//A
			keyboard.button &= 0b1011'1111;
			break;
		case VK_SPACE:
			keyboard.button &= 0b1111'1011;
			break;
		case VK_SHIFT:
			keyboard.button &= 0b1111'0111;
			break;
		}
		return 0L;
	}
    }
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
