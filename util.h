#pragma once

#include <fstream>
#include <math.h>

typedef unsigned int uint;
typedef unsigned char uchar;

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
	uint color;
};

//TODO Könnte besser sein setter und getter Funktionen für maus und tastatur zu schreiben
struct Mouse{
	ivec2 pos;
	char button;	//Bits: LMB, RMB, Rest ungenutzt
}; static Mouse mouse;
inline constexpr bool LMB(Mouse& m){return m.button&0b1000'0000;}
inline constexpr bool RMB(Mouse& m){return m.button&0b0100'0000;}

struct Keyboard{
	char button;	//Bits: W, A, S, D, SHIFT, SPACE, Rest ungenutzt
}; static Keyboard keyboard;
inline constexpr bool W(Keyboard& k){return k.button&0b1000'0000;}
inline constexpr bool A(Keyboard& k){return k.button&0b0100'0000;}
inline constexpr bool S(Keyboard& k){return k.button&0b0010'0000;}
inline constexpr bool D(Keyboard& k){return k.button&0b0001'0000;}
inline constexpr bool SHIFT(Keyboard& k){return k.button&0b0000'1000;}
inline constexpr bool SPACE(Keyboard& k){return k.button&0b0000'0100;}

void normalize(fvec3& vec){
	float length = sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
	vec.x /= length; vec.y /= length; vec.z /= length;
	return;
}

inline constexpr float dot(fvec3& a, fvec3& b){
	return (a.x * b.x + a.y * b.y + a.z * b.z);
}

inline constexpr float dot(fvec2& a, fvec2& b){
	return (a.x * b.x + a.y * b.y);
}

inline constexpr float cross(fvec2& a, fvec2& b){
	return (a.x * b.y - a.y * b.x);
}

struct AvgFrametime{
	float ms[8];
	float avg_ms;
	uchar time_point = 0;
	void add_time(float ms){
		this->ms[time_point/32] = ms;
		time_point += 32;
		float avg = 0;
		for(uint i=0; i < 8; ++i){
			avg += this->ms[i];
		}
		avg_ms = avg/8.;
	}
};

inline void create_cube(triangle* tri, unsigned int& count, float x, float y, float z, float dx, float dy, float dz){
	tri[count].point[0].x = x;
	tri[count].point[0].y = y;
	tri[count].point[0].z = z;
	tri[count].point[1].x = x+dx;
	tri[count].point[1].y = y;
	tri[count].point[1].z = z;
	tri[count].point[2].x = x;
	tri[count].point[2].y = y+dy;
	tri[count].point[2].z = z;

	tri[count+1].point[0].x = x;
	tri[count+1].point[0].y = y+dy;
	tri[count+1].point[0].z = z;
	tri[count+1].point[1].x = x+dx;
	tri[count+1].point[1].y = y+dy;
	tri[count+1].point[1].z = z;
	tri[count+1].point[2].x = x+dx;
	tri[count+1].point[2].y = y;
	tri[count+1].point[2].z = z;

	tri[count+2].point[0].x = x;
	tri[count+2].point[0].y = y;
	tri[count+2].point[0].z = z;
	tri[count+2].point[1].x = x;
	tri[count+2].point[1].y = y+dy;
	tri[count+2].point[1].z = z;
	tri[count+2].point[2].x = x;
	tri[count+2].point[2].y = y;
	tri[count+2].point[2].z = z+dz;

	tri[count+3].point[0].x = x;
	tri[count+3].point[0].y = y;
	tri[count+3].point[0].z = z+dz;
	tri[count+3].point[1].x = x;
	tri[count+3].point[1].y = y+dy;
	tri[count+3].point[1].z = z+dz;
	tri[count+3].point[2].x = x;
	tri[count+3].point[2].y = y+dy;
	tri[count+3].point[2].z = z;

	tri[count+4].point[0].x = x+dx;
	tri[count+4].point[0].y = y;
	tri[count+4].point[0].z = z;
	tri[count+4].point[1].x = x+dx;
	tri[count+4].point[1].y = y+dy;
	tri[count+4].point[1].z = z;
	tri[count+4].point[2].x = x+dx;
	tri[count+4].point[2].y = y;
	tri[count+4].point[2].z = z+dz;

	tri[count+5].point[0].x = x+dx;
	tri[count+5].point[0].y = y+dy;
	tri[count+5].point[0].z = z;
	tri[count+5].point[1].x = x+dx;
	tri[count+5].point[1].y = y;
	tri[count+5].point[1].z = z+dz;
	tri[count+5].point[2].x = x+dx;
	tri[count+5].point[2].y = y+dy;
	tri[count+5].point[2].z = z+dz;

	tri[count+6].point[0].x = x;
	tri[count+6].point[0].y = y;
	tri[count+6].point[0].z = z+dz;
	tri[count+6].point[1].x = x;
	tri[count+6].point[1].y = y+dy;
	tri[count+6].point[1].z = z+dz;
	tri[count+6].point[2].x = x+dx;
	tri[count+6].point[2].y = y;
	tri[count+6].point[2].z = z+dz;

	tri[count+7].point[0].x = x;
	tri[count+7].point[0].y = y+dy;
	tri[count+7].point[0].z = z+dz;
	tri[count+7].point[1].x = x+dx;
	tri[count+7].point[1].y = y+dy;
	tri[count+7].point[1].z = z+dz;
	tri[count+7].point[2].x = x+dx;
	tri[count+7].point[2].y = y;
	tri[count+7].point[2].z = z+dz;
	count += 8;
	return;
}

void read_obj(const char* filename, triangle* storage, unsigned int* count, float x, float y, float z){
	std::fstream file; file.open(filename, std::ios::in);
	if(!file.is_open()) throw std::runtime_error("Konnte Datei nicht öffnen!");
	std::string word;
	fvec3 points[9000];	//TODO std::list oder vectorq
	unsigned int current_count = *count;
	unsigned int p_count = 0;
//	unsigned int n_count = 0;
	unsigned int tri_count = 0;
	while(file >> word){	//Lese Datei Wort für Wort
		if(word[0] == 'o' && word.size() == 1){
			file >> word;
			std::cout << "Filename:           " << word << std::endl;
		}
		if(word[0] == 'v' && word.size() == 1){
			file >> word;
			points[p_count].x = std::atof(word.c_str())+x;
			file >> word;
			points[p_count].y = std::atof(word.c_str())+y;
			file >> word;
			points[p_count].z = std::atof(word.c_str())+z;
			++p_count;
		}
//		if(word[0] == 'v' && word[1] == 'n' && word.size() == 2){
//			file >> word;
//			points[n_count].normal.x = std::atof(word.c_str());
//			file >> word;
//			points[n_count].normal.y = std::atof(word.c_str());
//			file >> word;
//			points[n_count].normal.z = std::atof(word.c_str());
//			++n_count;
//		}
		if(word[0] == 'f' && word.size() == 1){
			//Lese nur die Eckpunkte TODO Flächengröße bestimmen
			unsigned int order[4];
			unsigned int c = 0;
			std::string val;

			file >> word; c = 0;
			for(size_t i=0; i < word.size(); ++i){
				if(word[i] >= '0' && word[i] <= '9') ++c;
				else break;
			} val.clear();
			for(unsigned int i=0; i < c; ++i){
				val += word[i];
			} order[0] = std::atoi(val.c_str())-1;

			file >> word; c = 0;
			for(size_t i=0; i < word.size(); ++i){
				if(word[i] >= '0' && word[i] <= '9') ++c;
				else break;
			} val.clear();
			for(unsigned int i=0; i < c; ++i){
				val += word[i];
			} order[1] = std::atoi(val.c_str())-1;

			file >> word; c = 0;
			for(size_t i=0; i < word.size(); ++i){
				if(word[i] >= '0' && word[i] <= '9') ++c;
				else break;
			} val.clear();
			for(unsigned int i=0; i < c; ++i){
				val += word[i];
			} order[2] = std::atoi(val.c_str())-1;

			auto read_pos = file.tellg();
			file >> word; c = 0; bool quad = true;
			if(word[0] < '0' || word[0] > '9' || file.eof()){	//Fläche ist ein Dreieck
				quad = false;
				file.seekg(read_pos, std::ios::beg);
			}

			storage[tri_count+current_count].point[0] = points[order[0]];
			storage[tri_count+current_count].point[1] = points[order[1]];
			storage[tri_count+current_count].point[2] = points[order[2]];
//			storage[tri_count+current_count].point[0].color = points[order[0]].normal;
//			storage[tri_count+current_count].point[1].color = points[order[1]].normal;
//			storage[tri_count+current_count].point[2].color = points[order[2]].normal;
			++tri_count;

			if(quad){
				for(size_t i=0; i < word.size(); ++i){
					if(word[i] >= '0' && word[i] <= '9') ++c;
					else break;
				} val.clear();
				for(unsigned int i=0; i < c; ++i){
					val += word[i];
				} order[3] = std::atoi(val.c_str())-1;
				storage[tri_count+current_count].point[0] = points[order[3]];
				storage[tri_count+current_count].point[1] = points[order[0]];
				storage[tri_count+current_count].point[2] = points[order[2]];
//				storage[tri_count+current_count].point[0].color = points[order[3]].normal;
//				storage[tri_count+current_count].point[1].color = points[order[0]].normal;
//				storage[tri_count+current_count].point[2].color = points[order[2]].normal;
				++tri_count;
			}
		}
	}
	*count += tri_count;
	std::cout << "Punkte gelesen:     " << p_count << std::endl;
	std::cout << "Dreiecke gelesen:   " << tri_count << std::endl;
	std::cout << "Dreiecke insgesamt: " << *count << '\n' << std::endl;
}
