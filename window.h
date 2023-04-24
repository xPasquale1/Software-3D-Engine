#pragma once

#include <windows.h>
#include <windowsX.h>
#include <algorithm>
#include "util.h"
#include "texture.h"

//#define WIREFRAME
#define CULL_BACKFACES

GLOBALVAR static uint _window_width = 1000;
GLOBALVAR static uint _window_height = 1000;
GLOBALVAR static uint _pixel_size = 2;
GLOBALVAR static uint* _pixels = nullptr;
GLOBALVAR static uint* _depth_buffer = nullptr;
GLOBALVAR static BITMAPINFO _bitmapInfo = {};

//Sollte nur einmalig aufgerufen und das handle gespeichert werden
HWND getWindow(HINSTANCE hInstance, const char* name, WNDPROC window_callback){
	WNDCLASS wc = {};
	wc.lpfnWndProc = window_callback;
	wc.hInstance = hInstance;
	wc.lpszClassName = "Window-Class";
	wc.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&wc);

	HWND hwnd = CreateWindow(wc.lpszClassName, name, WS_VISIBLE | WS_OVERLAPPEDWINDOW, 0, 0, _window_width, _window_height, NULL, NULL, hInstance, NULL);

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

inline void clear_window()noexcept{
	uint buffer_width = _window_width/_pixel_size;
	uint buffer_height = _window_height/_pixel_size;
	for(uint y=0; y < buffer_height; ++y){
		for(uint x=0; x < buffer_width; ++x){
			uint idx = y*buffer_width+x;
			_pixels[idx] = 0;
			_depth_buffer[idx] = 0xFFFFFFFF;
		}
	}
}

inline void draw(HWND window)noexcept{
#ifdef PERFORMANCE_ANALYZER
	_perfAnalyzer.start_timer(1);
#endif
	int buffer_width = _window_width/_pixel_size;
	int buffer_height = _window_height/_pixel_size;
	HDC hdc = GetDC(window);
	StretchDIBits(hdc, 0, _window_height, _window_width, -_window_height, 0, 0, buffer_width, buffer_height, _pixels, &_bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
	ReleaseDC(window, hdc);
#ifdef PERFORMANCE_ANALYZER
	_perfAnalyzer.record_data(1);
#endif
}

inline constexpr uint RGBA(uchar r, uchar g, uchar b, uchar a=255){return uint(b|g<<8|r<<16|a<<24);}
inline constexpr uchar R(uint color){return uchar(color>>16);}
inline constexpr uchar G(uint color){return uchar(color>>8);}
inline constexpr uchar B(uint color){return uchar(color);}

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

inline uint texture(uint* texture, float u, float v)noexcept{
	int u1 = u*texture[0];
	int v1 = v*texture[1];
	int idx = u1*texture[0]+v1+2;
	return texture[idx];
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

inline void draw_triangle(triangle& tri)noexcept{
	uint buffer_width = _window_width/_pixel_size;
	uint buffer_height = _window_height/_pixel_size;
	fvec3 pt0 = tri.point[0]; fvec3 pt1 = tri.point[1]; fvec3 pt2 = tri.point[2];
	pt0.x = ((pt0.x/2)+0.5)*buffer_width; pt1.x = ((pt1.x/2)+0.5)*buffer_width; pt2.x = ((pt2.x/2)+0.5)*buffer_width;
	pt0.y = ((pt0.y/2)+0.5)*buffer_height; pt1.y = ((pt1.y/2)+0.5)*buffer_height; pt2.y = ((pt2.y/2)+0.5)*buffer_height;
#ifndef WIREFRAME
	uint ymin = std::min(pt0.y, std::min(pt1.y, pt2.y));
	uint ymax = std::max(pt0.y, std::max(pt1.y, pt2.y));
	uint xmin = std::min(pt0.x, std::min(pt1.x, pt2.x));
	uint xmax = std::max(pt0.x, std::max(pt1.x, pt2.x));

	fvec2 vs1 = {pt1.x - pt0.x, pt1.y - pt0.y};
	fvec2 vs2 = {pt2.x - pt0.x, pt2.y - pt0.y};
	float div = cross(vs1, vs2);

	//Berechne uv/z vor
	float uv0x = tri.uv[0].x/tri.point[0].z; float uv1x = tri.uv[1].x/tri.point[1].z; float uv2x = tri.uv[2].x/tri.point[2].z;
	float uv0y = tri.uv[0].y/tri.point[0].z; float uv1y = tri.uv[1].y/tri.point[1].z; float uv2y = tri.uv[2].y/tri.point[2].z;

	//Berechne u und v initial und inkrementiere dann nur noch entsprechend
	fvec2 q = {xmin - pt0.x, ymin - pt0.y};
	float u = cross(q, vs2)/div; float v = cross(vs1, q)/div;
	float deltaX_u = (pt2.y - pt0.y)/div; float deltaX_v = (pt1.y - pt0.y)/div;
	float deltaY_u = (pt2.x - pt0.x)/div; float deltaY_v = (pt1.x - pt0.x)/div;
	for(uint y = ymin; y <= ymax; ++y){
		float tmp_u = u; float tmp_v = v;
		for(uint x = xmin; x <= xmax; ++x){
			//w -> pt0, u -> pt1, v -> pt2
			if((u >= 0)&&(v >= 0)&&(u + v <= 1)){
				float w = 1-u-v;
				uint idx = y*buffer_width+x;
				float depth = 1./(w/pt0.z + u/pt1.z + v/pt2.z);
				float inc_depth = depth*10000;	//TODO depth buffer endlich eine Range geben damit eine erwartete Genauigkeit erfasst werden kann
				if(inc_depth <= _depth_buffer[idx]){
					_depth_buffer[idx] = (uint)inc_depth;
					float s = (w*uv0x + u*uv1x + v*uv2x);
					float t = (w*uv0y + u*uv1y + v*uv2y);
					s *= depth; t *= depth;
					_pixels[idx] = texture(_default_texture, s, t);
				}
			}
	        u += deltaX_u; v -= deltaX_v;
		}
		u = tmp_u; v = tmp_v;
		u -= deltaY_u; v += deltaY_v;
	}
#else
	fvec2 l1 = {pt0.x, pt0.y};
	fvec2 l2 = {pt1.x, pt1.y};
	fvec2 l3 = {pt2.x, pt2.y};
	draw_line(l1, l2, RGBA(255, 255, 255, 255));
	draw_line(l1, l3, RGBA(255, 255, 255, 255));
	draw_line(l2, l3, RGBA(255, 255, 255, 255));
#endif
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
	--i;
	--temp_count;
	--count;
	return;
}

inline void clip_plane(plane& p, triangle* buffer, byte& count)noexcept{
	byte tmp_off = count;		//Offset wo das aktuelle neue Dreieck hinzugefügt werden soll
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
			case 1:{	//Das aktuelle Dreieck kann einfach geändert werden
				ray_plane_intersection_new(p, in_v[0], out_v[0], in_uv[0], out_uv[0], buffer[i].point[1]);
				buffer[i].uv[1] = out_uv[0];
				ray_plane_intersection_new(p, in_v[0], out_v[1], in_uv[0], out_uv[1], buffer[i].point[2]);
				buffer[i].uv[2] = out_uv[1];
				buffer[i].point[0] = in_v[0];
				buffer[i].uv[0] = in_uv[0];
				break;
			}
			case 2:{	//2 neue Dreiecke müssen hinzugefügt werden und das aktuelle entfernt
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

#define XMIN -1.001f
#define XMAX 1.001f
#define YMIN -1.001f
#define YMAX 1.001f

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

inline constexpr uint color_picker(uint i)noexcept{
	switch(i){
	case 0: return RGBA(255, 0, 0, 255);
	case 1: return RGBA(0, 255, 0, 255);
	case 2: return RGBA(0, 0, 255, 255);
	case 3: return RGBA(255, 0, 255, 255);
	case 4: return RGBA(0, 255, 255, 255);
	case 5: return RGBA(255, 255, 0, 255);
	}
	return RGBA(120, 120, 120, 255);
}

inline void rasterize(triangle* tris, uint start_idx, uint triangle_count, camera* cam, uchar render_mode)noexcept{
#ifdef PERFORMANCE_ANALYZER
	_perfAnalyzer.total_triangles += triangle_count - start_idx;
#endif
	float rotm[3][3];
	float aspect_ratio = _window_width/_window_height;
	float sin_rotx = sin(cam->rot.x);
	float cos_rotx = cos(cam->rot.x);
	float sin_roty = sin(cam->rot.y);
	float cos_roty = cos(cam->rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty; 	rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty; 	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;
	triangle buffer[32] = {};
    for(uint i=start_idx; i < triangle_count; ++i){
    	triangle tri = tris[i];
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
#ifdef CULL_BACKFACES
    	fvec3 l01 = {tri.point[1].x-tri.point[0].x, tri.point[1].y-tri.point[0].y, tri.point[1].z-tri.point[0].z};
    	fvec3 l02 = {tri.point[2].x-tri.point[0].x, tri.point[2].y-tri.point[0].y, tri.point[2].z-tri.point[0].z};
    	fvec3 normal = cross(l01, l02);
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
    		if(render_mode&0b1) draw_triangle_outline(buffer[j]);
    		else draw_triangle(buffer[j]);
#ifdef PERFORMANCE_ANALYZER
    		_perfAnalyzer.drawn_triangles += 1;
#endif
    	}
    }
    return;
}
