#pragma once

#include <fstream>
#include <math.h>
#include <chrono>
#include <iostream>

typedef unsigned int uint;
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
};

//Error-Codes
enum ErrCode{
	SUCCESS = 0, BAD_ALLOC
};
inline constexpr void ErrCheck(ErrCode code, const char* msg="\0"){
	switch(code){
	case BAD_ALLOC:{
		std::cerr << "[BAD_ALLOC ERROR]" << msg << std::endl;
		break;
	}
	default: break;
	}
}

struct Mouse{
	ivec2 pos;
	char button=0;	//Bits: LMB, RMB, Rest ungenutzt
}; GLOBALVAR static Mouse _mouse;
enum{
	MOUSE_LMB=1, MOUSE_RMB=2
};
inline constexpr bool LMB(Mouse& m){return m.button&MOUSE_LMB;}
inline constexpr bool RMB(Mouse& m){return m.button&MOUSE_RMB;}

struct Keyboard{
	char button=0;	//Bits: W, A, S, D, SHIFT, SPACE, ESC, ungenutzt
}; GLOBALVAR static Keyboard _keyboard;
enum{
	KEYBOARD_W=1, KEYBOARD_A=2, KEYBOARD_S=4, KEYBOARD_D=8, KEYBOARD_SHIFT=16, KEYBOARD_SPACE=32, KEYBOARD_ESC=64
};
inline constexpr bool W(Keyboard& k){return k.button&KEYBOARD_W;}
inline constexpr bool A(Keyboard& k){return k.button&KEYBOARD_A;}
inline constexpr bool S(Keyboard& k){return k.button&KEYBOARD_S;}
inline constexpr bool D(Keyboard& k){return k.button&KEYBOARD_D;}
inline constexpr bool SHIFT(Keyboard& k){return k.button&KEYBOARD_SHIFT;}
inline constexpr bool SPACE(Keyboard& k){return k.button&KEYBOARD_SPACE;}
inline constexpr bool ESC(Keyboard& k){return k.button&KEYBOARD_ESC;}

inline constexpr void normalize(fvec3& vec){
	float length = sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
	vec.x /= length; vec.y /= length; vec.z /= length;
	return;
}
inline constexpr void normalize(fvec2& vec){
	float length = sqrt(vec.x*vec.x + vec.y*vec.y);
	vec.x /= length; vec.y /= length;
	return;
}
inline constexpr float dot(fvec3& a, fvec3& b){return (a.x * b.x + a.y * b.y + a.z * b.z);}
inline constexpr float dot(fvec2& a, fvec2& b){return (a.x * b.x + a.y * b.y);}
inline constexpr float cross(fvec2& a, fvec2& b){return (a.x * b.y - a.y * b.x);}
inline constexpr fvec3 cross(fvec3& a, fvec3& b){return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};}

inline constexpr float deg2rad(float deg){return deg*PI/180;}
inline constexpr float rad2deg(float rad){return rad*180/PI;}

inline constexpr fvec3 rotate(const fvec3& v, const fvec3& axis, float angle){
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0f - c;

    fvec3 result = {v.x * (oc * axis.x * axis.x + c) +
               	   v.y * (oc * axis.x * axis.y - axis.z * s) +
				   v.z * (oc * axis.x * axis.z + axis.y * s),
				   v.x * (oc * axis.y * axis.x + axis.z * s) +
				   v.y * (oc * axis.y * axis.y + c) +
				   v.z * (oc * axis.y * axis.z - axis.x * s),
				   v.x * (oc * axis.z * axis.x - axis.y * s) +
				   v.y * (oc * axis.z * axis.y + axis.x * s) +
				   v.z * (oc * axis.z * axis.z + c)
    };

    return result;
}

#define PERFORMANCE_ANALYZER
struct PerfAnalyzer{
	//Indexe: 0 rasterizer, 1 drawing, 2 ungenutzt
	float data[24] = {};
	uchar counter[2] = {};
	uint total_triangles = 0;
	uint drawn_triangles = 0;
	std::chrono::high_resolution_clock::time_point tp[2];
	void start_timer(uchar idx){tp[idx] = std::chrono::high_resolution_clock::now();}
	float stop_timer(uchar idx){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()-tp[idx]).count();}
	void reset(){
		total_triangles = 0;
		drawn_triangles = 0;
	}
	void record_data(uchar idx){
		float ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()-tp[idx]).count();
		data[counter[idx]/32+idx*8] = ms;
		counter[idx] += 32;
	}
	void record_data_no_inc(uchar idx){
		float ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()-tp[idx]).count();
		data[counter[idx]/32+idx*8] += ms;
	}
	float get_avg_data(uchar idx){
		float out = 0;
		for(uchar i=0; i < 8; ++i){
			out += data[i+8*idx];
		}
		return out/8.;
	}
}; GLOBALVAR static PerfAnalyzer _perfAnalyzer;

//TODO man kann aktuell nur objs die nur aus dreiecken bestehen lesen, sollte aber flächen aus n punkten lesen können
//TODO Auf Fehler testen
int read_obj(const char* filename, triangle* storage, uint* count, float x, float y, float z, float scale=1){
	std::fstream file; file.open(filename, std::ios::in);
	if(!file.is_open()) throw std::runtime_error("Konnte Datei nicht öffnen!");
	std::string word;
	std::vector<fvec3> points;
	std::vector<fvec3> normals;
	std::vector<fvec2> uvs;
	uint current_count = *count;
	uint tri_count = 0;
	while(file >> word){	//Lese Datei Wort für Wort
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

	tri[count+1].point[0].x = x;
	tri[count+1].point[0].y = y;
	tri[count+1].point[0].z = z;
	tri[count+1].point[1].x = x+dx;
	tri[count+1].point[1].y = y;
	tri[count+1].point[1].z = z;
	tri[count+1].point[2].x = x;
	tri[count+1].point[2].y = y+dy;
	tri[count+1].point[2].z = z;
	tri[count+1].uv[0] = {0, 0};
	tri[count+1].uv[1] = {0, 1};
	tri[count+1].uv[2] = {1, 0};

	tri[count+2].point[0].x = x;
	tri[count+2].point[0].y = y;
	tri[count+2].point[0].z = z;
	tri[count+2].point[1].x = x;
	tri[count+2].point[1].y = y+dy;
	tri[count+2].point[1].z = z;
	tri[count+2].point[2].x = x;
	tri[count+2].point[2].y = y;
	tri[count+2].point[2].z = z+dz;
	tri[count+2].uv[0] = {0, 1};
	tri[count+2].uv[1] = {1, 1};
	tri[count+2].uv[2] = {0, 0};

	tri[count+3].point[0].x = x;
	tri[count+3].point[0].y = y+dy;
	tri[count+3].point[0].z = z+dz;
	tri[count+3].point[1].x = x;
	tri[count+3].point[1].y = y;
	tri[count+3].point[1].z = z+dz;
	tri[count+3].point[2].x = x;
	tri[count+3].point[2].y = y+dy;
	tri[count+3].point[2].z = z;
	tri[count+3].uv[0] = {1, 0};
	tri[count+3].uv[1] = {0, 0};
	tri[count+3].uv[2] = {1, 1};

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

	tri[count+6].point[0].x = x;
	tri[count+6].point[0].y = y;
	tri[count+6].point[0].z = z+dz;
	tri[count+6].point[1].x = x;
	tri[count+6].point[1].y = y+dy;
	tri[count+6].point[1].z = z+dz;
	tri[count+6].point[2].x = x+dx;
	tri[count+6].point[2].y = y;
	tri[count+6].point[2].z = z+dz;
	tri[count+6].uv[0] = {0, 0};
	tri[count+6].uv[1] = {1, 0};
	tri[count+6].uv[2] = {0, 1};

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

	tri[count+8].point[0].x = x;
	tri[count+8].point[0].y = y+dy;
	tri[count+8].point[0].z = z;
	tri[count+8].point[1].x = x+dx;
	tri[count+8].point[1].y = y+dy;
	tri[count+8].point[1].z = z;
	tri[count+8].point[2].x = x;
	tri[count+8].point[2].y = y+dy;
	tri[count+8].point[2].z = z+dz;
	tri[count+8].uv[0] = {0, 0};
	tri[count+8].uv[1] = {1, 0};
	tri[count+8].uv[2] = {0, 1};

	tri[count+9].point[0].x = x+dx;
	tri[count+9].point[0].y = y+dy;
	tri[count+9].point[0].z = z+dz;
	tri[count+9].point[1].x = x;
	tri[count+9].point[1].y = y+dy;
	tri[count+9].point[1].z = z+dz;
	tri[count+9].point[2].x = x+dx;
	tri[count+9].point[2].y = y+dy;
	tri[count+9].point[2].z = z;
	tri[count+9].uv[0] = {1, 1};
	tri[count+9].uv[1] = {0, 1};
	tri[count+9].uv[2] = {1, 0};

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
