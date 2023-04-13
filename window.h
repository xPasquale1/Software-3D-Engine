#pragma once

#include <windows.h>
#include <windowsX.h>
#include <algorithm>
#include "util.h"
#include "texture.h"

//#define WIREFRAME

static uint window_width = 1000;
static uint window_height = 1000;
static uint pixel_size = 2;
static uint* pixels = nullptr;
static uint* depth_buffer = nullptr;
static BITMAPINFO bitmapInfo = {};

//Sollte nur einmalig aufgerufen und das handle gespeichert werden
HWND getWindow(HINSTANCE hInstance, const char* name, WNDPROC window_callback){
	WNDCLASS wc = {};
	wc.lpfnWndProc = window_callback;
	wc.hInstance = hInstance;
	wc.lpszClassName = "Window-Class";
	RegisterClass(&wc);

	HWND hwnd = CreateWindow(wc.lpszClassName, name, WS_VISIBLE | WS_OVERLAPPEDWINDOW, 0, 0, window_width, window_height, NULL, NULL, hInstance, NULL);

	bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;

	return hwnd;
}

inline void getMessages(HWND window){
	MSG msg;
	while(PeekMessage(&msg, window, 0, 0, PM_REMOVE)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

inline void clear_window(){
	uint buffer_width = window_width/pixel_size;
	uint buffer_height = window_height/pixel_size;
	for(uint y=0; y < buffer_height; ++y){
		for(uint x=0; x < buffer_width; ++x){
			uint idx = y*buffer_width+x;
			pixels[idx] = 0;
			depth_buffer[idx] = 0xFFFFFFFF;
		}
	}
}

inline void draw(HWND window){
#ifdef PERFORMANCE_ANALYZER
	perfAnalyzer.start_timer(1);
#endif
	int buffer_width = window_width/pixel_size;
	int buffer_height = window_height/pixel_size;
	HDC hdc = GetDC(window);
	StretchDIBits(hdc, 0, window_height, window_width, -window_height, 0, 0, buffer_width, buffer_height, pixels, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
	ReleaseDC(window, hdc);
#ifdef PERFORMANCE_ANALYZER
	perfAnalyzer.record_data(1);
#endif
}

inline constexpr uint RGBA(uchar r, uchar g, uchar b, uchar a=255){return uint(b|g<<8|r<<16|a<<24);}
inline constexpr uchar R(uint color){return uchar(color>>16);}
inline constexpr uchar G(uint color){return uchar(color>>8);}
inline constexpr uchar B(uint color){return uchar(color);}

inline void draw_rectangle(uint x, uint y, uint dx, uint dy, uint color){
	uint buffer_width = window_width/pixel_size;
	for(uint i=y; i < y+dy; ++i){
		for(uint j=x; j < x+dx; ++j){
			pixels[i*buffer_width+j] = color;
		}
	}
}

inline void draw_line(fvec2& start, fvec2& end, uint color){
    uint buffer_width = window_width/pixel_size;
    int dx = end.x-start.x;
    int dy = end.y-start.y;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    float xinc = dx/(float)steps;
    float yinc = dy/(float)steps;
    float x = start.x;
    float y = start.y;

    for(int i = 0; i <= steps; ++i){
        pixels[(int)y*buffer_width+(int)x] = color;
        x += xinc;
        y += yinc;
    }
}

inline uint texture(float u, float v){
	int u1 = u*(TEST_TEXTURE_WIDTH);
	int v1 = v*(TEST_TEXTURE_HEIGHT);
	uchar val = test_texture0[u1*TEST_TEXTURE_WIDTH+v1];
	return RGBA(val, val, val, 255);
}

inline void draw_triangle(triangle& tri){
	uint buffer_width = window_width/pixel_size;
	uint buffer_height = window_height/pixel_size;
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

	for(uint y = ymin; y <= ymax; y++){
		for(uint x = xmin; x <= xmax; x++){
			fvec2 q = {x - pt0.x, y - pt0.y};
			float div = cross(vs1, vs2);
			//w -> pt0, u -> pt1, v -> pt2
			float u = cross(q, vs2)/div;
			float v = cross(vs1, q)/div;
			if((u >= 0)&&(v >= 0)&&(u + v < 1)){//Eigentlich sollte es <= 1 sein aber floats sind leider nicht perfekt...
				float w = 1-u-v;
				uint idx = y*buffer_width+x;
				//TODO depth buffer endlich eine range geben damit eine gute Genauigkeit erfasst werden kann
				float depth = w*pt0.z*1000 + u*pt1.z*1000 + v*pt2.z*1000;
				if(depth <= depth_buffer[idx]){
					depth_buffer[idx] = (uint)depth;
//					pixels[idx] = RGBA(255*w, 255*u, 255*v, 255);
					float div2 = (pt1.z*pt2.z+pt2.z*u*(pt0.z-pt1.z)+pt1.z*v*(pt0.z-pt2.z));
					float a = (pt2.z*pt0.z*u)/div2;
					float b = (pt1.z*pt0.z*v)/div2;
					pixels[idx] = texture(a, b);
				}
			}
		}
	}
#else
	fvec2 l1 = {pt0.x, pt0.y};
	fvec2 l2 = {pt1.x, pt1.y};
	fvec2 l3 = {pt2.x, pt2.y};
	draw_line(l1, l2, tri.color);
	draw_line(l1, l3, tri.color);
	draw_line(l2, l3, tri.color);
#endif
}

struct plane{
	fvec3 pos;
	fvec3 normal;
};

inline bool ray_plane_intersection(plane& p, fvec3& dir, fvec3& pt, fvec3& cp){
	normalize(dir);
	float d = dot(dir, p.normal);
	if(d > 0){
		fvec3 i = p.pos;
		i.x -= pt.x; i.y -= pt.y; i.z -= pt.z;
		float t = dot(i, p.normal)/d;
		cp = pt; dir.x*=t; dir.y*=t; dir.z*=t;
		cp.x += dir.x; cp.y += dir.y; cp.z += dir.z;
		return true;
	}
	return false;
}

inline void remove(triangle* buffer, byte& count, byte& temp_count, byte& i){
	buffer[i] = buffer[temp_count-1];
	--i;
	--temp_count;
	--count;
	return;
}

//TODO  uv Koordinaten anpassen (Können vllt direkt über den t Wert in der ray_plane_intersection Funktion bestimmt werden?)
inline void clip_plane(plane& p, triangle* buffer, byte& count){
	byte tmp_off = count;		//Offset wo das aktuelle neue Dreieck hinzugefügt werden soll
	byte offset = count;		//Originaler Offset der neuen Dreiecke
	byte temp_count = count;	//Index des letzten originalen Dreiecks
	for(byte i=0; i < temp_count; ++i){
		fvec3 in_v[3], out_v[3];
		byte in = 0; byte out = 0;
		for(int j=0; j < 3; ++j){
			fvec3 vec = buffer[i].point[j]; vec.x -= p.pos.x; vec.y -= p.pos.y; vec.z -= p.pos.z;
			float dist = dot(vec, p.normal);
			(dist < 0.) ? out_v[out++] = vec : in_v[in++] = vec;
		}
		switch(in){
			case 0:{	//Dreieck liegt komplett ausserhalb, es muss entfernt werden
				remove(buffer, count, temp_count, i);
				break;
			}
			case 1:{	//Das aktuelle Dreieck kann grad geändert werden
				fvec3 dir = in_v[0];
				dir.x -= out_v[0].x; dir.y -= out_v[0].y; dir.z -= out_v[0].z;
				ray_plane_intersection(p, dir, in_v[0], buffer[i].point[1]);
				dir = in_v[0];
				dir.x -= out_v[1].x; dir.y -= out_v[1].y; dir.z -= out_v[1].z;
				ray_plane_intersection(p, dir, in_v[0], buffer[i].point[2]);
				buffer[i].point[0] = in_v[0];
				break;
			}
			case 2:{	//2 neue Dreiecke müssen hinzugefügt werden und das aktuelle entfernt
				remove(buffer, count, temp_count, i);
				fvec3 dir = in_v[0];
				dir.x -= out_v[0].x; dir.y -= out_v[0].y; dir.z -= out_v[0].z;
				ray_plane_intersection(p, dir, in_v[0], buffer[tmp_off].point[2]);
				buffer[tmp_off].point[0] = in_v[0];
				buffer[tmp_off].point[1] = in_v[1];
				dir = in_v[1];
				dir.x -= out_v[0].x; dir.y -= out_v[0].y; dir.z -= out_v[0].z;
				ray_plane_intersection(p, dir, in_v[1], buffer[tmp_off+1].point[2]);
				buffer[tmp_off+1].point[0] = buffer[tmp_off].point[2];
				buffer[tmp_off+1].point[1] = in_v[1];
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

inline byte clipping(triangle* buffer){
	byte count = 1;
	float aspect_ratio = window_width/window_height;

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

	plane pz = {}; pz.normal = {0, 0, 1}; pz.pos = {0, 0, 0};
	normalize(pz.normal);
	clip_plane(pz, buffer, count);

	return count;
}

struct camera{
	float focal_length;
	fvec3 pos;
	fvec2 rot;	//Yaw, pitch. rot.x ist die Rotation um die Y-Achse weil... uhh ja
};

inline constexpr uint color_picker(uint i){
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

inline void rasterize(triangle* tris, uint triangle_count, camera& cam){
#ifdef PERFORMANCE_ANALYZER
	perfAnalyzer.start_timer(0);
#endif
	float rotm[3][3];
	float aspect_ratio = window_width/window_height;
	float sin_rotx = sin(cam.rot.x);
	float cos_rotx = cos(cam.rot.x);
	float sin_roty = sin(cam.rot.y);
	float cos_roty = cos(cam.rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty; 	rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty; 	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;

#ifdef STATS
    std::cout << "Dreiecke vor Löschen: " << w.count << std::endl;
#endif
    for(uint i=0; i < triangle_count; ++i){
    	triangle tri = tris[i];
    	for(int j=0; j < 3; ++j){
    		float d[3];
    		d[0] = (tri.point[j].x-cam.pos.x);
    		d[1] = (tri.point[j].y-cam.pos.y);
    		d[2] = (tri.point[j].z-cam.pos.z);
    		float v[3]{0};
    	    for (uint a=0; a < 3; a++){
    	        for (uint b=0; b < 3; b++){
    	        	v[a] += (rotm[a][b]*d[b]);
    	        }
    	    }
    	    tri.point[j].x = v[0];
    	    tri.point[j].y = v[1];
    	    tri.point[j].z = v[2];
    	}
    	triangle buffer[32];
    	buffer[0] = tri;
    	byte count = clipping(buffer);
    	for(byte j=0; j < count; ++j){
    		fvec3 pt1 = buffer[j].point[0]; fvec3 pt2 = buffer[j].point[1]; fvec3 pt3 = buffer[j].point[2];
    		buffer[j].point[0].x = pt1.x*(cam.focal_length/pt1.z)/aspect_ratio; buffer[j].point[0].y = pt1.y*(cam.focal_length/pt1.z);
    		buffer[j].point[1].x = pt2.x*(cam.focal_length/pt2.z)/aspect_ratio; buffer[j].point[1].y = pt2.y*(cam.focal_length/pt2.z);
    		buffer[j].point[2].x = pt3.x*(cam.focal_length/pt3.z)/aspect_ratio; buffer[j].point[2].y = pt3.y*(cam.focal_length/pt3.z);
    		buffer[j].point[0].z = pt1.z; buffer[j].point[1].z = pt2.z; buffer[j].point[2].z = pt3.z;
    		draw_triangle(buffer[j]);
    	}
    }
#ifdef PERFORMANCE_ANALYZER
    perfAnalyzer.record_data(0);
#endif
    return;
}
