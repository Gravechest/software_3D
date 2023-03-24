#include <Windows.h>
#include <math.h>

#include "lighting.h"
#include "ray.h"
#include "tmath.h"

VIEW bsp_light[VW_SIZE][VW_SIZE][VW_SIZE];
LIGHT light;
int lightmap_depth;
LIGHTSPRITEHUB lightsprite;

void colorQuad(VEC3 pos,VEC3 color,SQUARE square,VEC3** texture){
	VEC3div(&color,square.size*square.size);
	VEC2rot(&pos.y,square.rot.y);
	VEC2rot(&pos.x,square.rot.x);
	pos.y += square.size;
	pos.z += square.size;
	float p_x = pos.y / square.size;
	float p_y =	pos.z / square.size;
	VEC3addVEC3(&texture[0][0],color);
	for(int i = 1;i < LIGHTMAP_DEPTH+1;i++){
		VEC3mul(&color,4.0f);
		VEC3addVEC3(&texture[i][(int)p_y*(1<<i)+(int)p_x],color);
		p_x*=2.0f;
		p_y*=2.0f;
	}
}

void traceLightRay(RAY3D ray,VEC3 ray_pos,VEC3 color,char bounce){
	while(ray.square_pos.x>=0&&ray.square_pos.y>=0&&ray.square_pos.z>=0&&
	ray.square_pos.x<VW_SIZE&&ray.square_pos.y<VW_SIZE&&ray.square_pos.z<VW_SIZE){
		VIEW view = bsp_light[ray.square_pos.x][ray.square_pos.y][ray.square_pos.z];
		VEC3addVEC3(&bsp[ray.square_pos.x][ray.square_pos.y][ray.square_pos.z].luminance_static,VEC3mulR(color,0.005f));
		float min_dst = 999999.0f;
		int quad_sel = -1;
		for(int j = 0;j < view.cnt;j++){
			QUAD quad = *view.quad[j];
			float dst = rayIntersectPlane(VEC3subVEC3R(ray_pos,quad.square.pos),ray.dir,quad.square.plane);
			if(dst > 0.0f && min_dst > dst){
				VEC3 ray_pos_r = VEC3addVEC3R(ray_pos,VEC3mulR(ray.dir,dst));
				VEC3subVEC3(&ray_pos_r,quad.square.pos);	
				VEC2rot(&ray_pos_r.y,quad.square.rot.y);
				VEC2rot(&ray_pos_r.x,quad.square.rot.x);
				if(tAbsf(ray_pos_r.y) < quad.square.size && tAbsf(ray_pos_r.z) < quad.square.size){
					quad_sel = j;
					min_dst = dst;
				}
			}	
		}
		QUAD* quad = rayInsersectQuadInView(view,ray_pos,ray.dir);
		if(quad){
			float dst = rayIntersectPlane(VEC3subVEC3R(ray_pos,quad->square.pos),ray.dir,quad->square.plane);
			VEC3 ray_pos_r = VEC3addVEC3R(ray_pos,VEC3mulR(ray.dir,dst));

			colorQuad(VEC3subVEC3R(ray_pos_r,quad->square.pos),color,quad->square,quad->texture);
			if(bounce--){
				float dst; 
				ray_pos = VEC3subVEC3R(quad->square.pos,VEC3mulR(ray.dir,0.01f));
				do{
					ray.dir = VEC3rnd();
					dst = rayIntersectPlane(VEC3subVEC3R(ray_pos,quad->square.pos),ray.dir,quad->square.plane);
					if(dst < 0.0f) dst = -0.1f / sqrtf(-dst);
				}
				while(dst > 0.0f || (tRnd() - 1.0f) > -dst);
				ray = ray3dCreate(VW_CRD(ray_pos),ray.dir);
				VEC3mul(&color,0.8f);
				traceLightRay(ray,ray_pos,color,bounce);
			}
			return;
		}
		ray3dItterate(&ray);
	}
}

void genLightThread(){
	for(int i = 0;i < light.amm;i++){
		VEC3 ray_pos = (VEC3){light.pos.x+(tRnd()-1.5f),light.pos.y+(tRnd()-1.5f),light.pos.z+(tRnd()-1.5f)};
		VEC3 ray_dir = VEC3rnd();
		RAY3D ray = ray3dCreate(VW_CRD(ray_pos),ray_dir);
		traceLightRay(ray,ray_pos,light.color,3);
	}
}

void genLight(VEC3 pos,VEC3 color,int amm){
	light = (LIGHT){.amm = amm,.pos = pos,.color = color};
	lightsprite.state[lightsprite.cnt++] = (LIGHTSPRITE){.pos =  pos,.color = {color.x*LIGHT_QUALITY/0.003f,color.y*LIGHT_QUALITY/0.003f,color.z*LIGHT_QUALITY/0.003f}};
	HANDLE cpulightgenthreads[8];
	for(int i = 0;i < 8;i++){
		cpulightgenthreads[i] = CreateThread(0,0,genLightThread,0,0,0);
        SetThreadPriority(cpulightgenthreads[i],THREAD_PRIORITY_BELOW_NORMAL);
	}
	WaitForMultipleObjects(8,cpulightgenthreads,1,INFINITE);
}

void calcColorDif(VEC3** texture,float** luminance_transition,int gen_c,IVEC2 gen){
	int c_1 = (gen.x+0)*(2<<gen_c)+(gen.y+0)*2;
	int c_2 = (gen.x+1)*(2<<gen_c)+(gen.y+0)*2;
	int c_3 = (gen.x+1)*(2<<gen_c)+(gen.y+1)*2;
	int c_4 = (gen.x+0)*(2<<gen_c)+(gen.y+1)*2;
	float b_1 = tMaxf(tMaxf(texture[gen_c+1][c_1].x,texture[gen_c+1][c_1].y),texture[gen_c+1][c_1].z);
	float b_2 = tMaxf(tMaxf(texture[gen_c+1][c_2].x,texture[gen_c+1][c_2].y),texture[gen_c+1][c_2].z); 
	float b_3 = tMaxf(tMaxf(texture[gen_c+1][c_3].x,texture[gen_c+1][c_3].y),texture[gen_c+1][c_3].z);
	float b_4 = tMaxf(tMaxf(texture[gen_c+1][c_4].x,texture[gen_c+1][c_4].y),texture[gen_c+1][c_4].z);
	float dif = tMaxf(tMaxf(b_1,b_2),tMaxf(b_3,b_4))-tMinf(tMinf(b_1,b_2),tMinf(b_3,b_4));
	luminance_transition[gen_c][gen.x*(1<<gen_c)+gen.y] = 1.0f/dif;
	if(gen_c < LIGHTMAP_DEPTH-1){
		gen.x*=2;
		gen.y*=2;
		calcColorDif(texture,luminance_transition,gen_c+1,(IVEC2){gen.x+0,gen.y+0});
		calcColorDif(texture,luminance_transition,gen_c+1,(IVEC2){gen.x+1,gen.y+0});
		calcColorDif(texture,luminance_transition,gen_c+1,(IVEC2){gen.x+1,gen.y+1});
		calcColorDif(texture,luminance_transition,gen_c+1,(IVEC2){gen.x+0,gen.y+1});
	}
}

void lighting(){
	genLight((VEC3){7.0f,15.0f,2.0f},LIGHT_RGB(1.0f,1.0f,1.0f),100000*LIGHT_QUALITY);
	genLight((VEC3){7.0f,15.0f,30.0f},LIGHT_RGB(1.0f,1.0f,0.3f),100000*LIGHT_QUALITY);

	genLight((VEC3){7.0f,32.0f,20.0f},LIGHT_RGB(1.0f,0.3f,1.0f),100000*LIGHT_QUALITY);

	genLight((VEC3){25.0f,4.0f,25.0f},LIGHT_RGB(1.0f,0.2f,0.1f),25000*LIGHT_QUALITY);

	if(LIGHTMAP_DEPTH){
		for(int i = 0;i < quadhub.cnt;i++){
			QUAD* quad = &quadhub.quad[i];
			calcColorDif(quad->texture,quad->luminance_transition,0,(IVEC2){0,0});
		}
	}
	lightmap_depth = LIGHTMAP_DEPTH;
}