#include <stdio.h>
#include <math.h>
#include <windows.h>

#include "source.h"
#include "lighting.h"
#include "ray.h"
#include "tmath.h"

#pragma comment(lib,"winmm.lib")

#undef RGB

int proc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

WNDCLASSA wndclass = {.lpfnWndProc = proc,.lpszClassName = "class",.lpszMenuName = "class"};
BITMAPINFO bmi = {sizeof(BITMAPINFOHEADER),0,0,1,24,BI_RGB};

char frame_sync;
HITBOXHUB hitboxhub;
IVEC2 scanline[WND_HEIGHT];
float z_buffer[WND_HEIGHT][WND_WIDHT];

CAMERA camera = {.pos = {3.0f,1.5f,1.5f},.exposure = 1.0f};
CAMERA camera_rd;
CPOINTHUB pointhub;
QUADHUB quadhub;
HWND window;
HDC context;
MSG Msg;
VRAM vram;
KEYS key;
PLAYER player;
VW_DRAWED vw_drawed;
ENTITYHUB entityhub;
TEXTUREATLAS texture32_16 = {.size = {32,16}};
TEXTUREATLAS texture16_16 = {.size = {16,16}};

VIEW bsp[VW_SIZE][VW_SIZE][VW_SIZE];

unsigned int global_tick;

RGB* loadBMP(char* name){
	HANDLE h = CreateFileA(name,GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	int fsize = GetFileSize(h,0);
	char* text = HeapAlloc(GetProcessHeap(),8,fsize+1);
	int offset;
	SetFilePointer(h,0x0a,0,0);
	ReadFile(h,&offset,4,0,0);
	SetFilePointer(h,offset,0,0);
	ReadFile(h,text,fsize-offset,0,0);
	CloseHandle(h);
	return text;
}

float rayIntersectPlane(VEC3 pos,VEC3 dir,VEC3 plane){
    return -(VEC3dotR(pos,plane))/VEC3dotR(dir,plane);
}

float rayIntersectBox(VEC3 ray_pos,VEC3 ray_dir,VEC3 box_pos,VEC3 box_size){
	VEC3 rel_pos = VEC3subVEC3R(ray_pos,box_pos);
	VEC3 delta = VEC3divFR(ray_dir,1.0f);
	VEC3 n = VEC3mulVEC3R(delta,rel_pos);
	VEC3 k = VEC3mulVEC3R(VEC3absR(delta),box_size);
	VEC3 t1 = VEC3subVEC3R(VEC3negR(n),k);
	VEC3 t2 = VEC3addVEC3R(VEC3negR(n),k);
	float tN = tMaxf(tMaxf(t1.x,t1.y),t1.z);
	float tF = tMinf(tMinf(t2.x,t2.y),t2.z);
	if(tN > tF || tF < 0.0f) return -1.0f;
	return tN;
}

QUAD* rayInsersectQuadInView(VIEW view,VEC3 ray_pos,VEC3 ray_dir){
	float min_dst = 999999.0f;
	QUAD* quad_sel = 0;
	for(int j = 0;j < view.cnt;j++){
		QUAD* quad = view.quad[j];
		float dst = rayIntersectPlane(VEC3subVEC3R(ray_pos,quad->square.pos),ray_dir,quad->square.plane);
		if(dst > 0.0f && min_dst > dst){
			VEC3 ray_pos_r = VEC3addVEC3R(ray_pos,VEC3mulR(ray_dir,dst));
			VEC3subVEC3(&ray_pos_r,quad->square.pos);	
			VEC2rot(&ray_pos_r.y,quad->square.rot.y);
			VEC2rot(&ray_pos_r.x,quad->square.rot.x);
			if(tAbsf(ray_pos_r.y) < quad->square.size && tAbsf(ray_pos_r.z) < quad->square.size){
				quad_sel = quad;
				min_dst = dst;
			}
		}	
	}
	return quad_sel;
}

void traceLightRayVar(RAY3D ray,VEC3 ray_pos,VEC3 color){
	while(ray.square_pos.x>=0&&ray.square_pos.y>=0&&ray.square_pos.z>=0&&
	ray.square_pos.x<VW_SIZE&&ray.square_pos.y<VW_SIZE&&ray.square_pos.z<VW_SIZE){
		VIEW view = bsp_light[ray.square_pos.x][ray.square_pos.y][ray.square_pos.z];
		VIEW* view_rd = &bsp[ray.square_pos.x][ray.square_pos.y][ray.square_pos.z];
		VEC3addVEC3(&view_rd->luminance_dynamic,VEC3mulR(color,0.02f));
		view_rd->luminance_tick = global_tick;
		QUAD* quad = rayInsersectQuadInView(view,ray_pos,ray.dir);
		if(quad){
			VEC3addVEC3(&quad->luminance_dynamic,color);
			quad->luminance_tick = global_tick;
			return;
		}
		ray3dItterate(&ray);
	}
}

int proc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam){
	switch(msg){
	case WM_LBUTTONDOWN:
		if(!player.gun_animation) player.gun_animation = 14;
		for(int i = 0;i < 10000;i++)
			traceLightRayVar(ray3dCreate(VEC3mulR(camera.pos,0.5f),VEC3rnd()),camera.pos,(VEC3){0.1f,0.3f,1.0f});
		for(int i = 0;i < entityhub.cnt;i++){
			ENTITY entity = entityhub.entity[i];
			VEC3 ray_dir = {camera_rd.dir_tri.w,camera_rd.dir_tri.y*camera_rd.dir_tri.z,camera_rd.dir_tri.x*camera_rd.dir_tri.z};
			float dst = rayIntersectBox(camera.pos,ray_dir,entity.pos,(VEC3){1.0f,0.2f,0.2f});
			if(dst != -1.0f){
				for(int j = i--;j < entityhub.cnt;j++){
					entityhub.entity[j] = entityhub.entity[j+1];
				}
				entityhub.cnt--;
			}
		}
		break;
	case WM_MOUSEMOVE:;
		POINT cur;
		RECT offset;
		VEC2 r;
		GetCursorPos(&cur);
		GetWindowRect(window,&offset);
		float mx = (float)(cur.x-(offset.left+WND_SIZE.x/2))*0.005f;
		camera.dir.x += mx;
		camera.dir.y -= (float)(cur.y-(offset.top+WND_SIZE.y/2))*0.005f;
		if(!player.on_ground) camera.swing += mx*1.5f;
		if((key.d && camera.swing > 0.0f) || (key.a && camera.swing < 0.0f)){
			VEC2rot(&camera.vel.y,-mx);
		}
		SetCursorPos(offset.left+WND_SIZE.x/2,offset.top+WND_SIZE.y/2);
		break;
	}
	return DefWindowProcA(hwnd,msg,wParam,lParam);
}

void addPoint(VEC3 pos){
	CPOINT* point = &pointhub.point[pointhub.cnt++];
	point->pos_3d = pos;
}

void addHitbox(VEC3 pos,VEC3 size){
	hitboxhub.hitbox[hitboxhub.cnt].pos    = (VEC3){pos.x+size.x/2.0f,pos.y+size.y/2.0f,pos.z+size.z/2.0f};
	hitboxhub.hitbox[hitboxhub.cnt++].size = (VEC3){size.x/2.0f,size.y/2.0f,size.z/2.0f};
}

void render(){
	while(TRUE){
		while(frame_sync) Sleep(1);
		StretchDIBits(context,0,0,WND_SIZE.x,WND_SIZE.y,0,0,WND_RESOLUTION.y,WND_RESOLUTION.x,vram.render,&bmi,DIB_RGB_COLORS,SRCCOPY);
		frame_sync = TRUE;
	}
}

char AABBCC(VEC3 pos_1,VEC3 pos_2,VEC3 size_1,VEC3 size_2){
	if(tAbs(pos_1.x-pos_2.x)<size_1.x+size_2.x &&
	   tAbs(pos_1.y-pos_2.y)<size_1.y+size_2.y &&
	   tAbs(pos_1.z-pos_2.z)<size_1.z+size_2.z) return TRUE;
	return FALSE;
}

void collision(VEC3* position,VEC3* velocity,int* on_ground,float z_offset){
	char on_ground_t = FALSE;
	for(int i = 0;i < hitboxhub.cnt;i++){
		HITBOX hitbox = hitboxhub.hitbox[i];
		hitbox.pos.x+=z_offset;
		VEC3 t_pos = VEC3_ZERO;
		if(AABBCC(*position,hitbox.pos,PLAYER_HITBOX,hitbox.size)){
			position->x -= velocity->x;
			if(!AABBCC(*position,hitbox.pos,PLAYER_HITBOX,hitbox.size)) t_pos.x = velocity->x;
			position->x += velocity->x;

			position->y -= velocity->y;
			if(!AABBCC(*position,hitbox.pos,PLAYER_HITBOX,hitbox.size)){
				if(*on_ground){
					float d = position->x-hitbox.pos.x+hitbox.size.x*0.5f-1.0f;
					if(d>0.0f && d<0.7f) 
						t_pos.x = -d;
					else 					
						t_pos.y = velocity->y;
				}
				else t_pos.y = velocity->y;
			}
			position->y += velocity->y;

			position->z -= velocity->z;
			if(!AABBCC(*position,hitbox.pos,PLAYER_HITBOX,hitbox.size)){
				if(*on_ground){
					float d = position->x-hitbox.pos.x+hitbox.size.x*0.5f-1.0f;
					if(d>0.0f && d<0.7f)
						t_pos.x = -d;
					else 					
						t_pos.z = velocity->z;
				}
				else t_pos.z = velocity->z;
			}
			position->z += velocity->z;
			VEC3subVEC3(position,t_pos);
			if(t_pos.x != 0.0f){
				velocity->x = 0.0f;
				on_ground_t = TRUE;
			}
			if(t_pos.y != 0.0f) velocity->y = 0.0f;
			if(t_pos.z != 0.0f) velocity->z = 0.0f;
		}
	}
	*on_ground = on_ground_t;
}

void physics(){
	for(;;){
		global_tick++;
		for(int i = 0;i < entityhub.cnt;i++){
			ENTITY* entity = &entityhub.entity[i];
			if(tRnd()<1.01f){
				VEC3 direction = VEC3rnd();
				VEC3addVEC3(&entity->vel,VEC3mulR(direction,0.03f));
				entity->rotation = atan2f(direction.y,direction.z);
			}
			VEC3addVEC3(&entity->pos,entity->vel);
			collision(&entity->pos,&entity->vel,&entity->on_ground,0.0f);
			entity->vel.x -= 0.0008f;
			if(entity->on_ground){
				entity->vel.y *= 0.98f;
				entity->vel.z *= 0.98f;
			}
			else{
				entity->vel.y *= 0.999f;
			 	entity->vel.z *= 0.999f;
			}
		}
		key.w = GetKeyState(VK_W) & 0x80;
		key.s = GetKeyState(VK_S) & 0x80;
		key.d = GetKeyState(VK_D) & 0x80;
		key.a = GetKeyState(VK_A) & 0x80;
		if(player.on_ground){
			if(key.w){
				float mod = key.d || key.a ? 0.0021f : 0.003f;
				camera.vel.z += cosf(camera.dir.x) * mod;
				camera.vel.y += sinf(camera.dir.x) * mod;
			}
			if(key.s){
				float mod = key.d || key.a ? 0.0021f : 0.003f;
				camera.vel.z -= cosf(camera.dir.x) * mod;
				camera.vel.y -= sinf(camera.dir.x) * mod;
			}
			if(key.d){
				float mod = key.s || key.w ? 0.0021f : 0.003f;
				camera.vel.z += cosf(camera.dir.x + M_PI * 0.5f) * mod;
				camera.vel.y += sinf(camera.dir.x + M_PI * 0.5f) * mod;
			}
			if(key.a){
				float mod = key.s || key.w ? 0.0021f : 0.003f;
				camera.vel.z -= cosf(camera.dir.x + M_PI * 0.5f) * mod;
				camera.vel.y -= sinf(camera.dir.x + M_PI * 0.5f) * mod;
			}
			if(GetKeyState(VK_SPACE)&0x80) camera.vel.x = 0.06f;
		}
		camera.dir_tri = (VEC4){cosf(camera.dir.x),sinf(camera.dir.x),cosf(camera.dir.y),sinf(camera.dir.y)};
		VEC3addVEC3(&camera.pos,camera.vel);
		collision(&camera.pos,&camera.vel,&player.on_ground,0.9f);
		if(player.on_ground){
			camera.vel.y *= 0.9f;
			camera.vel.z *= 0.9f;
		}
		else{
			camera.vel.y *= 0.999f;
			camera.vel.z *= 0.999f;
			if((key.d && camera.swing>0.0f) || (key.a && camera.swing<0.0f)){
				float forward = tAbsf(camera.swing);
				if(forward > 1.0f) forward = tMaxf(2.0f - forward,-1.0f);
				camera.vel.z += cosf(camera.dir.x) * forward * 0.0002f;
				camera.vel.y += sinf(camera.dir.x) * forward * 0.0002f;
			}
			camera.swing *= 0.98f;
		}
		camera.vel.x *= 0.9997f;
		camera.vel.x -= 0.0008f;
		Sleep(3);
	}
}

void project(CPOINT* point){
	VEC3 pos_t = point->pos_3d;
	float temp;
	pos_t = VEC3subVEC3R(camera_rd.pos,pos_t);
	temp    = pos_t.y * camera_rd.dir_tri.x - pos_t.z * camera_rd.dir_tri.y;
	pos_t.z = pos_t.y * camera_rd.dir_tri.y + pos_t.z * camera_rd.dir_tri.x;
	pos_t.y = temp;
	temp    = pos_t.x * camera_rd.dir_tri.z - pos_t.z * camera_rd.dir_tri.w;
	pos_t.z = pos_t.x * camera_rd.dir_tri.w + pos_t.z * camera_rd.dir_tri.z;
	pos_t.x = temp;
	point->pos.r_coord.x = FOV.x * pos_t.x / pos_t.z + WND_RESOLUTION.x / 2.0f;
	point->pos.r_coord.y = FOV.y * pos_t.y / pos_t.z + WND_RESOLUTION.y / 2.0f;
	point->pos.z_coord = pos_t.z;
	point->depth = pos_t.x*pos_t.x+pos_t.y*pos_t.y+pos_t.z*pos_t.z;
}

void projectSprite(CPOINT* point){
	VEC3 pos_t = point->pos_3d;
	float temp;
	temp    = pos_t.y * camera_rd.dir_tri.x - pos_t.z * camera_rd.dir_tri.y;
	pos_t.z = pos_t.y * camera_rd.dir_tri.y + pos_t.z * camera_rd.dir_tri.x;
	pos_t.y = temp;
	temp    = pos_t.x * camera_rd.dir_tri.z - pos_t.z * camera_rd.dir_tri.w;
	pos_t.z = pos_t.x * camera_rd.dir_tri.w + pos_t.z * camera_rd.dir_tri.z;
	pos_t.x = temp;
	point->pos.r_coord.x = FOV.x * pos_t.x / pos_t.z + WND_RESOLUTION.x / 2.0f;
	point->pos.r_coord.y = FOV.y * pos_t.y / pos_t.z + WND_RESOLUTION.y / 2.0f;
	point->pos.z_coord = pos_t.z;
	point->depth = pos_t.x*pos_t.x+pos_t.y*pos_t.y+pos_t.z*pos_t.z;
}

void setScanline(VEC2 pos_1,VEC2 pos_2){
	int p_begin,p_end;
	float delta,delta_pos;
	if(pos_1.x > pos_2.x){
		p_end = pos_2.x;
		p_begin = pos_1.x;
		delta = (pos_2.y - pos_1.y) / (pos_2.x - pos_1.x);
		delta_pos = pos_1.y + delta * ((int)pos_1.x-pos_1.x);
	}
	else{
		p_begin = pos_2.x;
		p_end = pos_1.x;
		delta = (pos_1.y - pos_2.y) / (pos_1.x - pos_2.x);
		delta_pos = pos_2.y + delta * ((int)pos_2.x-pos_2.x);
	}
	if(p_begin < 0 || p_end >= WND_RESOLUTION.x) return;
	if(p_end < 0) p_end = 0;
	if(p_begin >= WND_RESOLUTION.x){
		delta_pos -= delta * (p_begin-WND_RESOLUTION.x);
		p_begin = WND_RESOLUTION.x;
	}
	while(p_begin-- > p_end){
		scanline[p_begin].x = tMin(scanline[p_begin].x,delta_pos);
		scanline[p_begin].y = tMax(scanline[p_begin].y,delta_pos);
		delta_pos -= delta;
	}
}
	
void drawQuad(CPOINT p_1,CPOINT p_2,CPOINT p_3,CPOINT p_4,QUAD* quad,IVEC2 gen,int gen_c){
	float split_threshold = SPLIT_THRESHOLD;
	int d_1 = tAbs(p_1.pos.r_coord.x-p_2.pos.r_coord.x) + tAbs(p_1.pos.r_coord.y-p_2.pos.r_coord.y);
	int d_2 = tAbs(p_2.pos.r_coord.x-p_3.pos.r_coord.x) + tAbs(p_2.pos.r_coord.y-p_3.pos.r_coord.y);
	int d_3 = tAbs(p_3.pos.r_coord.x-p_4.pos.r_coord.x) + tAbs(p_3.pos.r_coord.y-p_4.pos.r_coord.y);
	int d_4 = tAbs(p_4.pos.r_coord.x-p_1.pos.r_coord.x) + tAbs(p_4.pos.r_coord.y-p_1.pos.r_coord.y);
	char behind_vw_plane = p_1.pos.z_coord>0.0f || p_2.pos.z_coord>0.0f || p_3.pos.z_coord>0.0f || p_4.pos.z_coord>0.0f;
	if(gen_c < lightmap_depth) split_threshold *= quad->luminance_transition[gen_c][gen.x*(1<<gen_c)+gen.y];
	if(gen_c < lightmap_depth && (d_1+d_2+d_3+d_4 > split_threshold || behind_vw_plane)){
		CPOINT n_point_1,n_point_2,n_point_3,n_point_4,n_point_5;
		n_point_1.pos_3d = VEC3avgVEC3R(p_1.pos_3d,p_2.pos_3d);
		n_point_2.pos_3d = VEC3avgVEC3R(p_2.pos_3d,p_3.pos_3d);
		n_point_3.pos_3d = VEC3avgVEC3R(p_3.pos_3d,p_4.pos_3d);
		n_point_4.pos_3d = VEC3avgVEC3R(p_4.pos_3d,p_1.pos_3d);
		n_point_5.pos_3d = VEC3avgVEC3R(p_1.pos_3d,p_3.pos_3d);

		project(&n_point_1);
		project(&n_point_2);
		project(&n_point_3);
		project(&n_point_4);
		project(&n_point_5);

		gen.x*=2;
		gen.y*=2;

		drawQuad(p_1,n_point_1,n_point_5,n_point_4,quad,(IVEC2){gen.x+0,gen.y+0},gen_c+1);
		drawQuad(n_point_1,p_2,n_point_2,n_point_5,quad,(IVEC2){gen.x+1,gen.y+0},gen_c+1);
		drawQuad(n_point_5,n_point_2,p_3,n_point_3,quad,(IVEC2){gen.x+1,gen.y+1},gen_c+1);
		drawQuad(n_point_4,n_point_5,n_point_3,p_4,quad,(IVEC2){gen.x+0,gen.y+1},gen_c+1);
	}
	else{
		if(behind_vw_plane) return;
		int max_x = tMax(tMax(p_1.pos.r_coord.x,p_2.pos.r_coord.x),tMax(p_3.pos.r_coord.x,p_4.pos.r_coord.x));
		int min_x = tMin(tMin(p_1.pos.r_coord.x,p_2.pos.r_coord.x),tMin(p_3.pos.r_coord.x,p_4.pos.r_coord.x));
		max_x = tMin(max_x,WND_RESOLUTION.x);
		min_x = tMax(min_x,0);
		float dpt = p_1.depth + p_2.depth + p_3.depth + p_4.depth;
		for(int i = min_x;i < max_x;i++){
			scanline[i].x = WND_RESOLUTION.y;
			scanline[i].y = 0;
		}
		VEC3 color_f = quad->texture[gen_c][gen.x*(1<<gen_c)+gen.y];
		VEC3addVEC3(&color_f,quad->luminance_dynamic);
		VEC3mul(&color_f,camera.exposure);
		RGB color = {tMin(color_f.x,255),tMin(color_f.y,255),tMin(color_f.z,255)};
		setScanline(p_1.pos.r_coord,p_2.pos.r_coord);
		setScanline(p_2.pos.r_coord,p_3.pos.r_coord);
		setScanline(p_3.pos.r_coord,p_4.pos.r_coord);
		setScanline(p_4.pos.r_coord,p_1.pos.r_coord);
		for(int x = min_x;x < max_x;x++){
			int min_y = tMax(scanline[x].x,0);
			int max_y = tMin(scanline[x].y,WND_RESOLUTION.y);
			for(int y = min_y;y < max_y;y++){
				if(z_buffer[x][y]>dpt){
					z_buffer[x][y] = dpt;
					vram.draw[x*WND_RESOLUTION.y+y] = color;
				}
			}
		}
	}
}

void addQuad(POINT_S point,SQUARE square){
	QUAD* quad = &quadhub.quad[quadhub.cnt++];
	quad->points = point;
	quad->square = square;
	quad->texture   = HeapAlloc(GetProcessHeap(),0,sizeof(VEC3*)*LIGHTMAP_DEPTH+1);
	quad->luminance_transition = HeapAlloc(GetProcessHeap(),0,sizeof(float*)*LIGHTMAP_DEPTH);
	for(int i = 0,space = 1;i < LIGHTMAP_DEPTH;i++,space*=4)
		quad->luminance_transition[i] = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(float)*space);	
	for(int i = 0,space = 1;i < LIGHTMAP_DEPTH+1;i++,space*=4)
		quad->texture[i] = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(VEC3)*space);	
}

char rayIntersectSquare(VEC3 ray_pos,VEC3 ray_dir,SQUARE square){
	float dst = rayIntersectPlane(VEC3subVEC3R(ray_pos,square.pos),ray_dir,square.plane);
	if(dst > 0.0f){
		VEC3 ray_pos_r = VEC3addVEC3R(ray_pos,VEC3mulR(ray_dir,dst));
		VEC3subVEC3(&ray_pos_r,square.pos);	
		float p_x = VEC3length(VEC3mulVEC3R(ray_pos_r,square.x));
		float p_y = VEC3length(VEC3mulVEC3R(ray_pos_r,square.y));
		return p_x < square.size && p_y < square.size;
	}
	return FALSE;
}

void traceVwRay(RAY3D ray,VEC3 ray_pos){
	while(ray.square_pos.x>=0&&ray.square_pos.y>=0&&ray.square_pos.z>=0&&
	ray.square_pos.x<VW_SIZE&&ray.square_pos.y<VW_SIZE&&ray.square_pos.z<VW_SIZE){
		VIEW* view = &bsp[ray.square_pos.x][ray.square_pos.y][ray.square_pos.z];
		if(view->quad && !view->drawed){
			view->drawed = TRUE;
			vw_drawed.state[vw_drawed.cnt++] = &view->drawed;
			for(int j = 0;j < view->cnt;j++){
				QUAD* quad = view->quad[j];
				POINT_S points = quad->points;
				CPOINT* point_c = pointhub.point;
				if(quad->luminance_dynamic.r > 0.01f || quad->luminance_dynamic.r > 0.01f || quad->luminance_dynamic.r > 0.01f)
					while(quad->luminance_tick++ < global_tick) VEC3mul(&quad->luminance_dynamic,DYNAMIC_LIGHT_DECAYRATE);
				CPOINT point_1 = point_c[points.point_1];
				CPOINT point_2 = point_c[points.point_2];
				CPOINT point_3 = point_c[points.point_3];
				CPOINT point_4 = point_c[points.point_4];
				if(point_1.pos.z_coord > 0.0f && point_2.pos.z_coord > 0.0f && point_3.pos.z_coord > 0.0f && point_4.pos.z_coord > 0.0f) continue;
				drawQuad(point_1,point_2,point_3,point_4,quad,(IVEC2){0,0},0);
			}
			for(int j = 0;j < view->cnt;j++)
				if(rayIntersectSquare(ray_pos,ray.dir,(*view->quad[j]).square)) return;
		}
		ray3dItterate(&ray);
	}
}

void drawLight(VEC3 pos,VEC3 color){
	CPOINT point;
	point.pos_3d = VEC3subVEC3R(pos,camera.pos);
	projectSprite(&point);
	int size = 500.0f/point.pos.z_coord/RESOLUTION_SCALE;
	int min_x = tMax(point.pos.r_coord.x - size,0);
	int min_y = tMax(point.pos.r_coord.y - size,0);
	int max_x = tMin(point.pos.r_coord.x + size,WND_RESOLUTION.x);
	int max_y = tMin(point.pos.r_coord.y + size,WND_RESOLUTION.y);
	int offset_x = point.pos.r_coord.x - size < 0 ? size - point.pos.r_coord.x : 0;
	int offset_y = point.pos.r_coord.y - size < 0 ? size - point.pos.r_coord.y : 0;
	for(int x = min_x;x < max_x;x++){
		for(int y = min_y;y < max_y;y++){
			if(z_buffer[x][y] > point.depth*4.0f){
				float c_x = x-size-min_x+offset_x,c_y = y-size-min_y+offset_y;
				float dst_to_center = sqrtf(c_x*c_x+c_y*c_y+0.1f);
				int dst = (size*20)/dst_to_center-20;
				if(dst < 0) continue;
				VEC3 color_f = VEC3mulR(color,dst);
				vram.draw[x*WND_RESOLUTION.y+y].r = tMin(color_f.x+vram.draw[x*WND_RESOLUTION.y+y].r,255);
				vram.draw[x*WND_RESOLUTION.y+y].g = tMin(color_f.y+vram.draw[x*WND_RESOLUTION.y+y].g,255);
				vram.draw[x*WND_RESOLUTION.y+y].b = tMin(color_f.z+vram.draw[x*WND_RESOLUTION.y+y].b,255);
			}
		}
	} 
}

void drawSprite(VEC3 pos,RGB* texture,IVEC2 t_size){
	CPOINT point;
	point.pos_3d = VEC3subVEC3R(pos,camera.pos);
	projectSprite(&point);
	float size = 500.0f/point.pos.z_coord/RESOLUTION_SCALE;
	int min_x = tMax(point.pos.r_coord.x - size,0);
	int min_y = tMax(point.pos.r_coord.y - size*0.5f,0);
	int max_x = tMin(point.pos.r_coord.x + size,WND_RESOLUTION.x);
	int max_y = tMin(point.pos.r_coord.y + size*0.5f,WND_RESOLUTION.y);
	int offset_x = point.pos.r_coord.x - size        < 0 ? size        - point.pos.r_coord.x : 0;
	int offset_y = point.pos.r_coord.y - size * 0.5f < 0 ? size * 0.5f - point.pos.r_coord.y : 0;
	VIEW* view = &bsp[(int)(pos.x*0.5f)][(int)(pos.y*0.5f)][(int)(pos.z*0.5f)];
	VEC3 color = VEC3mulR(view->luminance_static,camera_rd.exposure);
	if(view->luminance_dynamic.r > 0.01f || view->luminance_dynamic.g > 0.01f || view->luminance_dynamic.b > 0.01f){
		while(view->luminance_tick++ < global_tick) VEC3mul(&view->luminance_dynamic,DYNAMIC_LIGHT_DECAYRATE);
		VEC3addVEC3(&color,view->luminance_dynamic);
	}
	for(int x = min_x;x < max_x;x++){
		int t_x = (x - min_x + offset_x) / (size/16);
		for(int y = min_y;y < max_y;y++){
			int t_y = (y - min_y + offset_y) / (size/16);
			if(z_buffer[x][y] > point.depth*4.0f && texture[t_x*16+t_y].r){
				vram.draw[x*WND_RESOLUTION.y+y].r = tMin(color.x * texture[t_x*16+t_y].r,255);
				vram.draw[x*WND_RESOLUTION.y+y].g = tMin(color.y * texture[t_x*16+t_y].g,255);
				vram.draw[x*WND_RESOLUTION.y+y].b = tMin(color.z * texture[t_x*16+t_y].b,255);
				z_buffer[x][y] = point.depth*4.0f;
			}	
		}
	}
}

void drawSpriteGui(IVEC2 pos,RGB* texture,IVEC2 t_size,IVEC2 size){
	IVEC3 bsp_pos = {camera.pos.x*0.5f,camera.pos.y*0.5f,camera.pos.z*0.5f};
	VIEW* view = &bsp[bsp_pos.x][bsp_pos.y][bsp_pos.z];
	VEC3 color = VEC3mulR(view->luminance_static,camera_rd.exposure);
	if(view->luminance_dynamic.r > 0.01f || view->luminance_dynamic.g > 0.01f || view->luminance_dynamic.b > 0.01f){
		while(view->luminance_tick++ < global_tick) VEC3mul(&view->luminance_dynamic,DYNAMIC_LIGHT_DECAYRATE);
		VEC3addVEC3(&color,view->luminance_dynamic);
	}	
	for(int x = pos.x;x < pos.x + size.x;x++){
		int t_x = ((float)(x-pos.x)/size.x) * t_size.x;
		for(int y = pos.y;y < pos.y + size.y;y++){
			int t_y = ((float)(y-pos.y)/size.y) * t_size.y;
			if(texture[t_x*16+t_y].g){
				vram.draw[x*WND_RESOLUTION.y+y].r = tMin(color.x * texture[t_x*16+t_y].r,255);
				vram.draw[x*WND_RESOLUTION.y+y].g = tMin(color.y * texture[t_x*16+t_y].g,255);
				vram.draw[x*WND_RESOLUTION.y+y].b = tMin(color.z * texture[t_x*16+t_y].b,255);
				z_buffer[x][y] = 0.0f;
			}
		}
	}
}

void loop15ms(){
	for(;;){
		if(player.gun_animation) player.gun_animation--;
		Sleep(15);
	}
}

void draw(){
	for(;;){
		camera_rd = camera;
		for(int i = 0;i < pointhub.cnt;i++) project(&pointhub.point[i]);
		vram.draw[WND_RESOLUTION.x/2*WND_RESOLUTION.y+WND_RESOLUTION.y/2] = (RGB){0,255,0};
		z_buffer[WND_RESOLUTION.x/2][WND_RESOLUTION.y/2] = 0.0f;
		int animation = player.gun_animation ? player.gun_animation / 5 + 1 : 0;
		drawSpriteGui((IVEC2){0,WND_RESOLUTION.y/2-100/RESOLUTION_SCALE},texture16_16.texture[animation],texture16_16.size,RD_SQUARE(200));
		for(int x = 0;x < WND_RESOLUTION.x;x++){
			for(int y = 0;y < WND_RESOLUTION.y;y++){
				if(z_buffer[x][y] == 999999.0f){
					VEC3 ray_ang = {0.0f,0.0f,0.0f};
					float pixelOffsetY = (((float)(x) * 2.0f / WND_RESOLUTION.x) - 1.0f) / (FOV.x / WND_RESOLUTION.x * 2.0f);
					float pixelOffsetX = (((float)(y) * 2.0f / WND_RESOLUTION.y) - 1.0f) / (FOV.y / WND_RESOLUTION.y * 2.0f);
					ray_ang.x = camera_rd.dir_tri.w + camera_rd.dir_tri.z * pixelOffsetY;
					ray_ang.z = (camera_rd.dir_tri.x * camera_rd.dir_tri.z - camera_rd.dir_tri.x * camera_rd.dir_tri.w * pixelOffsetY) - camera_rd.dir_tri.y * pixelOffsetX;
					ray_ang.y = (camera_rd.dir_tri.y * camera_rd.dir_tri.z - camera_rd.dir_tri.y * camera_rd.dir_tri.w * pixelOffsetY) + camera_rd.dir_tri.x * pixelOffsetX;
					RAY3D ray = ray3dCreate(VW_CRD(camera_rd.pos),ray_ang);
					traceVwRay(ray,camera_rd.pos);
				}
				if(z_buffer[x][y] == 999999.0f) vram.draw[x*WND_RESOLUTION.y+y] = (RGB){255,0,0};
			}
		}
		for(int i = 0;i < entityhub.cnt;i++){
			ENTITY entity = entityhub.entity[i];
			float rotation = ROTATION(entity.pos.y-camera.pos.y,entity.pos.z-camera.pos.z) * 4.0f;
			drawSprite(entityhub.entity[i].pos,texture32_16.texture[(int)rotation],texture32_16.size);
		}
		for(int i = 0;i < lightsprite.cnt;i++) drawLight(lightsprite.state[i].pos,lightsprite.state[i].color);
		for(int i = 0;i < vw_drawed.cnt;i++) *vw_drawed.state[i] = FALSE;
		vw_drawed.cnt = 0;
		int sample_c = (WND_RESOLUTION.x-1)/32*(WND_RESOLUTION.y/32)*127;
		int brightness = 1;
		for(int x = 0;x < WND_RESOLUTION.x;x+=32){	
			for(int y = 0;y < WND_RESOLUTION.y;y+=32){
				RGB color = vram.draw[x*WND_RESOLUTION.y+y];
				brightness += tMax(color.r,tMax(color.g,color.b));
			}
		}
		float exposure_n = 1.0f/((float)brightness / sample_c);
		camera.exposure = (camera.exposure*99.0f+exposure_n)/100.0f;
		camera.exposure = (camera.exposure*99.0f+1.0f)/100.0f;
		while(!frame_sync) Sleep(1);
		frame_sync = FALSE;
		RGB* temp = vram.draw;
		vram.draw = vram.render;
		vram.render = temp;
		for(int i = 0;i < WND_RESOLUTION.x;i++){
			for(int j = 0;j < WND_RESOLUTION.y;j++) z_buffer[i][j] = 999999.0f;
		}
	}
}

VEC3 genWallVecTransform(VEC3 v,VEC2 rot){
	VEC2rot(&v.x,-rot.x);
	VEC2rot(&v.y,-rot.y);
	return v;
}

void genWall(VEC3 pos,VEC2 rot,IVEC2 size,float v_size){
	int offset = pointhub.cnt;
	VEC3 v_x = genWallVecTransform(GROUND_PLANE_X,rot);
	VEC3 v_y = genWallVecTransform(GROUND_PLANE_Y,rot);
	VEC3mul(&v_x,v_size);
	VEC3mul(&v_y,v_size);
	size.x++;
	size.y++;
	for(int x = 0;x < size.x;x++){
		VEC3 t_pos = pos;
		for(int y = 0;y < size.y;y++){
			addPoint(t_pos);
			VEC3addVEC3(&t_pos,v_y);
		}
		VEC3addVEC3(&pos,v_x);
	}
	for(int x = 0;x < size.x-1;x++){
		for(int y = 0;y < size.y-1;y++){
			SQUARE square;
			POINT_S point_s = {y+x*size.y+offset,y+x*size.y+1+offset,y+x*size.y+size.y+1+offset,y+x*size.y+size.y+offset};
			square.plane = genWallVecTransform(GROUND_PLANE,rot);
			square.rot = rot;
			square.x = v_x;
			square.y = v_y;
			square.pos = VEC3avgVEC3R(pointhub.point[point_s.point_1].pos_3d,pointhub.point[point_s.point_3].pos_3d);
			square.size = v_size*0.5f;
			addQuad(point_s,square);
		}
	}
}

void genBox(VEC3 pos){
	addHitbox(pos,(VEC3){0.5f,2.0f,2.0f});
	genWall((VEC3){pos.x+0.5f,pos.y+0.0f,pos.z+0.0f},PLANE_XY,(IVEC2){2,2},1.0f,0);
	genWall((VEC3){pos.x+0.0f,pos.y+0.0f,pos.z+0.0f},PLANE_XY,(IVEC2){2,2},1.0f,0);
	genWall((VEC3){pos.x+0.0f,pos.y+0.0f,pos.z+0.0f},PLANE_XZ,(IVEC2){1,4},0.5f,0);
	genWall((VEC3){pos.x+0.0f,pos.y+0.0f,pos.z+0.0f},PLANE_YZ,(IVEC2){1,4},0.5f,0);
	genWall((VEC3){pos.x+0.0f,pos.y+0.0f,pos.z+2.0f},PLANE_XZ,(IVEC2){1,4},0.5f,0);
	genWall((VEC3){pos.x+0.0f,pos.y+2.0f,pos.z+0.0f},PLANE_YZ,(IVEC2){1,4},0.5f,0);
}

void main(){
	timeBeginPeriod(1);
	hitboxhub.hitbox  = MALLOC(sizeof(HITBOX)*4000000);
	pointhub.point    = MALLOC(sizeof(CPOINT)*4000000);
	quadhub.quad      = MALLOC(sizeof(QUAD)*4000000);
	vram.render       = MALLOC(sizeof(RGB)*WND_RESOLUTION.x*WND_RESOLUTION.y);
	vram.draw         = MALLOC(sizeof(RGB)*WND_RESOLUTION.x*WND_RESOLUTION.y);
	vw_drawed.state   = MALLOC(sizeof(int*)*100000);
	lightsprite.state = MALLOC(sizeof(LIGHTSPRITE)*1024);
	entityhub.entity  = MALLOC(sizeof(ENTITY)*1024);

	for(int i = 0;i < 10;i++){
		ENTITY* entity = &entityhub.entity[entityhub.cnt++];
		entity->pos = (VEC3){10.0f,10.0f,10.0f};
		entity->vel = VEC3_ZERO;
	}

	RGB* texture = loadBMP("img/test.bmp");
	int stride = texture32_16.size.y*TX_16_8_AMMOUNT;
	for(int i = 0;i < TX_16_8_AMMOUNT;i++){
		texture32_16.texture[i] = MALLOC(sizeof(RGB)*texture32_16.size.y*texture32_16.size.x);
		for(int x = 0;x < texture32_16.size.x;x++){
			for(int y = 0;y < texture32_16.size.y;y++){
				texture32_16.texture[i][x*texture32_16.size.y+y] = texture[x*stride+y+texture32_16.size.y*i];
			}
		}
	}
	MFREE(texture);

	texture = loadBMP("img/texture16_16.bmp");
	stride = texture16_16.size.y*TX_16_16_AMMOUNT;
	for(int i = 0;i < TX_16_16_AMMOUNT;i++){
		texture16_16.texture[i] = MALLOC(sizeof(RGB)*texture16_16.size.y*texture16_16.size.x);
		for(int x = 0;x < texture16_16.size.x;x++){
			for(int y = 0;y < texture16_16.size.y;y++){
				texture16_16.texture[i][x*texture16_16.size.y+y] = texture[x*stride+y+texture16_16.size.y*i];
			}
		}
	}
	MFREE(texture);
	
	genWall((VEC3){0.0f,0.0f,0.0f}, PLANE_XY,(IVEC2){40,40},1.0f);
	genWall((VEC3){0.0f,0.0f,0.0f}, PLANE_YZ,(IVEC2){40,40},1.0f);
	genWall((VEC3){0.0f,0.0f,0.0f}, PLANE_XZ,(IVEC2){40,40},1.0f);

	addHitbox((VEC3){-0.25f,0.0f,0.0f},(VEC3){0.25f,40.0f,40.0f});
	addHitbox((VEC3){0.0f,-0.25f,0.0f},(VEC3){40.0f,0.25f,40.0f});
	addHitbox((VEC3){0.0f,0.0f,-0.25f},(VEC3){40.0f,40.0f,0.25f});

	genWall((VEC3){40.0f,0.0f,0.0f},PLANE_XY,(IVEC2){40,40},1.0f);
	genWall((VEC3){0.0f,40.0f,0.0f},PLANE_YZ,(IVEC2){40,40},1.0f);
	genWall((VEC3){0.0f,0.0f,40.0f},PLANE_XZ,(IVEC2){40,40},1.0f);

	addHitbox((VEC3){40.0f,0.0f,0.0f},(VEC3){0.25f,40.0f,40.0f});
	addHitbox((VEC3){0.0f,40.0f,0.0f},(VEC3){40.0f,0.25f,40.0f});
	addHitbox((VEC3){0.0f,0.0f,40.0f},(VEC3){40.0f,40.0f,0.25f});

	genWall((VEC3){0.0f,10.0f,10.0f},(VEC2){M_PI*0.5f,M_PI*0.25f},(IVEC2){30,1},1.4142f);
	genWall((VEC3){0.0f,10.0f,10.0f},(VEC2){M_PI*0.5f,M_PI*0.75f},(IVEC2){30,1},1.4142f);
	genWall((VEC3){0.0f,11.0f,9.0f}, (VEC2){M_PI*0.5f,M_PI*0.25f},(IVEC2){30,1},1.4142f);
	genWall((VEC3){0.0f,11.0f,11.0f},(VEC2){M_PI*0.5f,M_PI*0.75f},(IVEC2){30,1},1.4142f);

	for(float i = 0.0;i < 20.0f;i+=5.0f){
		genWall((VEC3){0.0f,19.0f+i,9.0f}, PLANE_XZ,(IVEC2){30,2},1.0f);
		genWall((VEC3){0.0f,19.0f+i,9.0f}, PLANE_YZ,(IVEC2){30,2},1.0f);
		genWall((VEC3){0.0f,19.0f+i,11.0f},PLANE_XZ,(IVEC2){30,2},1.0f);
		genWall((VEC3){0.0f,21.0f+i,9.0f}, PLANE_YZ,(IVEC2){30,2},1.0f);
	}
	for(float i = 0.0;i < 20.0f;i+=0.5f){
		genWall((VEC3){0.0f+i,0.0f,5.0f+i},PLANE_XZ,(IVEC2){1,10}       ,0.5f);
		genWall((VEC3){0.0f,5.0f,5.0f+i}  ,PLANE_YZ,(IVEC2){i*2.0f+1,1 },0.5f);
		genWall((VEC3){0.5f+i,0.0f,5.0f+i},PLANE_XY,(IVEC2){10,1}       ,0.5f);
		addHitbox((VEC3){i,0.0f,5.0f+i},(VEC3){0.5f,5.0f,0.5f});
	}
	
	genBox((VEC3){17.5f,1.0f,28.0f});
	genBox((VEC3){17.0f,5.0f,28.0f});
	genBox((VEC3){16.5f,9.0f,28.0f});
	genBox((VEC3){15.5f,12.0f,26.0f});
	genBox((VEC3){15.5f,15.0f,28.0f});
	genBox((VEC3){14.5f,18.0f,27.0f});
	genBox((VEC3){13.5f,23.0f,28.0f});

	for(int i = 0;i < quadhub.cnt;i++){
		QUAD quad = quadhub.quad[i];
		VEC3 p_1 = pointhub.point[quad.points.point_1].pos_3d;
		VEC3 p_2 = pointhub.point[quad.points.point_2].pos_3d;
		VEC3 p_3 = pointhub.point[quad.points.point_3].pos_3d;
		VEC3 p_4 = pointhub.point[quad.points.point_4].pos_3d;
		IVEC3 min,max;
		min.x = tMinf(tMinf(p_1.x,p_2.x),tMinf(p_3.x,p_4.x))/VW_CB_SIZE;
		min.y = tMinf(tMinf(p_1.y,p_2.y),tMinf(p_3.y,p_4.y))/VW_CB_SIZE;
		min.z = tMinf(tMinf(p_1.z,p_2.z),tMinf(p_3.z,p_4.z))/VW_CB_SIZE;
		max.x = tMaxf(tMaxf(p_1.x,p_2.x),tMaxf(p_3.x,p_4.x))/VW_CB_SIZE;
		max.y = tMaxf(tMaxf(p_1.y,p_2.y),tMaxf(p_3.y,p_4.y))/VW_CB_SIZE;
		max.z = tMaxf(tMaxf(p_1.z,p_2.z),tMaxf(p_3.z,p_4.z))/VW_CB_SIZE;
		for(int x = min.x;x <= max.x;x++){
			for(int y = min.y;y <= max.y;y++){
				for(int z = min.z;z <= max.z;z++){
					IVEC3 bsp_crd = {x,y,z};
					VIEW* view = &bsp_light[bsp_crd.x][bsp_crd.y][bsp_crd.z];
					if(!view->quad) view->quad = HeapAlloc(GetProcessHeap(),0,sizeof(QUAD*)*1024);
					view->quad[view->cnt++] = &quadhub.quad[i];
				}
			}
		}
		VEC3 bsp_crd = VEC3avgVEC3R(p_1,p_3);
		VIEW* view = &bsp[(int)(bsp_crd.x/VW_CB_SIZE)][(int)(bsp_crd.y/VW_CB_SIZE)][(int)(bsp_crd.z/VW_CB_SIZE)];
		if(!view->quad) view->quad = MALLOC(sizeof(QUAD*)*1024);
		view->quad[view->cnt++] = &quadhub.quad[i];
	}

	CreateThread(0,0,lighting,0,0,0);

	bmi.bmiHeader.biWidth  = WND_RESOLUTION.y;
	bmi.bmiHeader.biHeight = WND_RESOLUTION.x;
	RegisterClassA(&wndclass);
	window = CreateWindowExA(0,"class","hello",WS_VISIBLE|WS_POPUP,0,0,WND_SIZE.x,WND_SIZE.y,0,0,wndclass.hInstance,0);
	context = GetDC(window);
	ShowCursor(FALSE);
	CreateThread(0,0,render,0,0,0);
	CreateThread(0,0,physics,0,0,0);
	CreateThread(0,0,draw,0,0,0);
	CreateThread(0,0,loop15ms,0,0,0);
	while(GetMessageA(&Msg,window,0,0)){
		TranslateMessage(&Msg);
		DispatchMessageA(&Msg);
	}
}
