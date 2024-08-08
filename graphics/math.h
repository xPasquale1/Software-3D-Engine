#pragma once

#include "util.h"

#define PI 3.14159265359

// #define BRANCHLESSMINMAX

struct ivec2{
	int x;
	int y;
};

struct uivec3{
	unsigned int x;
	unsigned int y;
	unsigned int z;
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

struct fvec4{
	float x;
	float y;
	float z;
	float w;
};

constexpr float distance(const ivec2& a, const ivec2& b)noexcept{return sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y));}

constexpr float length(const fvec3& vec)noexcept{return sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);}

constexpr fvec3 normalize(const fvec3& vec)noexcept{
	float invLength = 1.f/length(vec);
	return {vec.x*invLength, vec.y*invLength, vec.z*invLength};
}

constexpr fvec2 normalize(const fvec2& vec)noexcept{
	float length = 1.f/sqrt(vec.x*vec.x + vec.y*vec.y);
	return {vec.x*length, vec.y*length};
}

constexpr float dot(const fvec3& a, const fvec3& b)noexcept{return (a.x * b.x + a.y * b.y + a.z * b.z);}
constexpr float dot(const fvec2& a, const fvec2& b)noexcept{return (a.x * b.x + a.y * b.y);}
constexpr float cross(const fvec2& a, const fvec2& b)noexcept{return (a.x * b.y - a.y * b.x);}
constexpr fvec3 cross(const fvec3& a, const fvec3& b)noexcept{return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};}

constexpr float deg2rad(const float deg)noexcept{return deg*PI/180;}
constexpr float rad2deg(const float rad)noexcept{return rad*180/PI;}

constexpr float min(const float a, const float b)noexcept{
	#ifdef BRANCHLESSMINMAX
	return a*(a<b)+b*(b<=a);
	#else
	return a < b ? a : b;
	#endif
}
constexpr float max(const float a, const float b)noexcept{
	#ifdef BRANCHLESSMINMAX
	return a*(a>b)+b*(b>=a);
	#else
	return a > b ? a : b;
	#endif
}
constexpr int min(const int a, const int b)noexcept{
	#ifdef BRANCHLESSMINMAX
	return a*(a<b)+b*(b<=a);
	#else
	return a < b ? a : b;
	#endif
}
constexpr int max(const int a, const int b)noexcept{
	#ifdef BRANCHLESSMINMAX
	return a*(a>b)+b*(b>=a);
	#else
	return a > b ? a : b;
	#endif
}

constexpr float clamp(const float val, const float minVal, const float maxVal)noexcept{
	if(val < minVal) return minVal;
	if(val > maxVal) return maxVal;
	return val;
}

constexpr int clamp(const int val, const int minVal, const int maxVal)noexcept{
	if(val < minVal) return minVal;
	if(val > maxVal) return maxVal;
	return val;
}

//Interpoliere die Werte A und B linear für einen Punkt C
constexpr float interpolateLinear(float valA, float valB, fvec2 posA, fvec2 posB, fvec2 posC)noexcept{
	float valDiff = valB-valA;
	return valA + (valDiff*(posC.x-posA.x)+valDiff*(posC.y-posA.y))/((posB.x-posA.x)+(posB.y-posA.y));
}

constexpr fvec3 mulVec3Mat3x3(const fvec3& vec, const float mat[3][3])noexcept{
	fvec3 out = {0, 0, 0};
	out.x += (mat[0][0]*vec.x);
	out.x += (mat[0][1]*vec.y);
	out.x += (mat[0][2]*vec.z);
	out.y += (mat[1][0]*vec.x);
	out.y += (mat[1][1]*vec.y);
	out.y += (mat[1][2]*vec.z);
	out.z += (mat[2][0]*vec.x);
	out.z += (mat[2][1]*vec.y);
	out.z += (mat[2][2]*vec.z);
	return out;
}

constexpr fvec3 reflect(const fvec3& vec, const fvec3& normal)noexcept{
	float dotVal = dot(vec, normal);
	return {vec.x-2*dotVal*normal.x, vec.y-2*dotVal*normal.y, vec.z-2*dotVal*normal.z};
}

#define MULTIPLIER 1664525
#define INCREMENT 1013904223
unsigned long _rand;
unsigned long nextrand()noexcept{
    _rand = (MULTIPLIER*_rand+INCREMENT);
    return _rand;
}

constexpr float lerp(const float a, const float b, const float t)noexcept{
    return a+t*(b-a);
}

//Gibt 0 zurück, falls die Zahl positive ist, sonst 1
constexpr BYTE sign(const float val)noexcept{
	return ((*(DWORD*)&val)>>31);
}

//Testet ob die float Zahl negative ist und gibt entweder -1 oder 1 zurück
constexpr float negSign(const float val)noexcept{
	DWORD buffer = 0b00111111100000000000000000000000;	//Binäre Darstellung einer float 1
	buffer |= ((*(DWORD*)&val)&0b1000'0000'0000'0000'0000'0000'0000'0000);
	return *(float*)&buffer;
}
