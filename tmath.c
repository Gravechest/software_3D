#include <intrin.h>

#include "tmath.h"

float tMaxf(float p,float p2){
	return p > p2 ? p : p2;
}

float tMinf(float p,float p2){
	return p < p2 ? p : p2;
}

float tAbsf(float p){
	return p < 0.0f ? -p : p;
}

int tAbs(int x){
	return x > 0 ? x : -x;
}

int tMax(int p,int p2){
	return p > p2 ? p : p2;
}

int tMin(int p,int p2){
	return p < p2 ? p : p2;
}

int tHash(int x){
	x += (x << 10);
	x ^= (x >> 6);
	x += (x << 3);
	x ^= (x >> 11);
	x += (x << 15);
	return x;
}

float tRnd(){
	union p {
		float f;
		int u;
	}r;
	r.u = tHash(__rdtsc());
	r.u &= 0x007fffff;
	r.u |= 0x3f800000;
	return r.f;
}