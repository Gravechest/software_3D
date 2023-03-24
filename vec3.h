#pragma once

#define VEC3_ZERO (VEC3){0.0f,0.0f,0.0f}
#define VEC3_SINGLE(VALUE) (VEC3){VALUE,VALUE,VALUE}

typedef struct{
	union{
		float x;
		float r;
	};
	union{
		float y;
		float g;
	};
	union{
		float z;
		float b;
	};
}VEC3;

VEC3 VEC3subVEC3R(VEC3 p,VEC3 p2);
VEC3 VEC3addVEC3R(VEC3 v1,VEC3 v2);
VEC3 VEC3mulVEC3R(VEC3 v1,VEC3 v2);
void VEC3mul(VEC3* v1,float m);
void VEC3div(VEC3* v,float d);
VEC3 VEC3mulR(VEC3 p,float f);
VEC3 VEC3divR(VEC3 p,float d);
void VEC3subVEC3(VEC3* v1,VEC3 v2);
void VEC3addVEC3(VEC3* v1,VEC3 v2);
void VEC3mulVEC3(VEC3* v1,VEC3 v2);
float VEC3dotR(VEC3 v1,VEC3 v2);
float VEC3length(VEC3 p);
void VEC3normalize(VEC3* p);
float VEC3distance(VEC3 p,VEC3 p2);
VEC3 VEC3absR(VEC3 p);
VEC3 VEC3divFR(VEC3 p,float p2);
VEC3 VEC3avgVEC3R(VEC3 v1,VEC3 v2);
VEC3 VEC3avgVEC3R4(VEC3 v1,VEC3 v2,VEC3 v3,VEC3 v4);
VEC3 VEC3cross(VEC3 v1,VEC3 v2);
VEC3 VEC3normalizeR(VEC3 p);
VEC3 VEC3negR(VEC3 v);
VEC3 VEC3rnd();