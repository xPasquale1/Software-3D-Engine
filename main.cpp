#include <iostream>
#include <chrono>
#include <math.h>
#include "window.h"
#include "font.h"

//TODO programm crashed falls die clipping region gleich/größer wie der Bildschirm ist, wahrscheinlich schreibt
//der rasterizer ausserhalb des pixel arrays
//TODO aktuell gibt es kein far clipping plane, daher wird nur ein teil der depth buffer auflösung genutzt
//vllt kann man kein clipping machen, aber eine max. weite und daher auch auflösung festlegen

static bool running = true;
static camera cam = {1., {0, -30, 0}, {0, deg2rad(90)}};

LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define SPEED 0.05

void update(float dt);

INT WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow){
	HWND window = getWindow(hInstance, "Window", WindowProc);
	if(!window){
		std::cerr << "Konnte Fenster nicht erstellen!" << std::endl;
		return -1;
	}

	//TODO Ein Memory-Managment-System implementieren
	triangle* triangles = new(std::nothrow) triangle[40000];
	if(!triangles){
		std::cerr << "Konnte keinen Speicher für die statischen Dreiecke allokieren!" << std::endl;
		return -1;
	}
	uint triangle_count = 0;

	default_texture = load_texture("textures/icon.tex");
	if(!default_texture){
		std::cerr << "Konnte default texture nicht laden!" << std::endl;
		return -1;
	}

//	create_cube(triangles, triangle_count, -5, -5, 10, 10, 10, 10);
	read_obj("objects/terrain2.obj", triangles, &triangle_count, 0, 20, 0);

	SetCursorPos(500, 500);

	while(running){
		getMessages(window);
		update(perfAnalyzer.get_avg_data(0));
		clear_window();

		rasterize(triangles, triangle_count, cam);

		draw_int(5, 5, 8/pixel_size, perfAnalyzer.get_avg_data(0), RGBA(130, 130, 130, 255));
		draw_int(5, 55/pixel_size, 8/pixel_size, perfAnalyzer.get_avg_data(1), RGBA(130, 130, 130, 255));
		draw_int(5, 105/pixel_size, 8/pixel_size, perfAnalyzer.get_avg_data(2), RGBA(130, 130, 130, 255));
		draw(window);
	}

	DestroyWindow(window);
	delete[] pixels;
	delete[] depth_buffer;
	delete[] triangles;
	delete[] default_texture;
	return 0;
}

void update(float dt){
	float sin_rotx = sin(cam.rot.x);
	float cos_rotx = cos(cam.rot.x);
	if(W(keyboard)){
		cam.pos.x -= sin_rotx*SPEED*dt;
		cam.pos.z += cos_rotx*SPEED*dt;
	}
	if(S(keyboard)){
		cam.pos.x += sin_rotx*SPEED*dt;
		cam.pos.z -= cos_rotx*SPEED*dt;
	}
	if(D(keyboard)){
		cam.pos.x += cos_rotx*SPEED*dt;
		cam.pos.z += sin_rotx*SPEED*dt;
	}
	if(A(keyboard)){
		cam.pos.x -= cos_rotx*SPEED*dt;
		cam.pos.z -= sin_rotx*SPEED*dt;
	}
	if(SPACE(keyboard)){
		cam.pos.y -= SPEED*dt;
	}
	if(SHIFT(keyboard)){
		cam.pos.y += SPEED*dt;
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
			pixels = new(std::nothrow) uint[buffer_width*buffer_height];
			depth_buffer = new(std::nothrow) uint[buffer_width*buffer_height];
			if(!pixels || !depth_buffer){
				std::cerr << "Konnte keinen Speicher für pixel oder depth buffer allokieren!" << std::endl;
				buffer_width = 0;
				buffer_height = 0;
			}
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
		cam.rot.x -= ((float)pos.x-500) * 0.001;
		cam.rot.y += ((float)pos.y-500) * 0.001;
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
