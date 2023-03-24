#pragma once

#include "vec3.h"
#include "source.h"

#define LIGHT_QUALITY 16.0f
#define LIGHTMAP_DEPTH 3
#define LIGHT_RGB(r,g,b) (VEC3){r*0.003f/LIGHT_QUALITY,g*0.003f/LIGHT_QUALITY,b*0.003f/LIGHT_QUALITY}

typedef struct{
	VEC3 color;
	VEC3 pos;
}LIGHTSPRITE;

typedef struct{
	LIGHTSPRITE* state;
	int cnt;
}LIGHTSPRITEHUB;

typedef struct{
	VEC3 pos;
	VEC3 color;
	int amm;
}LIGHT;

extern VIEW bsp_light[VW_SIZE][VW_SIZE][VW_SIZE];
extern int lightmap_depth;
extern LIGHTSPRITEHUB lightsprite;

void lighting();