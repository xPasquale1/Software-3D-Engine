#pragma once

#include <windows.h>
#include <windowsX.h>
#include <algorithm>
#include "util.h"
#include "texture.h"

#define CULL_BACKFACES

//TODO entfernen
#define DEPTH_DIVISOR 10000.f

GLOBALVAR static uint _window_width = 1000;
GLOBALVAR static uint _window_height = 1000;
GLOBALVAR static uint _pixel_size = 2;
GLOBALVAR static uint* _pixels = nullptr;
GLOBALVAR static uint* _depth_buffer = nullptr;
GLOBALVAR static uint* _normal_buffer = nullptr;
GLOBALVAR static BITMAPINFO _bitmapInfo = {};

//Sollte nur einmalig aufgerufen und das handle gespeichert werden
HWND getWindow(HINSTANCE hInstance, const char* name, WNDPROC window_callback){
	WNDCLASS wc = {};
	wc.lpfnWndProc = window_callback;
	wc.hInstance = hInstance;
	wc.lpszClassName = "Window-Class";
	wc.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&wc);

	RECT rect;
    rect.top = 0;
    rect.bottom = _window_height;
    rect.left = 0;
    rect.right = _window_width;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	uint w = rect.right - rect.left;
	uint h = rect.bottom - rect.top;

	HWND hwnd = CreateWindow(wc.lpszClassName, name, WS_VISIBLE | WS_OVERLAPPEDWINDOW, 0, 0, w, h, NULL, NULL, hInstance, NULL);

	_bitmapInfo.bmiHeader.biSize = sizeof(_bitmapInfo.bmiHeader);
	_bitmapInfo.bmiHeader.biPlanes = 1;
	_bitmapInfo.bmiHeader.biBitCount = 32;
	_bitmapInfo.bmiHeader.biCompression = BI_RGB;

	return hwnd;
}

inline void getMessages(HWND window)noexcept{
	MSG msg;
	while(PeekMessage(&msg, window, 0, 0, PM_REMOVE)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

inline constexpr uint RGBA(uchar r, uchar g, uchar b, uchar a=255){return uint(b|g<<8|r<<16|a<<24);}
inline constexpr uchar R(uint color){return uchar(color>>16);}
inline constexpr uchar G(uint color){return uchar(color>>8);}
inline constexpr uchar B(uint color){return uchar(color);}

inline void clear_window()noexcept{
	uint buffer_width = _window_width/_pixel_size;
	uint buffer_height = _window_height/_pixel_size;
	for(uint y=0; y < buffer_height; ++y){
		for(uint x=0; x < buffer_width; ++x){
			uint idx = y*buffer_width+x;
			_pixels[idx] = RGBA(0, 0, 0);
			_depth_buffer[idx] = 0xFFFFFFFF;
		}
	}
}

enum RENDERMODE{
	SHADED_MODE = 0,
	WIREFRAME_MODE,
	DEPTH_MODE,
	DIFFUSE_MODE,
	NORMAL_MODE,
	SPECULAR_MODE
};
GLOBALVAR static RENDERMODE _render_mode = SHADED_MODE;

inline void draw(HWND window)noexcept{
#ifdef PERFORMANCE_ANALYZER
	start_timer(_perfAnalyzer, 1);
#endif
	int buffer_width = _window_width/_pixel_size;
	int buffer_height = _window_height/_pixel_size;
	HDC hdc = GetDC(window);
	StretchDIBits(hdc, 0, _window_height, _window_width, -_window_height, 0, 0, buffer_width, buffer_height, _pixels, &_bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
	ReleaseDC(window, hdc);	//TODO K�nnte unn�tig sein, da Kontext nie geändert wird
#ifdef PERFORMANCE_ANALYZER
	record_data(_perfAnalyzer, 1);
#endif
}

//Kopiert den Buffer der über _render_mode ausgewählt wird in den pixel array
inline void draw_buffer(){
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
			uint depth_color = _depth_buffer[i]/DEPTH_DIVISOR;
			_pixels[i] = RGBA(depth_color, depth_color, depth_color);
		}
		break;
	}
	default: break;
	}
}

inline void draw_rectangle(uint x, uint y, uint dx, uint dy, uint color)noexcept{
	uint buffer_width = _window_width/_pixel_size;
	for(uint i=y; i < y+dy; ++i){
		for(uint j=x; j < x+dx; ++j){
			_pixels[i*buffer_width+j] = color;
		}
	}
}

inline void draw_line(fvec2& start, fvec2& end, uint color)noexcept{
    uint buffer_width = _window_width/_pixel_size;
    int dx = end.x-start.x;
    int dy = end.y-start.y;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    float xinc = dx/(float)steps;
    float yinc = dy/(float)steps;
    float x = start.x;
    float y = start.y;

    for(int i = 0; i <= steps; ++i){
        _pixels[(int)y*buffer_width+(int)x] = color;
        x += xinc;
        y += yinc;
    }
}

//TODO aktuell noch "safe"
inline uint texture(uint* texture, float u, float v)noexcept{
	int u1 = u*texture[0];
	int v1 = v*texture[1];
	int idx = u1*texture[0]+v1+2;
	return texture[idx%(texture[0]*texture[1])];
}

inline void draw_triangle_outline(triangle& tri)noexcept{
	uint buffer_width = _window_width/_pixel_size;
	uint buffer_height = _window_height/_pixel_size;
	fvec3 pt0 = tri.point[0]; fvec3 pt1 = tri.point[1]; fvec3 pt2 = tri.point[2];
	pt0.x = ((pt0.x/2)+0.5)*buffer_width; pt1.x = ((pt1.x/2)+0.5)*buffer_width; pt2.x = ((pt2.x/2)+0.5)*buffer_width;
	pt0.y = ((pt0.y/2)+0.5)*buffer_height; pt1.y = ((pt1.y/2)+0.5)*buffer_height; pt2.y = ((pt2.y/2)+0.5)*buffer_height;
	fvec2 l1 = {pt0.x, pt0.y};
	fvec2 l2 = {pt1.x, pt1.y};
	fvec2 l3 = {pt2.x, pt2.y};
	draw_line(l1, l2, RGBA(255, 255, 255, 255));
	draw_line(l1, l3, RGBA(255, 255, 255, 255));
	draw_line(l2, l3, RGBA(255, 255, 255, 255));
}

GLOBALVAR static uint _pixels_drawn = 0;
GLOBALVAR static uint _pixels_not_drawn = 0;

inline void swapPoints(fvec3& p1, fvec3& p2)noexcept{
    fvec3 temp = {p1.x, p1.y, p1.z};
    p1 = p2;
    p2 = temp;
}

inline void fillBottomFlatTriangle(fvec3& v1, fvec3& v2, fvec3& v3, fvec3& normal, fvec2& uv1, fvec2& uv2, fvec2& uv3, uint color)noexcept{
	int v1x = v1.x; int v2x = v2.x; int v3x = v3.x;
	int v1y = v1.y; int v2y = v2.y; int v3y = v3.y;
	uint buffer_width = _window_width/_pixel_size;
	float invslope1 = (v2x-v1x)/(v2y-v1y);
	float invslope2 = (v3x-v1x)/(v3y-v1y);
	float curx1 = v1x;
	float curx2 = v1x;
	if(invslope2 < invslope1){
		float tmp = invslope1;
		invslope1 = invslope2;
		invslope2 = tmp;
	}
	float invZ1 = 1.f/v1.z; float invZ2 = 1.f/v2.z; float invZ3 = 1.f/v3.z;
	float div = (v2x-v1x)*(v3y-v2y)-(v2y-v1y)*(v3x-v2x);

	float uv1x = uv1.x/v1.z; float uv2x = uv2.x/v2.z; float uv3x = uv3.x/v3.z;
	float uv1y = uv1.y/v1.z; float uv2y = uv2.y/v2.z; float uv3y = uv3.y/v3.z;
	for(int y = v1y; y <= v2y; ++y){
		for(int x=curx1; x <= (int)curx2; ++x){
			//Baryzentrische Gewichtungen
			float m1 = ((float)(v2x - x)*(v3y - y)-(v3x - x)*(v2y - y))/div;
			float m2 = ((float)(v3x - x)*(v1y - y)-(v1x - x)*(v3y - y))/div;
			float m3 = 1.f-m2-m1;
			float depth = 1.f/(m1*invZ1+m2*invZ2+m3*invZ3);
			float scaledDepth = depth*DEPTH_DIVISOR;

			uint idx = y*buffer_width+x;
			if(scaledDepth <= _depth_buffer[idx]){
				_depth_buffer[idx] = (uint)scaledDepth;
				_normal_buffer[idx] = RGBA(127.5*(1+normal.x), 127.5*(1+normal.y), 127.5*(1+normal.z));
				fvec2 uv = {m1*uv1x+m2*uv2x+m3*uv3x, m1*uv1y+m2*uv2y+m3*uv3y};
				uv.x *= depth; uv.y *= depth;
				// _pixels[idx] = texture(_default_texture, uv.x, uv.y);
				_pixels[idx] = color;
			}
		}
		curx1 += invslope1;
		curx2 += invslope2;
	}
}

inline void fillTopFlatTriangle(fvec3& v1, fvec3& v2, fvec3& v3, fvec3& normal, fvec2& uv1, fvec2& uv2, fvec2& uv3, uint color)noexcept{
	uint buffer_width = _window_width/_pixel_size;
	int v1x = v1.x; int v2x = v2.x; int v3x = v3.x;
	int v1y = v1.y; int v2y = v2.y; int v3y = v3.y;
	float invslope1 = (float)(v3x-v1x)/(v3y-v1y);
	float invslope2 = (float)(v3x-v2x)/(v3y-v2y);
	float curx1 = v3x;
	float curx2 = v3x;
	if(invslope2 > invslope1){
		float tmp = invslope1;
		invslope1 = invslope2;
		invslope2 = tmp;
	}
	float invZ1 = 1.f/v1.z; float invZ2 = 1.f/v2.z; float invZ3 = 1.f/v3.z;
	float div = (v2x-v1x)*(v3y-v2y)-(v2y-v1y)*(v3x-v2x);

	float uv1x = uv1.x/v1.z; float uv2x = uv2.x/v2.z; float uv3x = uv3.x/v3.z;
	float uv1y = uv1.y/v1.z; float uv2y = uv2.y/v2.z; float uv3y = uv3.y/v3.z;
	for(int y=v3y; y > v1y; --y){
		for(int x=curx1; x <= (int)curx2; ++x){
			//Baryzentrische Gewichtungen
			float m1 = ((float)(v2x - x)*(v3y - y)-(v3x - x)*(v2y - y))/div;
			float m2 = ((float)(v3x - x)*(v1y - y)-(v1x - x)*(v3y - y))/div;
			float m3 = 1.f-m2-m1;
			float depth = 1.f/(m1*invZ1+m2*invZ2+m3*invZ3);
			float scaledDepth = depth*DEPTH_DIVISOR;

			uint idx = y*buffer_width+x;
			if(scaledDepth <= _depth_buffer[idx]){
				_depth_buffer[idx] = (uint)scaledDepth;
				_normal_buffer[idx] = RGBA(127.5*(1+normal.x), 127.5*(1+normal.y), 127.5*(1+normal.z));
				fvec2 uv = {m1*uv1x+m2*uv2x+m3*uv3x, m1*uv1y+m2*uv2y+m3*uv3y};
				uv.x *= depth; uv.y *= depth;
				// _pixels[idx] = texture(_default_texture, uv.x, uv.y);
				_pixels[idx] = color;
			}
		}
		curx1 -= invslope1;
		curx2 -= invslope2;
	}
}

byte _debugColorIdx = 0;
uint _debugColors[] = {
	RGBA(255, 0, 0),
	RGBA(0, 255, 0),
	RGBA(0, 0, 255),
	RGBA(128, 0, 128),
	RGBA(0, 128, 128),
	RGBA(128, 128, 0),
	RGBA(255, 255, 0),
	RGBA(255, 0, 255),
	RGBA(0, 255, 255)
};

inline uint _getDebugColor(uint idx)noexcept{
	return _debugColors[_debugColorIdx%(sizeof(_debugColors)/sizeof(uint))];
}

inline void draw_triangle(triangle& tri, fvec3& normal)noexcept{
	uint buffer_width = _window_width/_pixel_size;
	uint buffer_height = _window_height/_pixel_size;
	fvec3 pt0 = tri.point[0]; fvec3 pt1 = tri.point[1]; fvec3 pt2 = tri.point[2];
	int pt0x = ((pt0.x*0.5f)+0.5f)*buffer_width; int pt1x = ((pt1.x*0.5f)+0.5f)*buffer_width; int pt2x = ((pt2.x*0.5)+0.5)*buffer_width;
	int pt0y = ((pt0.y*0.5f)+0.5f)*buffer_height; int pt1y = ((pt1.y*0.5f)+0.5f)*buffer_height; int pt2y = ((pt2.y*0.5)+0.5)*buffer_height;
	//pt0 -> pt1 -> pt2 y-absteigend
	float uv0x = tri.uv[0].x; float uv1x = tri.uv[1].x; float uv2x = tri.uv[2].x;
	float uv0y = tri.uv[0].y; float uv1y = tri.uv[1].y; float uv2y = tri.uv[2].y;
	if(pt0y > pt1y) std::swap(pt0x, pt1x), std::swap(pt0y, pt1y), std::swap(pt0.z, pt1.z), std::swap(uv0x, uv1x), std::swap(uv0y, uv1y);
	if(pt0y > pt2y) std::swap(pt0x, pt2x), std::swap(pt0y, pt2y), std::swap(pt0.z, pt2.z), std::swap(uv0x, uv2x), std::swap(uv0y, uv2y);
	if(pt1y > pt2y) std::swap(pt1x, pt2x), std::swap(pt1y, pt2y), std::swap(pt1.z, pt2.z), std::swap(uv1x, uv2x), std::swap(uv1y, uv2y);
	//x-Inkrement für pt0 - pt2
	if(pt0y == pt2y) return;	//TODO vllt unnötig?
	if(pt0x == pt1x && pt0x == pt2x) return;
	float xIncP0P21 = (float)(pt2x-pt0x)/(pt2y-pt0y);
	float xIncP0P22 = xIncP0P21;
	//x-Inkrement für pt0 - pt1
	float xIncP0P1 = (float)(pt1x-pt0x)/(pt1y-pt0y);
	//Laufe von pt1 - pt2
	float xIncP1P2 = (float)(pt2x-pt1x)/(pt2y-pt1y);
	//Initialisiere die Startkoordinaten
	float startX = pt0x+0.5f;
	float endX = pt0x+0.5f;
	//Vorberechnungen
	float invZ1 = 1.f/pt0.z; float invZ2 = 1.f/pt1.z; float invZ3 = 1.f/pt2.z;
	float div = 1.f/((pt1x-pt0x)*(pt2y-pt1y)-(pt1y-pt0y)*(pt2x-pt1x));
	uv0x *= invZ1; uv1x *= invZ2; uv2x *= invZ3;
	uv0y *= invZ1; uv1y *= invZ2; uv2y *= invZ3;
	if(xIncP0P1 < xIncP0P21) std::swap(xIncP0P21, xIncP0P1), std::swap(xIncP0P22, xIncP1P2);
	if(pt0y != pt1y){
		for(int y=pt0y; y < pt1y; ++y){
			for(int x=startX; x <= (int)endX; ++x){
				float m1 = ((float)(pt1x - x)*(pt2y - y)-(pt2x - x)*(pt1y - y))*div;
				float m2 = ((float)(pt2x - x)*(pt0y - y)-(pt0x - x)*(pt2y - y))*div;
				float m3 = 1.f-m2-m1;
				if(m1 < 0.f || m2 < 0.f || m3 < 0.f){
					// uint idx = y*buffer_width+x;
					// _pixels[idx] = RGBA(255, 0, 0);
					continue;
				}
				float depth = 1.f/(m1*invZ1+m2*invZ2+m3*invZ3);
				uint scaledDepth = depth*DEPTH_DIVISOR;

				uint idx = y*buffer_width+x;
				if(scaledDepth < _depth_buffer[idx]){
					#ifdef PERFORMANCE_ANALYZER
					++_pixels_drawn;
					#endif
					_depth_buffer[idx] = scaledDepth;
					float uvx = (m1*uv0x + m2*uv1x + m3*uv2x)*depth;
					float uvy = (m1*uv0y + m2*uv1y + m3*uv2y)*depth;
					_pixels[idx] = texture(_default_texture, uvx, uvy);
					_normal_buffer[idx] = RGBA(127.5*(1+normal.x), 127.5*(1+normal.y), 127.5*(1+normal.z));
					// _pixels[idx] = RGBA(R(pt0col)*m1+R(pt1col)*m2+R(pt2col)*m3, G(pt0col)*m1+G(pt1col)*m2+G(pt2col)*m3, B(pt0col)*m1+B(pt1col)*m2+B(pt2col)*m3);
				}
				#ifdef PERFORMANCE_ANALYZER
				else ++_pixels_not_drawn;
				#endif
			}
			startX += xIncP0P21;
			endX += xIncP0P1;
		}
	}else{
		startX = pt1x+0.5f;
		if(startX > endX) std::swap(startX, endX);
	}
	for(int y=pt1y; y < pt2y; ++y){
		for(int x=startX; x <= (int)endX; ++x){
			float m1 = ((float)(pt1x - x)*(pt2y - y)-(pt2x - x)*(pt1y - y))*div;
			float m2 = ((float)(pt2x - x)*(pt0y - y)-(pt0x - x)*(pt2y - y))*div;
			float m3 = 1.f-m2-m1;
			if(m1 < 0.f || m2 < 0.f || m3 < 0.f){
				// uint idx = y*buffer_width+x;
				// _pixels[idx] = RGBA(255, 0, 0);
				continue;
			}
			float depth = 1.f/(m1*invZ1+m2*invZ2+m3*invZ3);
			uint scaledDepth = depth*DEPTH_DIVISOR;

			uint idx = y*buffer_width+x;
			if(scaledDepth < _depth_buffer[idx]){
				#ifdef PERFORMANCE_ANALYZER
				++_pixels_drawn;
				#endif
				_depth_buffer[idx] = scaledDepth;
				float uvx = (m1*uv0x + m2*uv1x + m3*uv2x)*depth;
				float uvy = (m1*uv0y + m2*uv1y + m3*uv2y)*depth;
				_pixels[idx] = texture(_default_texture, uvx, uvy);
				_normal_buffer[idx] = RGBA(127.5*(1+normal.x), 127.5*(1+normal.y), 127.5*(1+normal.z));
				// _pixels[idx] = RGBA(R(pt0col)*m1+R(pt1col)*m2+R(pt2col)*m3, G(pt0col)*m1+G(pt1col)*m2+G(pt2col)*m3, B(pt0col)*m1+B(pt1col)*m2+B(pt2col)*m3);
			}
			#ifdef PERFORMANCE_ANALYZER
			else ++_pixels_not_drawn;
			#endif
		}
		startX += xIncP0P22;
		endX += xIncP1P2;
	}
}

inline void draw_triangle_old(triangle& tri, fvec3& normal)noexcept{
	uint buffer_width = _window_width/_pixel_size;
	uint buffer_height = _window_height/_pixel_size;
	fvec3 pt0 = tri.point[0]; fvec3 pt1 = tri.point[1]; fvec3 pt2 = tri.point[2];
	pt0.x = ((pt0.x*0.5)+0.5)*buffer_width; pt1.x = ((pt1.x*0.5)+0.5)*buffer_width; pt2.x = ((pt2.x*0.5)+0.5)*buffer_width;
	pt0.y = ((pt0.y*0.5)+0.5)*buffer_height; pt1.y = ((pt1.y*0.5)+0.5)*buffer_height; pt2.y = ((pt2.y*0.5)+0.5)*buffer_height;
	if(pt0.y > pt1.y){
		swapPoints(pt0, pt1);
	}
	if(pt0.y > pt2.y){
		swapPoints(pt0, pt2);
	}
	if(pt1.y > pt2.y){
		swapPoints(pt1, pt2);
	}

	//TODO if else könnte unnötig für die Performance sein, fill top oder bottom macht dann halt nix
	if((int)pt1.y == (int)pt2.y){
		fillBottomFlatTriangle(pt0, pt1, pt2, normal, tri.uv[0], tri.uv[1], tri.uv[2], tri.colors[0]);
	}
	else if((int)pt0.y == (int)pt1.y){
		fillTopFlatTriangle(pt0, pt1, pt2, normal, tri.uv[0], tri.uv[1], tri.uv[2], tri.colors[0]);
	}
	else{
		fvec3 pt3 = {(pt0.x + ((pt1.y - pt0.y) / (pt2.y - pt0.y)) * (pt2.x - pt0.x)), pt1.y, 0};
		pt3.z = interpolateLinear(pt0.z, pt2.z, {pt0.x, pt0.y}, {pt2.x, pt2.y}, {pt3.x, pt3.y});
		fillBottomFlatTriangle(pt0, pt1, pt3, normal, tri.uv[0], tri.uv[1], tri.uv[2], tri.colors[0]);
		fillTopFlatTriangle(pt1, pt3, pt2, normal, tri.uv[0], tri.uv[1], tri.uv[2], tri.colors[0]);
	}
}

//TODO man kann den Anfang der "scanline" berechnen anstatt einer bounding box
//TODO scheinbar tut opengl alle vertex attribute interpolieren, auch per vertex normalen... überlegen wie man das implementieren kann
//Vielleicht kann man die Gewichtungen in einem Buffer nach aussen geben, und dann das alles selbst berechnen
inline void draw_triangle_old2(triangle& tri, fvec3& normal)noexcept{
	uint buffer_width = _window_width/_pixel_size;
	uint buffer_height = _window_height/_pixel_size;
	fvec3 pt0 = tri.point[0]; fvec3 pt1 = tri.point[1]; fvec3 pt2 = tri.point[2];
	pt0.x = ((pt0.x*0.5)+0.5)*buffer_width; pt1.x = ((pt1.x*0.5)+0.5)*buffer_width; pt2.x = ((pt2.x*0.5)+0.5)*buffer_width;
	pt0.y = ((pt0.y*0.5)+0.5)*buffer_height; pt1.y = ((pt1.y*0.5)+0.5)*buffer_height; pt2.y = ((pt2.y*0.5)+0.5)*buffer_height;

	uint ymin = min(pt0.y, min(pt1.y, pt2.y));
	uint ymax = max(pt0.y, max(pt1.y, pt2.y));
	uint xmin = min(pt0.x, min(pt1.x, pt2.x));
	uint xmax = max(pt0.x, max(pt1.x, pt2.x));

	fvec2 vs1 = {pt1.x - pt0.x, pt1.y - pt0.y};
	fvec2 vs2 = {pt2.x - pt0.x, pt2.y - pt0.y};
	float div = cross(vs1, vs2);

	//Berechne uv/z vor
	float uv0x = tri.uv[0].x/tri.point[0].z; float uv1x = tri.uv[1].x/tri.point[1].z; float uv2x = tri.uv[2].x/tri.point[2].z;
	float uv0y = tri.uv[0].y/tri.point[0].z; float uv1y = tri.uv[1].y/tri.point[1].z; float uv2y = tri.uv[2].y/tri.point[2].z;

	//Normalenvektor
	uint normalColor = RGBA(127.5f*(1+normal.x), 127.5f*(1+normal.y), 127.5f*(1+normal.z));

	//Berechne u und v initial und inkrementiere dann nur noch entsprechend
	fvec2 q = {xmin - pt0.x, ymin - pt0.y};
	float u = cross(q, vs2)/div; float v = cross(vs1, q)/div;
	float deltaX_u = (pt2.y - pt0.y)/div; float deltaX_v = (pt1.y - pt0.y)/div;
	float deltaY_u = (pt2.x - pt0.x)/div; float deltaY_v = (pt1.x - pt0.x)/div;
	//Berechne 1/pt<n>.z vor
	float pt0_z_inv = 1./pt0.z; float pt1_z_inv = 1./pt1.z; float pt2_z_inv = 1./pt2.z;
	for(uint y = ymin; y <= ymax; ++y){
		float tmp_u = u; float tmp_v = v;
		bool wasIn = false;
		for(uint x = xmin; x <= xmax; ++x){
			//w -> pt0, u -> pt1, v -> pt2
			if((v >= 0)&&(u >= 0)&&(u + v <= 1)){
				wasIn = true;
				float w = 1.f-u-v;
				uint idx = y*buffer_width+x;
				float depth = 1.f/(w*pt0_z_inv + u*pt1_z_inv + v*pt2_z_inv);	//TODO Iterativ lösbar?
				//TODO depth buffer endlich eine Range geben damit eine erwartete Genauigkeit erfasst werden kann
				float inc_depth = depth*DEPTH_DIVISOR;
				if(inc_depth <= _depth_buffer[idx]){
					#ifdef PERFORMANCE_ANALYZER
					++_pixels_drawn;
					#endif
					_depth_buffer[idx] = (uint)inc_depth;
					float s = (w*uv0x + u*uv1x + v*uv2x);	//TODO Iterativ lösbar?
					float t = (w*uv0y + u*uv1y + v*uv2y);	//TODO Iterativ lösbar?
					s *= depth; t *= depth;
					//TODO dynamisch texturen lesen können/materials hinzufügen
					_pixels[idx] = texture(_default_texture, s, t);
					_normal_buffer[idx] = normalColor;
				}
				#ifdef PERFORMANCE_ANALYZER
				else ++_pixels_not_drawn;
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

inline bool ray_plane_intersection_new(plane& p, fvec3& start, fvec3& end, fvec2& start_uv, fvec2& end_uv, fvec3& cp)noexcept{
	fvec3 dir = {end.x-start.x, end.y-start.y, end.z-start.z};
	float d = dot(p.normal, dir);
	if(d != 0){
		fvec3 tmp = {p.pos.x-start.x, p.pos.y-start.y, p.pos.z-start.z};
	    float t = dot(tmp, p.normal)/d;
	    cp = {start.x+dir.x*t, start.y+dir.y*t, start.z+dir.z*t};
	    end_uv.x = start_uv.x*(1-t)+end_uv.x*t;
	    end_uv.y = start_uv.y*(1-t)+end_uv.y*t;
	    return true;
	}
	return false;
}

inline __attribute__((always_inline)) void remove(triangle* buffer, byte& count, byte& temp_count, byte& i)noexcept{
	buffer[i] = buffer[temp_count-1];
	--i; --temp_count; --count;
	return;
}

inline void clip_plane(plane& p, triangle* buffer, byte& count)noexcept{
	byte tmp_off = count;		//Offset wo das aktuelle neue Dreieck hinzugef�gt werden soll
	byte offset = count;		//Originaler Offset der neuen Dreiecke
	byte temp_count = count;	//Index des letzten originalen Dreiecks
	for(byte i=0; i < temp_count; ++i){
		fvec3 in_v[3]; fvec3 out_v[3];
		fvec2 in_uv[3]; fvec2 out_uv[3];
		byte in = 0; byte out = 0;
		for(int j=0; j < 3; ++j){
			fvec3 vec = buffer[i].point[j]; vec.x -= p.pos.x; vec.y -= p.pos.y; vec.z -= p.pos.z;
			float dist = dot(vec, p.normal);
			if(dist < 0.){
				out_v[out] = buffer[i].point[j];
				out_uv[out] = buffer[i].uv[j];
				++out;
			}else{
				in_v[in] = buffer[i].point[j];
				in_uv[in] = buffer[i].uv[j];
				++in;
			}
		}
		switch(in){
			case 0:{	//Dreieck liegt komplett ausserhalb, es muss entfernt werden
				remove(buffer, count, temp_count, i);
				break;
			}
			case 1:{	//Das aktuelle Dreieck kann einfach ge�ndert werden
				ray_plane_intersection_new(p, in_v[0], out_v[0], in_uv[0], out_uv[0], buffer[i].point[1]);
				buffer[i].uv[1] = out_uv[0];
				ray_plane_intersection_new(p, in_v[0], out_v[1], in_uv[0], out_uv[1], buffer[i].point[2]);
				buffer[i].uv[2] = out_uv[1];
				buffer[i].point[0] = in_v[0];
				buffer[i].uv[0] = in_uv[0];
				break;
			}
			case 2:{	//2 neue Dreiecke m�ssen hinzugef�gt werden und das aktuelle entfernt
				remove(buffer, count, temp_count, i);
				fvec3 dir = in_v[0];
				dir.x -= out_v[0].x; dir.y -= out_v[0].y; dir.z -= out_v[0].z;
				fvec2 end_uv_tmp = out_uv[0];
				ray_plane_intersection_new(p, in_v[0], out_v[0], in_uv[0], out_uv[0], buffer[tmp_off].point[2]);
				buffer[tmp_off].uv[2] = out_uv[0];
				buffer[tmp_off].point[0] = in_v[0];
				buffer[tmp_off].uv[0] = in_uv[0];
				buffer[tmp_off].point[1] = in_v[1];
				buffer[tmp_off].uv[1] = in_uv[1];
				dir = in_v[1];
				dir.x -= out_v[0].x; dir.y -= out_v[0].y; dir.z -= out_v[0].z;
				out_uv[0] = end_uv_tmp;
				ray_plane_intersection_new(p, in_v[1], out_v[0], in_uv[1], out_uv[0], buffer[tmp_off+1].point[2]);
				buffer[tmp_off+1].uv[2] = out_uv[0];
				buffer[tmp_off+1].point[0] = buffer[tmp_off].point[2];
				buffer[tmp_off+1].uv[0] = buffer[tmp_off].uv[2];
				buffer[tmp_off+1].point[1] = in_v[1];
				buffer[tmp_off+1].uv[1] = in_uv[1];
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

inline byte clipping(triangle* buffer)noexcept{
	byte count = 1;
	float aspect_ratio = _window_width/_window_height;

	plane pz = {}; pz.normal = {0, 0, 1}; pz.pos = {0, 0, 0};
	normalize(pz.normal);
	clip_plane(pz, buffer, count);

	plane px = {}; px.normal = {XMIN/aspect_ratio, 0, 1};
	normalize(px.normal);
	clip_plane(px, buffer, count);

	plane pnx = {}; pnx.normal = {XMAX/aspect_ratio, 0, 1};
	normalize(pnx.normal);
	clip_plane(pnx, buffer, count);

	plane py = {}; py.normal = {0, YMIN, 1};
	normalize(py.normal);
	clip_plane(py, buffer, count);

	plane pny = {}; pny.normal = {0, YMAX, 1};
	normalize(pny.normal);
	clip_plane(pny, buffer, count);

	return count;
}

struct camera{
	float focal_length;
	fvec3 pos;
	fvec2 rot;	//Yaw, pitch. rot.x ist die Rotation um die Y-Achse weil... uhh ja
};

inline void rasterize(triangle* tris, uint start_idx, uint triangle_count, camera* cam)noexcept{
#ifdef PERFORMANCE_ANALYZER
	_perfAnalyzer.total_triangles += triangle_count - start_idx;
	_pixels_drawn = 0;
	_pixels_not_drawn = 0;
#endif
	float rotm[3][3];
	float aspect_ratio = _window_width/_window_height;
	float sin_rotx = sin(cam->rot.x);
	float cos_rotx = cos(cam->rot.x);
	float sin_roty = sin(cam->rot.y);
	float cos_roty = cos(cam->rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty;		rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty;	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;
	triangle buffer[32];
    for(uint i=start_idx; i < triangle_count; ++i){
    	triangle tri = tris[i];
    	//TODO kann man auch im Dreieck speichern
    	fvec3 l1 = {tri.point[1].x-tri.point[0].x, tri.point[1].y-tri.point[0].y, tri.point[1].z-tri.point[0].z};
    	fvec3 l2 = {tri.point[2].x-tri.point[0].x, tri.point[2].y-tri.point[0].y, tri.point[2].z-tri.point[0].z};
    	fvec3 world_normal = cross(l1, l2);
    	normalize(world_normal);
    	for(int j=0; j < 3; ++j){
    		float d[3];
    		d[0] = (tri.point[j].x-cam->pos.x);
    		d[1] = (tri.point[j].y-cam->pos.y);
    		d[2] = (tri.point[j].z-cam->pos.z);
    		float v[3]{0};
    	    for (uint a=0; a < 3; ++a){
    	        for (uint b=0; b < 3; ++b){
    	        	v[a] += (rotm[a][b]*d[b]);
    	        }
    	    }
    	    tri.point[j].x = v[0];
    	    tri.point[j].y = v[1];
    	    tri.point[j].z = v[2];
    	}
    	fvec3 l01 = {tri.point[1].x-tri.point[0].x, tri.point[1].y-tri.point[0].y, tri.point[1].z-tri.point[0].z};
    	fvec3 l02 = {tri.point[2].x-tri.point[0].x, tri.point[2].y-tri.point[0].y, tri.point[2].z-tri.point[0].z};
    	fvec3 normal = cross(l01, l02);
    	normalize(normal);
#ifdef CULL_BACKFACES
    	if(dot(tri.point[0], normal) > 0) continue;
#endif
    	buffer[0] = tri;
    	byte count = clipping(buffer);
    	for(byte j=0; j < count; ++j){
    		fvec3 pt1 = buffer[j].point[0]; fvec3 pt2 = buffer[j].point[1]; fvec3 pt3 = buffer[j].point[2];
    		buffer[j].point[0].x = pt1.x*(cam->focal_length/pt1.z)/aspect_ratio; buffer[j].point[0].y = pt1.y*(cam->focal_length/pt1.z);
    		buffer[j].point[1].x = pt2.x*(cam->focal_length/pt2.z)/aspect_ratio; buffer[j].point[1].y = pt2.y*(cam->focal_length/pt2.z);
    		buffer[j].point[2].x = pt3.x*(cam->focal_length/pt3.z)/aspect_ratio; buffer[j].point[2].y = pt3.y*(cam->focal_length/pt3.z);
    		buffer[j].point[0].z = pt1.z; buffer[j].point[1].z = pt2.z; buffer[j].point[2].z = pt3.z;
    		switch(_render_mode){
    		case WIREFRAME_MODE:
    			draw_triangle_outline(buffer[j]);
    			break;
    		default:
    			draw_triangle_old2(buffer[j], world_normal);
    			break;
    		}
#ifdef PERFORMANCE_ANALYZER
    		_perfAnalyzer.drawn_triangles += 1;
#endif
    	}
    }
    return;
}
