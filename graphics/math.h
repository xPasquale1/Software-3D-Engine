#pragma once

#include "util.h"

#define PI 3.14159265359

struct ivec2{
	int x;
	int y;
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

constexpr inline float distance(ivec2& a, ivec2& b){return sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y));}

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