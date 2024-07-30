#pragma once

#include "util.h"

#define PI 3.14159265359

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
//TODO Rückgabewert lieber machen, da es dann besser zu den anderen Funktionen passt
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

constexpr float min(float a, float b)noexcept{
//	return a*(a<b)+b*(b<=a);
	return a < b ? a : b;
}
constexpr float max(float a, float b)noexcept{
//	return a*(a>b)+b*(b>=a);
	return a > b ? a : b;
}
constexpr int min(int a, int b)noexcept{
//	return a*(a<b)+b*(b<=a);
	return a < b ? a : b;
}
constexpr int max(int a, int b)noexcept{
//	return a*(a>b)+b*(b>=a);
	return a > b ? a : b;
}

constexpr float clamp(float val, float minVal, float maxVal)noexcept{
	if(val < minVal) return minVal;
	if(val > maxVal) return maxVal;
	return val;
}

constexpr int clamp(int val, int minVal, int maxVal)noexcept{
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

float lerp(float a, float b, float t){
    return a+t*(b-a);
}
