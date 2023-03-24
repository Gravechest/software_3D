#pragma once

#define VEC2_ZERO (VEC2){0.0f,0.0f}

typedef struct{
	float x;
	float y;
}VEC2;

VEC2 VEC2subVEC2R(VEC2 v1,VEC2 v2);
VEC2 VEC2addVEC2R(VEC2 v1,VEC2 v2);
VEC2 VEC2mulVEC2R(VEC2 v1,VEC2 v2);
void VEC2div(VEC2* v,float d);
void VEC2mul(VEC2* v,float m);
VEC2 VEC2mulR(VEC2 p,float f);
VEC2 VEC2divR(VEC2 p,float d);
void VEC2subVEC2(VEC2* v1,VEC2 v2);
void VEC2addVEC2(VEC2* v1,VEC2 v2);
float VEC2dotR(VEC2 v1,VEC2 v2);
float VEC2length(VEC2 p);
void VEC2normalize(VEC2* p);
VEC2 VEC2normalizeR(VEC2 p);
float VEC2distance(VEC2 p,VEC2 p2);
VEC2 VEC2divFR(VEC2 p,float p2);
VEC2 VEC2absR(VEC2 p);
void VEC2rot(VEC2* v,float r);