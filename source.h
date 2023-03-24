#pragma once

#include "vec3.h"
#include "vec2.h"

#define MALLOC(MEM) HeapAlloc(GetProcessHeap(),0,MEM)
#define MALLOC_ZERO(MEM) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,MEM)
#define MFREE(MEM) HeapFree(GetProcessHeap(),0,MEM)

#define TX_16_8_AMMOUNT 8
#define TX_16_16_AMMOUNT 4

#define SPLIT_THRESHOLD 100.0f

#define DYNAMIC_LIGHT_DECAYRATE 0.8f

#define GROUND_PLANE   (VEC3){1.0f,0.0f,0.0f}
#define GROUND_PLANE_X (VEC3){0.0f,1.0f,0.0f}
#define GROUND_PLANE_Y (VEC3){0.0f,0.0f,1.0f}

#define PLAYER_HITBOX (VEC3){1.0f,0.05f,0.05f}

#define M_PI 3.14159265359f
#define ROTATION(X,Y) ((atan2f(X,Y) + M_PI) / M_PI)

#define VK_W 0x57
#define VK_S 0x53	
#define VK_A 0x41
#define VK_D 0x44

#define RESOLUTION_SCALE 2

#define WND_WIDHT (1920/RESOLUTION_SCALE)
#define WND_HEIGHT (1080/RESOLUTION_SCALE)
#define WND_SIZE (IVEC2){1920,1080}
#define WND_RESOLUTION (IVEC2){WND_HEIGHT,WND_WIDHT}

#define VW_SIZE 64
#define VW_CB_SIZE 2.0f

#define VW_CRD(pos) (VEC3){pos.x/VW_CB_SIZE,pos.y/VW_CB_SIZE,pos.z/VW_CB_SIZE}

#define FOV_T (500.0f/RESOLUTION_SCALE)
#define FOV (VEC2){FOV_T,FOV_T}

#define PLANE_XY (VEC2){0.0f,0.0f}
#define PLANE_YZ (VEC2){M_PI*0.5f,0.0f}
#define PLANE_XZ (VEC2){M_PI*0.5f,M_PI*0.5f}

#define RD_SQUARE(SZ) (IVEC2){SZ/RESOLUTION_SCALE,SZ/RESOLUTION_SCALE}

typedef struct{
	unsigned char r;
	unsigned char g;
	unsigned char b;
}RGB;

typedef struct{
	RGB* draw;
	RGB* render;
}VRAM;

typedef struct{
	float x;
	float y;
	float z;
	float w;
}VEC4;

typedef struct{
	int x;
	int y;
}IVEC2;

typedef struct{
	int x;
	int y;
	int z;
}IVEC3;

typedef struct{
	VEC2  r_coord;
	float z_coord;
}RPOINT;

typedef struct{
	VEC3 pos_3d;
	RPOINT pos;
	float depth;
}CPOINT;

typedef struct{
	CPOINT* point;
	int cnt;
}CPOINTHUB;

typedef struct{
	int point_1;
	int point_2;
	int point_3;
	int point_4;
}POINT_S;

typedef struct{
	VEC3 plane;
	VEC3 pos;
	VEC3 x;
	VEC3 y;
	VEC2 rot;
	float size;
}SQUARE;

typedef struct{
	VEC3** texture;
	float** luminance_transition;
	SQUARE square;
	POINT_S points;
	unsigned int luminance_tick;
	VEC3 luminance_dynamic;
	int texture_id;
}QUAD;

typedef struct{
	QUAD* quad;
	int cnt;
}QUADHUB;

typedef struct{
	VEC3 pos;
	VEC3 vel;
	VEC2 dir;
	VEC4 dir_tri;
	float exposure;
	float swing;
}CAMERA;

typedef struct{
	VEC3 pos;
	VEC3 size;
}HITBOX;

typedef struct{
	HITBOX* hitbox;
	int cnt;
}HITBOXHUB;

typedef struct{
	int cube;
}SHAPES;

typedef struct{
	QUAD** quad;
	VEC3 luminance_static;
	unsigned int luminance_tick;
	VEC3 luminance_dynamic;
	int drawed;
	int cnt;
}VIEW;

typedef struct{
	char w;
	char s;
	char d;
	char a;
}KEYS;

typedef struct{
	int on_ground;
	int gun_animation;
}PLAYER;

typedef struct{
	int cnt;
	int** state;
}VW_DRAWED;

typedef struct{
	float rotation;
	VEC3 vel;
	VEC3 pos;
	int on_ground;
}ENTITY;

typedef struct{
	ENTITY* entity;
	int cnt;
}ENTITYHUB;

typedef struct{
	RGB* texture[128];
	IVEC2 size;
}TEXTUREATLAS;

enum{
	TX_VOID,
	TX_EDGE
};

extern QUADHUB quadhub;
extern VIEW bsp[VW_SIZE][VW_SIZE][VW_SIZE];

float rayIntersectPlane(VEC3 pos,VEC3 dir,VEC3 plane);
QUAD* rayInsersectQuadInView(VIEW view,VEC3 ray_pos,VEC3 ray_dir);