#pragma once

#include <fstream>
#include "window.h"
#include "util.h"

static uint* default_texture = nullptr;

inline constexpr uint RGBA(uchar, uchar, uchar, uchar);	//meh...
int load_texture(const char* name, uint*& _texture){
	std::fstream file; file.open(name, std::ios::in);
	if(!file.is_open()) throw std::runtime_error("Konnte Texture-Datei nicht öffnen!");
	//Lese Breite und Höhe
	std::string word;
	file >> word;
	uint width = std::atoi(word.c_str());
	file >> word;
	uint height = std::atoi(word.c_str());
	delete[] _texture;
	_texture = new(std::nothrow) uint[width*height+2];	//+2 für Breite und Höhe
	if(!_texture){
		std::cerr << "Konnte keinen Speicher für die Texture allokieren!" << std::endl;
		return BAD_ALLOC;
	}
	_texture[0] = width; _texture[1] = height;
	for(uint i=2; i < width*height+2; ++i){
		file >> word;
		uchar r = std::atoi(word.c_str());
		file >> word;
		uchar g = std::atoi(word.c_str());
		file >> word;
		uchar b = std::atoi(word.c_str());
		file >> word;
		uchar a = std::atoi(word.c_str());
		_texture[i] = RGBA(r, g, b, a);
	}
	return SUCCESS;
}
