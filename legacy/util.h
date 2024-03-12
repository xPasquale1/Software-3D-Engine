#pragma once

#include <fstream>
#include <math.h>
#include <chrono>
#include <iostream>

typedef unsigned long uint;
typedef unsigned char uchar;

#define PI 3.14159265359

#define GLOBALVAR

struct ivec2{
	int x;
	int y;
};

struct ivec3{
	int x;
	int y;
	int z;
};

struct fvec2{
	float x;
	float y;
};

struct fvec3{
	float x;
	float y;
	float z;
};

struct triangle{
	fvec3 point[3];
	fvec2 uv[3];
//	fvec3 normal;
	uint colors[3];	//TODO entfernen?
};

//Error-Codes
enum ErrCode{
	SUCCESS = 0,
	BAD_ALLOC,
	TEXTURE_NOT_FOUND,
	MODEL_NOT_FOUND,
	FILE_NOT_FOUND
};
enum ErrCodeFlags{
	NO_ERR_FLAG = 0,
	NO_ERR_OUTPUT,
	ERR_ON_FATAL
};
inline constexpr int ErrCheck(ErrCode code, const char* msg="\0", ErrCodeFlags flags=NO_ERR_FLAG){
	switch(code){
	case BAD_ALLOC:
		if(!(flags&NO_ERR_OUTPUT)) std::cerr << "[BAD_ALLOC ERROR]" << msg << std::endl;
		return ERR_ON_FATAL;
	case TEXTURE_NOT_FOUND:
		if(!(flags&NO_ERR_OUTPUT)) std::cerr << "[TEXTURE_NOT_FOUND ERROR]" << msg << std::endl;
		return 0;
	case MODEL_NOT_FOUND:
		if(!(flags&NO_ERR_OUTPUT)) std::cerr << "[MODEL_NOT_FOUND ERROR]" << msg << std::endl;
		return 0;
	case FILE_NOT_FOUND:
		if(!(flags&NO_ERR_OUTPUT)) std::cerr << "[FILE_NOT_FOUND ERROR]" << msg << std::endl;
		return 0;
	default: return 0;
	}
	return 0;
}

struct Mouse{
	ivec2 pos;
	uint button=0;	//Bits: LMB, RMB, Rest ungenutzt
}; GLOBALVAR static Mouse _mouse;
enum MOUSEBUTTON{
	MOUSE_LMB=1,
	MOUSE_RMB=2
};
inline constexpr bool LMB(Mouse& m){return m.button&MOUSE_LMB;}
inline constexpr bool RMB(Mouse& m){return m.button&MOUSE_RMB;}

//setGlobalMousebutton, resetGlobalMousebutton, getGlobalMousebutton
inline void setgMouse(MOUSEBUTTON button){_mouse.button |= button;}
inline void resetgMouse(MOUSEBUTTON button){_mouse.button &= ~button;}
inline bool getgMouse(MOUSEBUTTON button){return _mouse.button & button;}
//setMousebutton, resetMousebutton, getMousebutton
inline constexpr void setMouse(Mouse& mouse, MOUSEBUTTON button){mouse.button |= button;}
inline constexpr void resetMouse(Mouse& mouse, MOUSEBUTTON button){mouse.button &= ~button;}
inline constexpr bool getMouse(Mouse& mouse, MOUSEBUTTON button){return mouse.button & button;}

struct Keyboard{
	uint button=0;	//Bits: W, A, S, D, SHIFT, SPACE, ESC, rest ungenutzt
}; GLOBALVAR static Keyboard _keyboard;
enum KEYBOARDBUTTON{
	KEY_W=1,
	KEY_A=2,
	KEY_S=4,
	KEY_D=8,
	KEY_SHIFT=16,
	KEY_SPACE=32,
	KEY_ESC=64
};

//setGlobalKey, resetGlobalKey, getGlobalKey
inline void setgKey(KEYBOARDBUTTON button){_keyboard.button |= button;}
inline void resetgKey(KEYBOARDBUTTON button){_keyboard.button &= ~button;}
inline bool getgKey(KEYBOARDBUTTON button){return _keyboard.button & button;}
//setKey, resetKey, getKey
inline constexpr void setKey(Keyboard& keyboard, KEYBOARDBUTTON button){keyboard.button |= button;}
inline constexpr void resetKey(Keyboard& keyboard, KEYBOARDBUTTON button){keyboard.button &= ~button;}
inline constexpr bool getKey(Keyboard& keyboard, KEYBOARDBUTTON button){return keyboard.button & button;}

inline constexpr void normalize(fvec3& vec)noexcept{
	float length = sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
	vec.x /= length; vec.y /= length; vec.z /= length;
}
inline constexpr void normalize(fvec2& vec)noexcept{
	float length = sqrt(vec.x*vec.x + vec.y*vec.y);
	vec.x /= length; vec.y /= length;
}
inline constexpr float dot(fvec3& a, fvec3& b)noexcept{return (a.x * b.x + a.y * b.y + a.z * b.z);}
inline constexpr float dot(fvec2& a, fvec2& b)noexcept{return (a.x * b.x + a.y * b.y);}
inline constexpr float cross(fvec2& a, fvec2& b)noexcept{return (a.x * b.y - a.y * b.x);}
inline constexpr fvec3 cross(fvec3& a, fvec3& b)noexcept{return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};}

inline constexpr float deg2rad(float deg)noexcept{return deg*PI/180;}
inline constexpr float rad2deg(float rad)noexcept{return rad*180/PI;}

inline constexpr float min(float a, float b)noexcept{
//	return a*(a<b)+b*(b<=a);
	return a < b ? a : b;
}
inline constexpr float max(float a, float b)noexcept{
//	return a*(a>b)+b*(b>=a);
	return a > b ? a : b;
}

inline constexpr float clamp(float val, float minVal, float maxVal){
	return max(min(val, maxVal), minVal);
}

//Interpoliere die Werte A und B linear für einen Punkt C
inline constexpr float interpolateLinear(float valA, float valB, fvec2 posA, fvec2 posB, fvec2 posC){
	float valDiff = valB-valA;
	return valA + (valDiff*(posC.x-posA.x)+valDiff*(posC.y-posA.y))/((posB.x-posA.x)+(posB.y-posA.y));
}

#define PERFORMANCE_ANALYZER
#define PERFORMANCE_ANALYZER_DATA_POINTS 3
struct PerfAnalyzer{
	//Indexe: 0 rasterizer, 1 drawing, 2 ungenutzt
	float data[PERFORMANCE_ANALYZER_DATA_POINTS*8] = {};
	uchar counter[PERFORMANCE_ANALYZER_DATA_POINTS-1] = {};
	uint total_triangles = 0;
	uint drawn_triangles = 0;
	std::chrono::high_resolution_clock::time_point tp[2];
}; GLOBALVAR static PerfAnalyzer _perfAnalyzer;

void start_timer(PerfAnalyzer& pa, uchar idx){pa.tp[idx] = std::chrono::high_resolution_clock::now();}
float stop_timer(PerfAnalyzer& pa, uchar idx){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()-pa.tp[idx]).count();}
void reset(PerfAnalyzer& pa){
	pa.total_triangles = 0;
	pa.drawn_triangles = 0;
}
void record_data(PerfAnalyzer& pa, uchar idx){
	float ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()-pa.tp[idx]).count();
	pa.data[pa.counter[idx]/32+idx*8] = ms;
	pa.counter[idx] += 32;
}
void record_data_no_inc(PerfAnalyzer& pa, uchar idx){
	float ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()-pa.tp[idx]).count();
	pa.data[pa.counter[idx]/32+idx*8] += ms;
}
float get_avg_data(PerfAnalyzer& pa, uchar idx){
	float out = 0;
	for(uchar i=0; i < 8; ++i){
		out += pa.data[i+8*idx];
	}
	return out/8.;
}

//TODO man kann aktuell nur objs die nur aus dreiecken bestehen lesen, sollte aber fl�chen aus n punkten lesen k�nnen
//TODO Auf Fehler testen
ErrCode read_obj(const char* filename, triangle* storage, uint* count, float x, float y, float z, float scale=1){
	std::fstream file; file.open(filename, std::ios::in);
	if(!file.is_open()) throw std::runtime_error("Konnte Datei nicht öffnen!");
	std::string word;
	std::vector<fvec3> points;
	std::vector<fvec3> normals;
	std::vector<fvec2> uvs;
	uint current_count = *count;
	uint tri_count = 0;
	while(file >> word){	//Lese Datei Wort f�r Wort
		if(word[0] == 'o' && word.size() == 1){
			file >> word;
			std::cout << "Filename:               " << word << std::endl;
		}
		if(word[0] == 'v' && word.size() == 1){
			file >> word;
			fvec3 point;
			point.x = (std::atof(word.c_str())+x)*scale;
			file >> word;
			point.y = (std::atof(word.c_str())+y)*scale;
			file >> word;
			point.z = (std::atof(word.c_str())+z)*scale;
			points.push_back(point);
		}
		if(word[0] == 'v' && word[1] == 'n' && word.size() == 2){
			file >> word;
			fvec3 normal;
			normal.x = std::atof(word.c_str());
			file >> word;
			normal.y = std::atof(word.c_str());
			file >> word;
			normal.z = std::atof(word.c_str());
			normals.push_back(normal);
		}
		if(word[0] == 'v' && word[1] == 't' && word.size() == 2){
			file >> word;
			fvec2 uv;
			uv.x = std::atof(word.c_str());
			file >> word;
			uv.y = std::atof(word.c_str());
			uvs.push_back(uv);
		}
		if(word[0] == 'f' && word.size() == 1){
			//Lese nur die Eckpunkte
			uint pt_order[4];
			uint uv_order[4];
			uint c = 0;
			uint c1 = 0;
			std::string val;

			//Lese Punkt/Texture Koordinate
			file >> word; c = 0;
			//Suche alle Zahlen die den Punkt beschreiben
			for(size_t i=0; i < word.size(); ++i){
				if(word[i] >= '0' && word[i] <= '9') ++c;
				else break;
			} val.clear();
			for(uint i=0; i < c; ++i){
				val += word[i];
			} pt_order[0] = std::atoi(val.c_str())-1;
			//Suche alle Zahlen die die Texturkoordinaten bestimmen
			c1 = c+1; c = 0;
			for(size_t i=c1; i < word.size(); ++i){
				if(word[i] >= '0' && word[i] <= '9') ++c;
				else break;
			} val.clear();
			for(uint i=c1; i < c+c1; ++i){
				val += word[i];
			} uv_order[0] = std::atoi(val.c_str())-1;

			//Lese Punkt/Texture Koordinate
			file >> word; c = 0;
			//Suche alle Zahlen die den Punkt beschreiben
			for(size_t i=0; i < word.size(); ++i){
				if(word[i] >= '0' && word[i] <= '9') ++c;
				else break;
			} val.clear();
			for(uint i=0; i < c; ++i){
				val += word[i];
			} pt_order[1] = std::atoi(val.c_str())-1;
			//Suche alle Zahlen die die Texturkoordinaten bestimmen
			c1 = c+1; c = 0;
			for(size_t i=c1; i < word.size(); ++i){
				if(word[i] >= '0' && word[i] <= '9') ++c;
				else break;
			} val.clear();
			for(uint i=c1; i < c+c1; ++i){
				val += word[i];
			} uv_order[1] = std::atoi(val.c_str())-1;

			//Lese Punkt/Texture Koordinate
			file >> word; c = 0;
			//Suche alle Zahlen die den Punkt beschreiben
			for(size_t i=0; i < word.size(); ++i){
				if(word[i] >= '0' && word[i] <= '9') ++c;
				else break;
			} val.clear();
			for(uint i=0; i < c; ++i){
				val += word[i];
			} pt_order[2] = std::atoi(val.c_str())-1;
			//Suche alle Zahlen die die Texturkoordinaten bestimmen
			c1 = c+1; c = 0;
			for(size_t i=c1; i < word.size(); ++i){
				if(word[i] >= '0' && word[i] <= '9') ++c;
				else break;
			} val.clear();
			for(uint i=c1; i < c+c1; ++i){
				val += word[i];
			} uv_order[2] = std::atoi(val.c_str())-1;

			storage[tri_count+current_count].point[0] = points[pt_order[0]];
			storage[tri_count+current_count].point[1] = points[pt_order[1]];
			storage[tri_count+current_count].point[2] = points[pt_order[2]];
			storage[tri_count+current_count].uv[0] = uvs[uv_order[0]];
			storage[tri_count+current_count].uv[1] = uvs[uv_order[1]];
			storage[tri_count+current_count].uv[2] = uvs[uv_order[2]];
//			float x = (normals[pt_order[0]].x + normals[pt_order[1]].x + normals[pt_order[2]].x)/3.;
//			float y = (normals[pt_order[0]].y + normals[pt_order[1]].y + normals[pt_order[2]].y)/3.;
//			float z = (normals[pt_order[0]].z + normals[pt_order[1]].x + normals[pt_order[2]].z)/3.;
//			storage[tri_count+current_count].normal = {x, y, z};
			++tri_count;
		}
	}
	*count += tri_count;
	std::cout << "Punkte gelesen:         " << points.size() << std::endl;
	std::cout << "UV-Koordinaten gelesen: " << uvs.size() << std::endl;
	std::cout << "Dreiecke gelesen:       " << tri_count << std::endl;
	std::cout << "Dreiecke insgesamt:     " << *count << '\n' << std::endl;
	return SUCCESS;
}

inline void create_cube(triangle* tri, uint& count, float x, float y, float z, float dx, float dy, float dz){
	tri[count].point[0].x = x+dx;
	tri[count].point[0].y = y+dy;
	tri[count].point[0].z = z;
	tri[count].point[1].x = x+dx;
	tri[count].point[1].y = y;
	tri[count].point[1].z = z;
	tri[count].point[2].x = x;
	tri[count].point[2].y = y+dy;
	tri[count].point[2].z = z;
	tri[count].uv[0] = {1, 1};
	tri[count].uv[1] = {0, 1};
	tri[count].uv[2] = {1, 0};

	tri[count+1].point[2].x = x;
	tri[count+1].point[2].y = y;
	tri[count+1].point[2].z = z;
	tri[count+1].point[1].x = x+dx;
	tri[count+1].point[1].y = y;
	tri[count+1].point[1].z = z;
	tri[count+1].point[0].x = x;
	tri[count+1].point[0].y = y+dy;
	tri[count+1].point[0].z = z;
	tri[count+1].uv[2] = {0, 0};
	tri[count+1].uv[1] = {0, 1};
	tri[count+1].uv[0] = {1, 0};

	tri[count+2].point[2].x = x;
	tri[count+2].point[2].y = y;
	tri[count+2].point[2].z = z;
	tri[count+2].point[1].x = x;
	tri[count+2].point[1].y = y+dy;
	tri[count+2].point[1].z = z;
	tri[count+2].point[0].x = x;
	tri[count+2].point[0].y = y;
	tri[count+2].point[0].z = z+dz;
	tri[count+2].uv[2] = {0, 1};
	tri[count+2].uv[1] = {1, 1};
	tri[count+2].uv[0] = {0, 0};

	tri[count+3].point[2].x = x;
	tri[count+3].point[2].y = y+dy;
	tri[count+3].point[2].z = z+dz;
	tri[count+3].point[1].x = x;
	tri[count+3].point[1].y = y;
	tri[count+3].point[1].z = z+dz;
	tri[count+3].point[0].x = x;
	tri[count+3].point[0].y = y+dy;
	tri[count+3].point[0].z = z;
	tri[count+3].uv[2] = {1, 0};
	tri[count+3].uv[1] = {0, 0};
	tri[count+3].uv[0] = {1, 1};

	tri[count+4].point[0].x = x+dx;
	tri[count+4].point[0].y = y;
	tri[count+4].point[0].z = z;
	tri[count+4].point[1].x = x+dx;
	tri[count+4].point[1].y = y+dy;
	tri[count+4].point[1].z = z;
	tri[count+4].point[2].x = x+dx;
	tri[count+4].point[2].y = y;
	tri[count+4].point[2].z = z+dz;
	tri[count+4].uv[0] = {0, 1};
	tri[count+4].uv[1] = {1, 1};
	tri[count+4].uv[2] = {0, 0};

	tri[count+5].point[0].x = x+dx;
	tri[count+5].point[0].y = y+dy;
	tri[count+5].point[0].z = z+dz;
	tri[count+5].point[1].x = x+dx;
	tri[count+5].point[1].y = y;
	tri[count+5].point[1].z = z+dz;
	tri[count+5].point[2].x = x+dx;
	tri[count+5].point[2].y = y+dy;
	tri[count+5].point[2].z = z;
	tri[count+5].uv[0] = {1, 0};
	tri[count+5].uv[1] = {0, 0};
	tri[count+5].uv[2] = {1, 1};

	tri[count+6].point[2].x = x;
	tri[count+6].point[2].y = y;
	tri[count+6].point[2].z = z+dz;
	tri[count+6].point[1].x = x;
	tri[count+6].point[1].y = y+dy;
	tri[count+6].point[1].z = z+dz;
	tri[count+6].point[0].x = x+dx;
	tri[count+6].point[0].y = y;
	tri[count+6].point[0].z = z+dz;
	tri[count+6].uv[2] = {0, 0};
	tri[count+6].uv[1] = {1, 0};
	tri[count+6].uv[0] = {0, 1};

	tri[count+7].point[0].x = x+dx;
	tri[count+7].point[0].y = y+dy;
	tri[count+7].point[0].z = z+dz;
	tri[count+7].point[1].x = x;
	tri[count+7].point[1].y = y+dy;
	tri[count+7].point[1].z = z+dz;
	tri[count+7].point[2].x = x+dx;
	tri[count+7].point[2].y = y;
	tri[count+7].point[2].z = z+dz;
	tri[count+7].uv[0] = {1, 1};
	tri[count+7].uv[1] = {1, 0};
	tri[count+7].uv[2] = {0, 1};

	tri[count+8].point[2].x = x;
	tri[count+8].point[2].y = y+dy;
	tri[count+8].point[2].z = z;
	tri[count+8].point[1].x = x+dx;
	tri[count+8].point[1].y = y+dy;
	tri[count+8].point[1].z = z;
	tri[count+8].point[0].x = x;
	tri[count+8].point[0].y = y+dy;
	tri[count+8].point[0].z = z+dz;
	tri[count+8].uv[2] = {0, 0};
	tri[count+8].uv[1] = {1, 0};
	tri[count+8].uv[0] = {0, 1};

	tri[count+9].point[2].x = x+dx;
	tri[count+9].point[2].y = y+dy;
	tri[count+9].point[2].z = z+dz;
	tri[count+9].point[1].x = x;
	tri[count+9].point[1].y = y+dy;
	tri[count+9].point[1].z = z+dz;
	tri[count+9].point[0].x = x+dx;
	tri[count+9].point[0].y = y+dy;
	tri[count+9].point[0].z = z;
	tri[count+9].uv[2] = {1, 1};
	tri[count+9].uv[1] = {0, 1};
	tri[count+9].uv[0] = {1, 0};

	tri[count+10].point[0].x = x;
	tri[count+10].point[0].y = y;
	tri[count+10].point[0].z = z;
	tri[count+10].point[1].x = x+dx;
	tri[count+10].point[1].y = y;
	tri[count+10].point[1].z = z;
	tri[count+10].point[2].x = x;
	tri[count+10].point[2].y = y;
	tri[count+10].point[2].z = z+dz;
	tri[count+10].uv[0] = {0, 0};
	tri[count+10].uv[1] = {1, 0};
	tri[count+10].uv[2] = {0, 1};

	tri[count+11].point[0].x = x+dx;
	tri[count+11].point[0].y = y;
	tri[count+11].point[0].z = z+dz;
	tri[count+11].point[1].x = x;
	tri[count+11].point[1].y = y;
	tri[count+11].point[1].z = z+dz;
	tri[count+11].point[2].x = x+dx;
	tri[count+11].point[2].y = y;
	tri[count+11].point[2].z = z;
	tri[count+11].uv[0] = {1, 1};
	tri[count+11].uv[1] = {0, 1};
	tri[count+11].uv[2] = {1, 0};
	count += 12;
	return;
}
