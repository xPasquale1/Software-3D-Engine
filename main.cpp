#include <iostream>
#include <chrono>
#include <math.h>
#include "window.h"
#include "font.h"

//TODO programm crashed falls die clipping region gleich/größer wie der Bildschirm ist, wahrscheinlich schreibt
//der rasterizer ausserhalb des pixel arrays
//TODO aktuell gibt es kein far clipping plane, daher wird nur ein teil der depth buffer auflösung genutzt
//vorteil es muss kein farplane clipping durchgeführt werden, aber sollte evtl. doch gemacht werden

static bool running = true;
static camera cam = {1., {}, {}};

LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define SPEED 0.1

void update();

INT WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow){
	HWND window = getWindow(hInstance, "Window", WindowProc);
	if(!window){
		std::cerr << "Konnte Fenster nicht erstellen!" << std::endl;
		exit(-2);
	}

	//TODO Ein Memory-Managment-System implementieren
	triangle* triangles = new triangle[18000];
	uint triangle_count = 0;

	uchar color_switcher = 0;

//	create_cube(triangles, triangle_count, -5, -5, 10, 10, 10, 10);
	read_obj("objects/terrain1.obj", triangles, &triangle_count, 0, 20, 0);
	for(uint i=0; i < triangle_count; ++i){
		triangles[i].color = RGBA(color_switcher++, 255-color_switcher, 255, 255);
	}

	AvgFrametime ft;

	while(running){

		auto t1 = std::chrono::system_clock::now();
		getMessages(window);
		update();
		clear_window();

		rasterize(triangles, triangle_count, cam);

		draw_int(5, 5, 4, ft.avg_ms, RGBA(255, 255, 255, 255));
		draw(window);
		auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - t1).count();
		ft.add_time(millis);
	}

	DestroyWindow(window);
	delete[] pixels;
	delete[] depth_buffer;
	delete[] triangles;
	return 0;
}

void update(){
	float sin_rotx = sin(cam.rot.x);
	float cos_rotx = cos(cam.rot.x);
	if(W(keyboard)){
		cam.pos.x -= sin_rotx*SPEED;
		cam.pos.z += cos_rotx*SPEED;
	}
	if(S(keyboard)){
		cam.pos.x += sin_rotx*SPEED;
		cam.pos.z -= cos_rotx*SPEED;
	}
	if(D(keyboard)){
		cam.pos.x += cos_rotx*SPEED;
		cam.pos.z += sin_rotx*SPEED;
	}
	if(A(keyboard)){
		cam.pos.x -= cos_rotx*SPEED;
		cam.pos.z -= sin_rotx*SPEED;
	}
	if(SPACE(keyboard)){
		cam.pos.y -= SPEED;
	}
	if(SHIFT(keyboard)){
		cam.pos.y += SPEED;
	}
}

LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch(uMsg){
    case WM_SIZE:{
    	window_width = LOWORD(lParam);
    	window_height = HIWORD(lParam);
    	uint buffer_width = window_width/pixel_size;
    	uint buffer_height = window_height/pixel_size;
    	if(buffer_width > 0 && buffer_height > 0){
			delete[] pixels;
			delete[] depth_buffer;
			pixels = new uint[buffer_width*buffer_height];
			depth_buffer = new uint[buffer_width*buffer_height];
			bitmapInfo.bmiHeader.biWidth = buffer_width;
			bitmapInfo.bmiHeader.biHeight = buffer_height;
    	}
    	return 0L;
    }
    case WM_DESTROY:
    case WM_CLOSE:{
    	running = false;
    	return 0L;
    }
	case WM_MOUSEMOVE:{
		tagPOINT pos;
		GetCursorPos(&pos);
		mouse.pos.x = pos.x;
		mouse.pos.y = pos.y;
		cam.rot.x -= ((float)pos.x-300) * 0.001;
		cam.rot.y += ((float)pos.y-300) * 0.001;
		SetCursorPos(300, 300);
		return 0L;
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
