#pragma once

#include "vec3.h"
#include "source.h"

typedef struct{
	VEC3 pos;
	VEC3 dir;
	VEC3 delta;
	VEC3 side;

	IVEC3 step;
	IVEC3 square_pos;

	int square_side;
}RAY3D;

RAY3D ray3dCreate(VEC3 pos,VEC3 dir);
void ray3dItterate(RAY3D *ray);