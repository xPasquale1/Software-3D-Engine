#pragma once

#include <iostream>
#include <vector>
#include <math.h>
#include "math.h"

typedef unsigned char BYTE;			//8  Bit
typedef signed char SBYTE;			//8  Bit signed
typedef unsigned short WORD;		//16 Bit
typedef signed short SWORD;			//16 Bit signed
typedef unsigned long DWORD;		//32 Bit
typedef signed long SDWORD;			//32 Bit signed
typedef unsigned long long QWORD;	//64 Bit
typedef signed long long SQWORD;	//64 Bit signed

//TODO sollte das error zeug nicht auch in window.h?
//Error-Codes
enum ErrCode{
	SUCCESS = 0,
	GENERIC_ERROR,
	APP_INIT,
	BAD_ALLOC,
	CREATE_WINDOW,
	TEXTURE_NOT_FOUND,
	MODEL_NOT_FOUND,
	MATERIAL_NOT_FOUND,
	MODEL_BAD_FORMAT,
	MATERIAL_BAD_FORMAT,
	FILE_NOT_FOUND,
	WINDOW_NOT_FOUND,
	INIT_RENDER_TARGET
};
enum ErrCodeFlags{
	ERR_NO_FLAG = 0,
	ERR_NO_OUTPUT = 1,
	ERR_ON_FATAL = 2
};
//TODO ERR_ON_FATAL ausgeben können wenn der nutzer es so möchte
ErrCode ErrCheck(ErrCode code, const char* msg="\0", ErrCodeFlags flags=ERR_NO_FLAG)noexcept{
	switch(code){
	case BAD_ALLOC:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[BAD_ALLOC ERROR] " << msg << std::endl;
		return code;
	case GENERIC_ERROR:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[GENERIC_ERROR ERROR] " << msg << std::endl;
		return code;
	case CREATE_WINDOW:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[CREATE_WINDOW ERROR] " << msg << std::endl;
		return code;
	case TEXTURE_NOT_FOUND:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[TEXTURE_NOT_FOUND ERROR] " << msg << std::endl;
		return code;
	case MODEL_NOT_FOUND:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[MODEL_NOT_FOUND ERROR] " << msg << std::endl;
		return code;
	case MODEL_BAD_FORMAT:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[MODEL_BAD_FORMAT ERROR] " << msg << std::endl;
		return code;
	case FILE_NOT_FOUND:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[FILE_NOT_FOUND ERROR] " << msg << std::endl;
		return code;
	case WINDOW_NOT_FOUND:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[WINDOW_NOT_FOUND ERROR] " << msg << std::endl;
		return code;
	case APP_INIT:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[APP_INIT ERROR] " << msg << std::endl;
		return code;
	case INIT_RENDER_TARGET:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[INIT_RENDER_TARGET ERROR] " << msg << std::endl;
		return code;
	case MATERIAL_NOT_FOUND:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[MATERIAL_NOT_FOUND ERROR] " << msg << std::endl;
		return code;
	case MATERIAL_BAD_FORMAT:
		if(!(flags&ERR_NO_OUTPUT)) std::cerr << "[MATERIAL_BAD_FORMAT ERROR] " << msg << std::endl;
		return code;
	default: return SUCCESS;
	}
	return SUCCESS;
}

enum MOUSEBUTTON{
	MOUSE_LMB = 1,
	MOUSE_RMB = 2
};
struct Mouse{
	ivec2 pos = {};
	char button = 0;	//Bits: LMB, RMB, Rest ungenutzt
}; static Mouse mouse;

constexpr bool getButton(Mouse& mouse, MOUSEBUTTON button)noexcept{return (mouse.button & button);}
constexpr void setButton(Mouse& mouse, MOUSEBUTTON button)noexcept{mouse.button |= button;}
constexpr void resetButton(Mouse& mouse, MOUSEBUTTON button)noexcept{mouse.button &= ~button;}

constexpr  const char* stringLookUp2(long value)noexcept{
	return &"001020304050607080900111213141516171819102122232425262728292"
			"031323334353637383930414243444546474849405152535455565758595"
			"061626364656667686960717273747576777879708182838485868788898"
			"09192939495969798999"[value<<1];
}
//std::to_string ist langsam, das ist simpel und schnell
static char _dec_to_str_out[12] = "00000000000";
const char* longToString(long value)noexcept{
	char* ptr = _dec_to_str_out + 11;
	*ptr = '0';
	char c = 0;
	if(value <= 0){
		value < 0 ? c = '-' : c = '0';
		value = 0-value;
	}
	while(value >= 100){
		const char* tmp = stringLookUp2(value%100);
		ptr[0] = tmp[0];
		ptr[-1] = tmp[1];
		ptr -= 2;
		value /= 100;
	}
	while(value){
		*ptr-- = '0'+value%10;
		value /= 10;
	}
	if(c) *ptr-- = c;
	return ptr+1;
}

//value hat decimals Nachkommestellen
std::string intToString(int value, BYTE decimals=2)noexcept{
	std::string out = longToString(value);
	if(out.size() < ((size_t)decimals+1) && out[0] != '-') out.insert(0, (decimals+1)-out.size(), '0');
	else if(out.size() < ((size_t)decimals+2) && out[0] == '-') out.insert(1, (decimals+1)-(out.size()-1), '0');
	if(decimals) out.insert(out.size()-decimals, 1, '.');
	return out;
}

std::string floatToString(float value, BYTE decimals=2)noexcept{
	WORD precision = pow(10, decimals);
	long val = value * precision;
	return intToString(val, decimals);
}

enum KEYBOARDBUTTON : unsigned long long{
	KEY_0 = 0ULL,
	KEY_1 = 1ULL << 0,
	KEY_3 = 1ULL << 1,
	KEY_4 = 1ULL << 2,
	KEY_5 = 1ULL << 3,
	KEY_6 = 1ULL << 4,
	KEY_7 = 1ULL << 5,
	KEY_8 = 1ULL << 6,
	KEY_9 = 1ULL << 7,
	KEY_A = 1ULL << 8,
	KEY_B = 1ULL << 9,
	KEY_C = 1ULL << 10,
	KEY_D = 1ULL << 11,
	KEY_E = 1ULL << 12,
	KEY_F = 1ULL << 13,
	KEY_G = 1ULL << 14,
	KEY_H = 1ULL << 15,
	KEY_I = 1ULL << 16,
	KEY_J = 1ULL << 17,
	KEY_K = 1ULL << 18,
	KEY_L = 1ULL << 19,
	KEY_M = 1ULL << 20,
	KEY_N = 1ULL << 21,
	KEY_O = 1ULL << 22,
	KEY_P = 1ULL << 23,
	KEY_Q = 1ULL << 24,
	KEY_R = 1ULL << 25,
	KEY_S = 1ULL << 26,
	KEY_T = 1ULL << 27,
	KEY_U = 1ULL << 28,
	KEY_V = 1ULL << 29,
	KEY_W = 1ULL << 30,
	KEY_X = 1ULL << 31,
	KEY_Y = 1ULL << 32,
	KEY_Z = 1ULL << 33,
	KEY_SHIFT = 1ULL << 34,
	KEY_SPACE = 1ULL << 35,
	KEY_CTRL = 1ULL << 36,
	KEY_ALT = 1ULL << 37,
	KEY_ESC = 1ULL << 38,
	KEY_TAB = 1ULL << 39,
	KEY_ENTER = 1ULL << 40,
	KEY_BACK = 1ULL << 41
};
struct Keyboard{
	unsigned long long buttons;	//Bits siehe enum oben
}; static Keyboard keyboard;

constexpr bool getButton(Keyboard& keyboard, KEYBOARDBUTTON button)noexcept{return keyboard.buttons & button;}
constexpr void setButton(Keyboard& keyboard, KEYBOARDBUTTON button)noexcept{keyboard.buttons |= button;}
constexpr void resetButton(Keyboard& keyboard, KEYBOARDBUTTON button)noexcept{keyboard.buttons &= ~button;}

struct Timer{
	LARGE_INTEGER startTime;
	LARGE_INTEGER frequency;
};

//Setzt den Startzeitpunkt des Timers zurück
void resetTimer(Timer& timer)noexcept{
	QueryPerformanceFrequency(&timer.frequency); 
	QueryPerformanceCounter(&timer.startTime);
}
//Gibt den Zeitunterschied seid dem Startzeitpunkt in Millisekunden zurück
QWORD getTimerMillis(Timer& timer)noexcept{
	LARGE_INTEGER endTime;
	QueryPerformanceCounter(&endTime);
	LONGLONG timediff = endTime.QuadPart - timer.startTime.QuadPart;
	timediff *= 1000;
	return (timediff / timer.frequency.QuadPart);
}
//Gibt den Zeitunterschied seid dem Startzeitpunkt in Mikrosekunden zurück
QWORD getTimerMicros(Timer& timer)noexcept{
	LARGE_INTEGER endTime;
	QueryPerformanceCounter(&endTime);
	LONGLONG timediff = endTime.QuadPart - timer.startTime.QuadPart;
	timediff *= 1000000;
	return (timediff / timer.frequency.QuadPart);
}
//Gibt den Zeitunterschied seid dem Startzeitpunkt in "Nanosekunden" zurück
//(leider hängt alles von QueryPerformanceFrequency() ab, also kann es sein, dass man nur Intervalle von Nanosekunden bekommt)
QWORD getTimerNanos(Timer& timer)noexcept{
	LARGE_INTEGER endTime;
	QueryPerformanceCounter(&endTime);
	LONGLONG timediff = endTime.QuadPart - timer.startTime.QuadPart;
	timediff *= 1000000000;
	return (timediff / timer.frequency.QuadPart);
}

#define PERFORMANCE_ANALYZER
#define PERFORMANCE_ANALYZER_DATA_POINTS 3
struct PerfAnalyzer{
	DWORD totalTriangles = 0;
	DWORD drawnTriangles = 0;		//Wegen clipping und backface culling nicht gezeichnete Dreiecke
	DWORD pixelsDrawn = 0;
	DWORD pixelsCulled = 0;			//Wegen Depthbuffer nicht gezeichnete Pixel
	DWORD pointlessTriangles = 0;
	Timer timer[PERFORMANCE_ANALYZER_DATA_POINTS];
}; static PerfAnalyzer _perfAnalyzer;

//Setzt Statistiken zurück
void resetData(PerfAnalyzer& pa)noexcept{
	pa.totalTriangles = 0;
	pa.drawnTriangles = 0;
	pa.pixelsDrawn = 0;
	pa.pixelsCulled = 0;
	pa.pointlessTriangles = 0;
}

//TODO Kompression

DWORD searchInWindow(){
	return 0;
}
DWORD compress(BYTE* input, DWORD size, BYTE* output){
	return 0;
}

DWORD decompress(BYTE* input, DWORD size, BYTE* output){
	return 0;
}
