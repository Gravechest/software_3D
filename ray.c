#include "ray.h"

RAY3D ray3dCreate(VEC3 pos,VEC3 dir){
	RAY3D ray;
	ray.pos = pos;
	ray.dir = dir;
	ray.delta = VEC3absR(VEC3divFR(ray.dir,1.0f));

	if(ray.dir.x < 0.0f){
		ray.step.x = -1;
		ray.side.x = (ray.pos.x-(int)ray.pos.x) * ray.delta.x;
	}
	else{
		ray.step.x = 1;
		ray.side.x = ((int)ray.pos.x + 1.0f - ray.pos.x) * ray.delta.x;
	}
	if(ray.dir.y < 0.0f){
		ray.step.y = -1;
		ray.side.y = (ray.pos.y-(int)ray.pos.y) * ray.delta.y;
	}
	else{
		ray.step.y = 1;
		ray.side.y = ((int)ray.pos.y + 1.0f - ray.pos.y) * ray.delta.y;
	}
	if(ray.dir.z < 0.0f){
		ray.step.z = -1;
		ray.side.z = (ray.pos.z-(int)ray.pos.z) * ray.delta.z;
	}
	else{
		ray.step.z = 1;
		ray.side.z = ((int)ray.pos.z + 1.0f - ray.pos.z) * ray.delta.z;
	}
	ray.square_pos.x = ray.pos.x;
	ray.square_pos.y = ray.pos.y;
	ray.square_pos.z = ray.pos.z;
	return ray;
}

void ray3dItterate(RAY3D *ray){
	if(ray->side.x < ray->side.y){
		if(ray->side.x < ray->side.z){
			ray->square_pos.x += ray->step.x;
			ray->side.x += ray->delta.x;
		}
		else{
			ray->square_pos.z += ray->step.z;
			ray->side.z += ray->delta.z;
		}
	}
	else if(ray->side.y < ray->side.z){
		ray->square_pos.y += ray->step.y;
		ray->side.y += ray->delta.y;
	}
	else{	
		ray->square_pos.z += ray->step.z;
		ray->side.z += ray->delta.z;
	}
}