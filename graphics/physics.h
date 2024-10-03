#pragma once

#pragma once
#include <math.h>
#include "util.h"
#include "window.h"

typedef unsigned int uint;

/*    4------5
 * 0------1  |
 * |  |   |  |
 * |  6---|--7
 * 2------3
 * Normalen: 0 oben, 1 rechts, 2 vorne, 3 unten, 4 links, 5 hinten
 */

struct Box{
	fvec3 pos = {0, 0, 0};
	fvec3 vel = {0, 0, 0};
	quaternion q = {0, 0, 0, 1};
	fvec3 rotv = {0, 0, 0};
	fvec3 size = {5, 5, 5};
	fvec3 pts[8];
	fvec3 dpts[8];	//TODO unnötig wenn man Punkte anhand der Normalen berechnen würde...
	float mass = 1;
	triangle tris[12];	//TODO eigentlich auch unnötig zu speichern...
	fvec3 norm[3];
	bool dyn = true;
	float In = 5;
};

void update_tris(Box& b){
	b.tris[0].point[0].pos = b.pts[0]; b.tris[0].point[1].pos = b.pts[1]; b.tris[0].point[2].pos = b.pts[2];
	b.tris[1].point[0].pos = b.pts[1]; b.tris[1].point[1].pos = b.pts[2]; b.tris[1].point[2].pos = b.pts[3];
	b.tris[2].point[0].pos = b.pts[0]; b.tris[2].point[1].pos = b.pts[2]; b.tris[2].point[2].pos = b.pts[6];
	b.tris[3].point[0].pos = b.pts[0]; b.tris[3].point[1].pos = b.pts[4]; b.tris[3].point[2].pos = b.pts[6];

	b.tris[4].point[0].pos = b.pts[1]; b.tris[4].point[1].pos = b.pts[3]; b.tris[4].point[2].pos = b.pts[5];
	b.tris[5].point[0].pos = b.pts[3]; b.tris[5].point[1].pos = b.pts[5]; b.tris[5].point[2].pos = b.pts[7];
	b.tris[6].point[0].pos = b.pts[4]; b.tris[6].point[1].pos = b.pts[5]; b.tris[6].point[2].pos = b.pts[6];
	b.tris[7].point[0].pos = b.pts[5]; b.tris[7].point[1].pos = b.pts[6]; b.tris[7].point[2].pos = b.pts[7];
	b.tris[8].point[0].pos = b.pts[0]; b.tris[8].point[1].pos = b.pts[1]; b.tris[8].point[2].pos = b.pts[4];
	b.tris[9].point[0].pos = b.pts[1]; b.tris[9].point[1].pos = b.pts[5]; b.tris[9].point[2].pos = b.pts[4];
	b.tris[10].point[0].pos = b.pts[2]; b.tris[10].point[1].pos = b.pts[3]; b.tris[10].point[2].pos = b.pts[6];
	b.tris[11].point[0].pos = b.pts[3]; b.tris[11].point[1].pos = b.pts[6]; b.tris[11].point[2].pos = b.pts[7];

	for(uint i=0; i < 12; ++i){
		b.tris[i].point[0].normal = {b.tris[i].point[0].pos.x - b.pos.x, b.tris[i].point[0].pos.y - b.pos.y, b.tris[i].point[0].pos.z - b.pos.z};
		b.tris[i].point[1].normal = {b.tris[i].point[1].pos.x - b.pos.x, b.tris[i].point[1].pos.y - b.pos.y, b.tris[i].point[1].pos.z - b.pos.z};
		b.tris[i].point[2].normal = {b.tris[i].point[2].pos.x - b.pos.x, b.tris[i].point[2].pos.y - b.pos.y, b.tris[i].point[2].pos.z - b.pos.z};
		normalize(b.tris[i].point[0].normal);
		normalize(b.tris[i].point[1].normal);
		normalize(b.tris[i].point[2].normal);
	}
}

void update_points(Box& b, float dt){
	//Integriere
	fvec3 dir = {b.rotv.x, b.rotv.y, b.rotv.z};
	float vel = sqrt(b.rotv.x*b.rotv.x+b.rotv.y*b.rotv.y+b.rotv.z*b.rotv.z);
	vel *= dt;
	quaternion nq = AngleAxis(vel, dir);

	//Update Punkte
	//TODO Könnte man auch anhand der Normalen berechnen...
	b.q = Compose(nq, b.q);
	for(uint i=0; i < 8; ++i){
		b.pts[i] = Rotate(b.q, b.dpts[i]);
		b.pts[i].x += b.pos.x; b.pts[i].y += b.pos.y; b.pts[i].z += b.pos.z;
	}

	//Normalen-Flächen
	b.norm[0] = {0, 1, 0}; b.norm[1] = {1, 0, 0}; b.norm[2] = {0, 0, -1};
	for(uint i=0; i < 3; ++i){
		b.norm[i] = Rotate(b.q, b.norm[i]);
	}
}

void init_box(Box& b, fvec3 position, fvec3 rotation, fvec3 size, uint color){
	b.pos = position;
	b.size = size;
	b.dpts[0].x = b.pts[0].x = -size.x; b.dpts[0].y = b.pts[0].y = +size.y; b.dpts[0].z = b.pts[0].z = -size.z;
	b.dpts[1].x = b.pts[1].x = +size.x; b.dpts[1].y = b.pts[1].y = +size.y; b.dpts[1].z = b.pts[1].z = -size.z;
	b.dpts[2].x = b.pts[2].x = -size.x; b.dpts[2].y = b.pts[2].y = -size.y; b.dpts[2].z = b.pts[2].z = -size.z;
	b.dpts[3].x = b.pts[3].x = +size.x; b.dpts[3].y = b.pts[3].y = -size.y; b.dpts[3].z = b.pts[3].z = -size.z;
	b.dpts[4].x = b.pts[4].x = -size.x; b.dpts[4].y = b.pts[4].y = +size.y; b.dpts[4].z = b.pts[4].z = +size.z;
	b.dpts[5].x = b.pts[5].x = +size.x; b.dpts[5].y = b.pts[5].y = +size.y; b.dpts[5].z = b.pts[5].z = +size.z;
	b.dpts[6].x = b.pts[6].x = -size.x; b.dpts[6].y = b.pts[6].y = -size.y; b.dpts[6].z = b.pts[6].z = +size.z;
	b.dpts[7].x = b.pts[7].x = +size.x; b.dpts[7].y = b.pts[7].y = -size.y; b.dpts[7].z = b.pts[7].z = +size.z;
	b.pts[0].x += b.pos.x; b.pts[0].y += b.pos.y; b.pts[0].z += b.pos.z;
	b.pts[1].x += b.pos.x; b.pts[1].y += b.pos.y; b.pts[1].z += b.pos.z;
	b.pts[2].x += b.pos.x; b.pts[2].y += b.pos.y; b.pts[2].z += b.pos.z;
	b.pts[3].x += b.pos.x; b.pts[3].y += b.pos.y; b.pts[3].z += b.pos.z;
	b.pts[4].x += b.pos.x; b.pts[4].y += b.pos.y; b.pts[4].z += b.pos.z;
	b.pts[5].x += b.pos.x; b.pts[5].y += b.pos.y; b.pts[5].z += b.pos.z;
	b.pts[6].x += b.pos.x; b.pts[6].y += b.pos.y; b.pts[6].z += b.pos.z;
	b.pts[7].x += b.pos.x; b.pts[7].y += b.pos.y; b.pts[7].z += b.pos.z;
	for(uint i=0; i < 12; ++i){
		b.tris[i].point[0].color = {((color>>16) & 0xFF)/255.f, ((color>>8) & 0xFF)/255.f, ((color) & 0xFF)/255.f};
		b.tris[i].point[1].color = {((color>>16) & 0xFF)/255.f, ((color>>8) & 0xFF)/255.f, ((color) & 0xFF)/255.f};
		b.tris[i].point[2].color = {((color>>16) & 0xFF)/255.f, ((color>>8) & 0xFF)/255.f, ((color) & 0xFF)/255.f};
	}
	update_points(b, 0);
}

#define FRICTION 1

void update(Box& b, float dt, fvec3 force){
	b.vel.x += (force.x/b.mass) * dt * FRICTION;
	b.vel.y += (force.y/b.mass) * dt * FRICTION;
	b.vel.z += (force.z/b.mass) * dt * FRICTION;
	b.pos.x += b.vel.x * dt;
	b.pos.y += b.vel.y * dt;
	b.pos.z += b.vel.z * dt;
}

inline void apply_impulse(Box& b, fvec3 direction, fvec3 impulse){
	b.vel.x += impulse.x/b.mass;
	b.vel.y += impulse.y/b.mass;
	b.vel.z += impulse.z/b.mass;

	fvec3 cros = cross(direction, impulse);
	float inv = 1/b.In;
	b.rotv.x += inv*cros.x;
	b.rotv.y += inv*cros.y;
	b.rotv.z += inv*cros.z;
}

bool box_box_collision(Box& b1, Box& b2, fvec3& direction, fvec3* pts, uint& pt_count){
	if(!b1.dyn) return false;
		fvec3 axis[15];
		float distance = 340282300000000000000000000000000000000.f;

		//Objekt-Flächen
		axis[0] = b1.norm[0];
		axis[1] = b1.norm[1];
		axis[2] = b1.norm[2];

		axis[3] = b2.norm[0];
		axis[4] = b2.norm[1];
		axis[5] = b2.norm[2];

		fvec3 b1x = b1.norm[1];
		fvec3 b1y = b1.norm[0];
		fvec3 b1z = b1.norm[2];
		fvec3 b2x = b2.norm[1];
		fvec3 b2y = b2.norm[0];
		fvec3 b2z = b2.norm[2];
		axis[6] = cross(b1x, b2x);
		normalize(axis[6]);
		axis[7] = cross(b1y, b2x);
		normalize(axis[7]);
		axis[8] = cross(b1z, b2x);
		normalize(axis[8]);

		axis[9] = cross(b1x, b2y);
		normalize(axis[9]);
		axis[10] = cross(b1y, b2y);
		normalize(axis[10]);
		axis[11] = cross(b1z, b2y);
		normalize(axis[11]);

		axis[12] = cross(b1x, b2z);
		normalize(axis[12]);
		axis[13] = cross(b1y, b2z);
		normalize(axis[13]);
		axis[14] = cross(b1z, b2z);
		normalize(axis[14]);

		fvec3 pt1_buffer_min[8];
		fvec3 pt1_buffer_max[8];
		fvec3 pt2_buffer_min[8];
		fvec3 pt2_buffer_max[8];
		uint pt1_count = 0;
		uint pt2_count = 0;
		for(int i=0; i < 15; ++i){
			if(axis[i].x == 0 && axis[i].y == 0 && axis[i].z == 0) continue;
			uint pts1_min = 0;
			uint pts1_max = 0;
			uint pts2_min = 0;
			uint pts2_max = 0;

			float min1 = dot(axis[i], b1.pts[0]);
			float max1 = min1;
			pt1_buffer_min[pts1_min++] = b1.pts[0];
			pt1_buffer_max[pts1_max++] = b1.pts[0];
			for(uint j=1; j < 8; ++j){
				float value = dot(axis[i], b1.pts[j]);
				if(fequal(value, min1)){
					pt1_buffer_min[pts1_min++] = b1.pts[j];
				}
				else if(value < min1){
					min1 = value;
					pts1_min = 0;
					pt1_buffer_min[pts1_min++] = b1.pts[j];
				}
				if(fequal(value, max1)){
					pt1_buffer_max[pts1_max++] = b1.pts[j];
				}
				else if(value > max1){
					max1 = value;
					pts1_max = 0;
					pt1_buffer_max[pts1_max++] = b1.pts[j];
				}
			}

			float min2 = dot(axis[i], b2.pts[0]);
			float max2 = min2;
			pt2_buffer_min[pts2_min++] = b2.pts[0];
			pt2_buffer_max[pts2_max++] = b2.pts[0];
			for(uint j=1; j < 8; ++j){
				float value = dot(axis[i], b2.pts[j]);
				if(fequal(value, min2)){
					pt2_buffer_min[pts2_min++] = b2.pts[j];
				}
				else if(value < min2){
					min2 = value;
					pts2_min = 0;
					pt2_buffer_min[pts2_min++] = b2.pts[j];
				}
				if(fequal(value, max2)){
					pt2_buffer_max[pts2_max++] = b2.pts[j];
				}
				else if(value > max2){
					max2 = value;
					pts2_max = 0;
					pt2_buffer_max[pts2_max++] = b2.pts[j];
				}
			}

			float d1 = max2 - min1;
			float d2 = max1 - min2;
			if(d1 < 0 || d2 < 0) return false;

			if(d1 < distance && d1 < d2){
				direction = axis[i];
				distance = d1;
				for(uint i=0; i < pts1_min; ++i){
					pts[i] = pt1_buffer_min[i];
				}
				for(uint i=pts1_min; i < pts1_min+pts2_max; ++i){
					pts[i] = pt2_buffer_max[i-pts1_min];
				}
				pt1_count = pts1_min;
				pt2_count = pts2_max;
			}
			else if(d2 < distance){
				direction = axis[i];
				distance = d2;
				for(uint i=0; i < pts1_max; ++i){
					pts[i] = pt1_buffer_max[i];
				}
				for(uint i=pts1_max; i < pts1_max+pts2_min; ++i){
					pts[i] = pt2_buffer_min[i-pts1_max];
				}
				pt1_count = pts1_max;
				pt2_count = pts2_min;
			}
		}

		//Kollisionspunkte bestimmen
		switch(pt1_count){
		case 1:{	//Punkt-Irgendwas -> fertig
			pt_count = 1;
			break;
		};
		case 2:{	//Kann Kante-Punkt, Kante-Kante oder Kante-Fläche sein
			switch(pt2_count){
			case 1:{	//Kante-Punkt -> fertig
				pts[0] = pts[2];
				pt_count = 1;
				break;
			}
			case 2:{	//Kante-Kante -> Schnittpunkt finden TODO testen ob identisch -> 2 Schnittpunkte
				pts[0] = line_line_intersection(pts[0], pts[1], pts[2], pts[3]);
				pt_count = 1;
				break;
			}
			case 4:{	//Kante-Fläche -> Kante mit allen Kanten der Fläche "stutzen"
				fvec3 pt;
				byte res = line_line_segment_intersection(pts[0], pts[1], pts[2], pts[3], pt);
				switch(res){
				case 0: break;
				case 1:{pts[1] = pt; break;}
				case 2:{pts[0] = pt; break;}
				}
				res = line_line_segment_intersection(pts[0], pts[1], pts[4], pts[5], pt);
				switch(res){
				case 0: break;
				case 1:{pts[1] = pt; break;}
				case 2:{pts[0] = pt; break;}
				}
				res = line_line_segment_intersection(pts[0], pts[1], pts[2], pts[4], pt);
				switch(res){
				case 0: break;
				case 1:{pts[1] = pt; break;}
				case 2:{pts[0] = pt; break;}
				}
				res = line_line_segment_intersection(pts[0], pts[1], pts[3], pts[5], pt);
				switch(res){
				case 0: break;
				case 1:{pts[1] = pt; break;}
				case 2:{pts[0] = pt; break;}
				}
				pt_count = 2;
				break;
			}
			break;
			}
			break;
		}
		case 3:{	//TODO
			break;
		}
		case 4:{	//Kann Fläche-Fläche sein, Fläche-Kante oder Fläche-Punkt
			switch(pt2_count){
			case 1:{	//Fläche-Punkt -> fertig
				pts[0] = pts[4];
				pt_count = 1;
				break;
			}
			case 2:{	//Fläche-Kante
				fvec3 pt;
				byte res = line_line_segment_intersection(pts[4], pts[5], pts[0], pts[1], pt);
				switch(res){
				case 0: break;
				case 1:{pts[5] = pt; break;}
				case 2:{pts[4] = pt; break;}
				}
				res = line_line_segment_intersection(pts[4], pts[5], pts[2], pts[3], pt);
				switch(res){
				case 0: break;
				case 1:{pts[5] = pt; break;}
				case 2:{pts[4] = pt; break;}
				}
				res = line_line_segment_intersection(pts[4], pts[5], pts[0], pts[2], pt);
				switch(res){
				case 0: break;
				case 1:{pts[5] = pt; break;}
				case 2:{pts[4] = pt; break;}
				}
				res = line_line_segment_intersection(pts[4], pts[5], pts[1], pts[3], pt);
				switch(res){
				case 0: break;
				case 1:{pts[5] = pt; break;}
				case 2:{pts[4] = pt; break;}
				}
				pts[0] = pts[4];
				pts[1] = pts[5];
				pt_count = 2;
				break;
			}
			case 4:{	//Fläche-Fläche
				fvec3 pt;
				byte res = line_line_segment_intersection(pts[0], pts[1], pts[4], pts[5], pt);
				switch(res){
				case 0: break;
				case 1:{pts[1] = pt; break;}
				case 2:{pts[0] = pt; break;}
				}
				res = line_line_segment_intersection(pts[0], pts[1], pts[6], pts[7], pt);
				switch(res){
				case 0: break;
				case 1:{pts[1] = pt; break;}
				case 2:{pts[0] = pt; break;}
				}
				res = line_line_segment_intersection(pts[0], pts[1], pts[4], pts[6], pt);
				switch(res){
				case 0: break;
				case 1:{pts[1] = pt; break;}
				case 2:{pts[0] = pt; break;}
				}
				res = line_line_segment_intersection(pts[0], pts[1], pts[5], pts[7], pt);
				switch(res){
				case 0: break;
				case 1:{pts[1] = pt; break;}
				case 2:{pts[0] = pt; break;}
				}

				res = line_line_segment_intersection(pts[2], pts[3], pts[4], pts[5], pt);
				switch(res){
				case 0: break;
				case 1:{pts[3] = pt; break;}
				case 2:{pts[2] = pt; break;}
				}
				res = line_line_segment_intersection(pts[2], pts[3], pts[6], pts[7], pt);
				switch(res){
				case 0: break;
				case 1:{pts[3] = pt; break;}
				case 2:{pts[2] = pt; break;}
				}
				res = line_line_segment_intersection(pts[2], pts[3], pts[4], pts[6], pt);
				switch(res){
				case 0: break;
				case 1:{pts[3] = pt; break;}
				case 2:{pts[2] = pt; break;}
				}
				res = line_line_segment_intersection(pts[2], pts[3], pts[5], pts[7], pt);
				switch(res){
				case 0: break;
				case 1:{pts[3] = pt; break;}
				case 2:{pts[2] = pt; break;}
				}

				res = line_line_segment_intersection(pts[0], pts[2], pts[4], pts[5], pt);
				switch(res){
				case 0: break;
				case 1:{pts[2] = pt; break;}
				case 2:{pts[0] = pt; break;}
				}
				res = line_line_segment_intersection(pts[0], pts[2], pts[6], pts[7], pt);
				switch(res){
				case 0: break;
				case 1:{pts[2] = pt; break;}
				case 2:{pts[0] = pt; break;}
				}
				res = line_line_segment_intersection(pts[0], pts[2], pts[4], pts[6], pt);
				switch(res){
				case 0: break;
				case 1:{pts[2] = pt; break;}
				case 2:{pts[0] = pt; break;}
				}
				res = line_line_segment_intersection(pts[0], pts[2], pts[5], pts[7], pt);
				switch(res){
				case 0: break;
				case 1:{pts[2] = pt; break;}
				case 2:{pts[0] = pt; break;}
				}

				res = line_line_segment_intersection(pts[1], pts[3], pts[4], pts[5], pt);
				switch(res){
				case 0: break;
				case 1:{pts[3] = pt; break;}
				case 2:{pts[1] = pt; break;}
				}
				res = line_line_segment_intersection(pts[1], pts[3], pts[6], pts[7], pt);
				switch(res){
				case 0: break;
				case 1:{pts[3] = pt; break;}
				case 2:{pts[1] = pt; break;}
				}
				res = line_line_segment_intersection(pts[1], pts[3], pts[4], pts[6], pt);
				switch(res){
				case 0: break;
				case 1:{pts[3] = pt; break;}
				case 2:{pts[1] = pt; break;}
				}
				res = line_line_segment_intersection(pts[1], pts[3], pts[5], pts[7], pt);
				switch(res){
				case 0: break;
				case 1:{pts[3] = pt; break;}
				case 2:{pts[1] = pt; break;}
				}

				pt_count = 4;
				break;
			}
			break;
			}
			break;
		}
		default:{
			std::string msg;
			msg += "Konnte die Anzahl der Kollisionspunkte nicht bestimmen! Punkteanzahl: ";
			msg += std::to_string(pt1_count) + ", " + std::to_string(pt2_count);
			throw std::runtime_error(msg);
		}
		}

		fvec3 dir = {b1.pos.x - b2.pos.x, b1.pos.y - b2.pos.y, b1.pos.z - b2.pos.z};
		if(dot(dir, direction) > 0){
			direction.x *= -1;
			direction.y *= -1;
			direction.z *= -1;
		}
		b1.pos.x -= direction.x*distance;
		b1.pos.y -= direction.y*distance;
		b1.pos.z -= direction.z*distance;
		return true;
}

bool SAT_Box_Box(Box& b1, Box& b2, fvec3& direction, fvec3* pts, uint& pt_count){
	if(!b1.dyn) return false;
	fvec3 axis[15];
	float distance = 340282300000000000000000000000000000000.f;

	//Objekt-Flächen
	axis[0] = b1.norm[0];
	axis[1] = b1.norm[1];
	axis[2] = b1.norm[2];

	axis[3] = b2.norm[0];
	axis[4] = b2.norm[1];
	axis[5] = b2.norm[2];

	fvec3 b1x = b1.norm[1];
	fvec3 b1y = b1.norm[0];
	fvec3 b1z = b1.norm[2];
	fvec3 b2x = b2.norm[1];
	fvec3 b2y = b2.norm[0];
	fvec3 b2z = b2.norm[2];
	axis[6] = cross(b1x, b2x);
	normalize(axis[6]);
	axis[7] = cross(b1y, b2x);
	normalize(axis[7]);
	axis[8] = cross(b1z, b2x);
	normalize(axis[8]);

	axis[9] = cross(b1x, b2y);
	normalize(axis[9]);
	axis[10] = cross(b1y, b2y);
	normalize(axis[10]);
	axis[11] = cross(b1z, b2y);
	normalize(axis[11]);

	axis[12] = cross(b1x, b2z);
	normalize(axis[12]);
	axis[13] = cross(b1y, b2z);
	normalize(axis[13]);
	axis[14] = cross(b1z, b2z);
	normalize(axis[14]);

	fvec3 pt1_buffer_min[8];
	fvec3 pt1_buffer_max[8];
	fvec3 pt2_buffer_min[8];
	fvec3 pt2_buffer_max[8];

	fvec3 b1pts[8];
	for(int i=0; i < 8; ++i){
		b1pts[i] = {b1.pts[i].x - b1.pos.x, b1.pts[i].y - b1.pos.y, b1.pts[i].z - b1.pos.z};
	}
	fvec3 b2pts[8];
	for(int i=0; i < 8; ++i){
		b2pts[i] = {b2.pts[i].x - b1.pos.x, b2.pts[i].y - b1.pos.y, b2.pts[i].z - b1.pos.z};
	}

	uint pt1_count = 0;
	uint pt2_count = 0;
	for(int i=0; i < 15; ++i){
		if(axis[i].x == 0 && axis[i].y == 0 && axis[i].z == 0) continue;
		uint pts1_min = 0;
		uint pts1_max = 0;
		uint pts2_min = 0;
		uint pts2_max = 0;

		float min1 = dot(axis[i], b1pts[0]);
		float max1 = min1;
		pt1_buffer_min[pts1_min++] = b1.pts[0];
		pt1_buffer_max[pts1_max++] = b1.pts[0];
		for(uint j=1; j < 8; ++j){
			float value = dot(axis[i], b1pts[j]);
			if(fequal(value, min1)){
				pt1_buffer_min[pts1_min++] = b1.pts[j];
			}
			else if(value < min1){
				min1 = value;
				pts1_min = 0;
				pt1_buffer_min[pts1_min++] = b1.pts[j];
			}
			if(fequal(value, max1)){
				pt1_buffer_max[pts1_max++] = b1.pts[j];
			}
			else if(value > max1){
				max1 = value;
				pts1_max = 0;
				pt1_buffer_max[pts1_max++] = b1.pts[j];
			}
		}

		float min2 = dot(axis[i], b2pts[0]);
		float max2 = min2;
		pt2_buffer_min[pts2_min++] = b2.pts[0];
		pt2_buffer_max[pts2_max++] = b2.pts[0];
		for(uint j=1; j < 8; ++j){
			float value = dot(axis[i], b2pts[j]);
			if(fequal(value, min2)){
				pt2_buffer_min[pts2_min++] = b2.pts[j];
			}
			else if(value < min2){
				min2 = value;
				pts2_min = 0;
				pt2_buffer_min[pts2_min++] = b2.pts[j];
			}
			if(fequal(value, max2)){
				pt2_buffer_max[pts2_max++] = b2.pts[j];
			}
			else if(value > max2){
				max2 = value;
				pts2_max = 0;
				pt2_buffer_max[pts2_max++] = b2.pts[j];
			}
		}

		float d1 = max2 - min1;
		float d2 = max1 - min2;
		if(d1 < 0 || d2 < 0) return false;

		if(d1 < distance && d1 < d2){
			direction = axis[i];
			distance = d1;
			for(uint i=0; i < pts1_min; ++i){
				pts[i] = pt1_buffer_min[i];
			}
			for(uint i=pts1_min; i < pts1_min+pts2_max; ++i){
				pts[i] = pt2_buffer_max[i-pts1_min];
			}
			pt1_count = pts1_min;
			pt2_count = pts2_max;
		}
		else if(d2 < distance){
			direction = axis[i];
			distance = d2;
			for(uint i=0; i < pts1_max; ++i){
				pts[i] = pt1_buffer_max[i];
			}
			for(uint i=pts1_max; i < pts1_max+pts2_min; ++i){
				pts[i] = pt2_buffer_min[i-pts1_max];
			}
			pt1_count = pts1_max;
			pt2_count = pts2_min;
		}
	}

	//Kollisionspunkte bestimmen
	switch(pt1_count){
	case 1:{	//Punkt-Irgendwas -> fertig
		pt_count = 1;
		break;
	};
	case 2:{	//Kann Kante-Punkt, Kante-Kante oder Kante-Fläche sein
		switch(pt2_count){
		case 1:{	//Kante-Punkt -> fertig
			pts[0] = pts[2];
			pt_count = 1;
			break;
		}
		case 2:{	//Kante-Kante -> Schnittpunkt finden TODO testen ob identisch -> 2 Schnittpunkte
			pts[0] = line_line_intersection(pts[0], pts[1], pts[2], pts[3]);
			pt_count = 1;
			break;
		}
		case 4:{	//Kante-Fläche -> Kante mit allen Kanten der Fläche "stutzen"
			fvec3 pt;
			byte res = line_line_segment_intersection(pts[0], pts[1], pts[2], pts[3], pt);
			switch(res){
			case 0: break;
			case 1:{pts[1] = pt; break;}
			case 2:{pts[0] = pt; break;}
			}
			res = line_line_segment_intersection(pts[0], pts[1], pts[4], pts[5], pt);
			switch(res){
			case 0: break;
			case 1:{pts[1] = pt; break;}
			case 2:{pts[0] = pt; break;}
			}
			res = line_line_segment_intersection(pts[0], pts[1], pts[2], pts[4], pt);
			switch(res){
			case 0: break;
			case 1:{pts[1] = pt; break;}
			case 2:{pts[0] = pt; break;}
			}
			res = line_line_segment_intersection(pts[0], pts[1], pts[3], pts[5], pt);
			switch(res){
			case 0: break;
			case 1:{pts[1] = pt; break;}
			case 2:{pts[0] = pt; break;}
			}
			pt_count = 2;
			break;
		}
		break;
		}
		break;
	}
	case 3:{	//TODO
		break;
	}
	case 4:{	//Kann Fläche-Fläche sein, Fläche-Kante oder Fläche-Punkt
		switch(pt2_count){
		case 1:{	//Fläche-Punkt -> fertig
			pts[0] = pts[4];
			pt_count = 1;
			break;
		}
		case 2:{	//Fläche-Kante
			fvec3 pt;
			byte res = line_line_segment_intersection(pts[4], pts[5], pts[0], pts[1], pt);
			switch(res){
			case 0: break;
			case 1:{pts[5] = pt; break;}
			case 2:{pts[4] = pt; break;}
			}
			res = line_line_segment_intersection(pts[4], pts[5], pts[2], pts[3], pt);
			switch(res){
			case 0: break;
			case 1:{pts[5] = pt; break;}
			case 2:{pts[4] = pt; break;}
			}
			res = line_line_segment_intersection(pts[4], pts[5], pts[0], pts[2], pt);
			switch(res){
			case 0: break;
			case 1:{pts[5] = pt; break;}
			case 2:{pts[4] = pt; break;}
			}
			res = line_line_segment_intersection(pts[4], pts[5], pts[1], pts[3], pt);
			switch(res){
			case 0: break;
			case 1:{pts[5] = pt; break;}
			case 2:{pts[4] = pt; break;}
			}
			pts[0] = pts[4];
			pts[1] = pts[5];
			pt_count = 2;
			break;
		}
		case 4:{	//Fläche-Fläche
			fvec3 pt;
			byte res = line_line_segment_intersection(pts[0], pts[1], pts[4], pts[5], pt);
			switch(res){
			case 0: break;
			case 1:{pts[1] = pt; break;}
			case 2:{pts[0] = pt; break;}
			}
			res = line_line_segment_intersection(pts[0], pts[1], pts[6], pts[7], pt);
			switch(res){
			case 0: break;
			case 1:{pts[1] = pt; break;}
			case 2:{pts[0] = pt; break;}
			}
			res = line_line_segment_intersection(pts[0], pts[1], pts[4], pts[6], pt);
			switch(res){
			case 0: break;
			case 1:{pts[1] = pt; break;}
			case 2:{pts[0] = pt; break;}
			}
			res = line_line_segment_intersection(pts[0], pts[1], pts[5], pts[7], pt);
			switch(res){
			case 0: break;
			case 1:{pts[1] = pt; break;}
			case 2:{pts[0] = pt; break;}
			}

			res = line_line_segment_intersection(pts[2], pts[3], pts[4], pts[5], pt);
			switch(res){
			case 0: break;
			case 1:{pts[3] = pt; break;}
			case 2:{pts[2] = pt; break;}
			}
			res = line_line_segment_intersection(pts[2], pts[3], pts[6], pts[7], pt);
			switch(res){
			case 0: break;
			case 1:{pts[3] = pt; break;}
			case 2:{pts[2] = pt; break;}
			}
			res = line_line_segment_intersection(pts[2], pts[3], pts[4], pts[6], pt);
			switch(res){
			case 0: break;
			case 1:{pts[3] = pt; break;}
			case 2:{pts[2] = pt; break;}
			}
			res = line_line_segment_intersection(pts[2], pts[3], pts[5], pts[7], pt);
			switch(res){
			case 0: break;
			case 1:{pts[3] = pt; break;}
			case 2:{pts[2] = pt; break;}
			}

			res = line_line_segment_intersection(pts[0], pts[2], pts[4], pts[5], pt);
			switch(res){
			case 0: break;
			case 1:{pts[2] = pt; break;}
			case 2:{pts[0] = pt; break;}
			}
			res = line_line_segment_intersection(pts[0], pts[2], pts[6], pts[7], pt);
			switch(res){
			case 0: break;
			case 1:{pts[2] = pt; break;}
			case 2:{pts[0] = pt; break;}
			}
			res = line_line_segment_intersection(pts[0], pts[2], pts[4], pts[6], pt);
			switch(res){
			case 0: break;
			case 1:{pts[2] = pt; break;}
			case 2:{pts[0] = pt; break;}
			}
			res = line_line_segment_intersection(pts[0], pts[2], pts[5], pts[7], pt);
			switch(res){
			case 0: break;
			case 1:{pts[2] = pt; break;}
			case 2:{pts[0] = pt; break;}
			}

			res = line_line_segment_intersection(pts[1], pts[3], pts[4], pts[5], pt);
			switch(res){
			case 0: break;
			case 1:{pts[3] = pt; break;}
			case 2:{pts[1] = pt; break;}
			}
			res = line_line_segment_intersection(pts[1], pts[3], pts[6], pts[7], pt);
			switch(res){
			case 0: break;
			case 1:{pts[3] = pt; break;}
			case 2:{pts[1] = pt; break;}
			}
			res = line_line_segment_intersection(pts[1], pts[3], pts[4], pts[6], pt);
			switch(res){
			case 0: break;
			case 1:{pts[3] = pt; break;}
			case 2:{pts[1] = pt; break;}
			}
			res = line_line_segment_intersection(pts[1], pts[3], pts[5], pts[7], pt);
			switch(res){
			case 0: break;
			case 1:{pts[3] = pt; break;}
			case 2:{pts[1] = pt; break;}
			}

			pt_count = 4;
			break;
		}
		break;
		}
		break;
	}
	default:{
		std::string msg;
		msg += "Konnte die Anzahl der Kollisionspunkte nicht bestimmen! Punkteanzahl: ";
		msg += std::to_string(pt1_count) + ", " + std::to_string(pt2_count);
		throw std::runtime_error(msg);
	}
	}

	fvec3 dir = {b1.pos.x - b2.pos.x, b1.pos.y - b2.pos.y, b1.pos.z - b2.pos.z};
	if(dot(dir, direction) > 0){
		direction.x *= -1;
		direction.y *= -1;
		direction.z *= -1;
	}
	b1.pos.x -= direction.x*distance;
	b1.pos.y -= direction.y*distance;
	b1.pos.z -= direction.z*distance;
	return true;
}

void linear_impulse_static(Box& b, fvec3 normal, float e, fvec3* points, uint points_count){
	if(points_count > 8) throw std::runtime_error("Zu viele Kollisionspunkte bis jetzt!");
	fvec3 directions[8];
	fvec3 impulses[8];
	for(uint i=0; i < points_count; ++i){
		fvec3 point = points[i];
		fvec3 r = {point.x - b.pos.x, point.y - b.pos.y, point.z - b.pos.z};
		fvec3 w = cross(b.rotv, r);
		fvec3 cn = {b.vel.x+w.x, b.vel.y+w.y, b.vel.z+w.z};
		float d = dot(cn, normal);
		float j = (-(1+e)*d)/(1/b.mass + dot((1/b.In*(cross(cross(r, normal), r))), normal));
		impulses[i] = {j*normal.x, j*normal.y, j*normal.z};
		directions[i] = r;
	}
	for(uint i=0; i < points_count; ++i){
		apply_impulse(b, directions[i], impulses[i]);
	}
	return;
}

void linear_impulse_dynamic(Box& b1, Box& b2, fvec3 normal, float e, fvec3* points, uint points_count){
	if(points_count > 8) throw std::runtime_error("Zu viele Kollisionspunkte bis jetzt!");
	fvec3 directions[8];
	fvec3 impulsesb1[8];
	fvec3 impulsesb2[8];
	for(uint i=0; i < points_count; ++i){
		fvec3 point = points[i];
		fvec3 r1 = {point.x - b1.pos.x, point.y - b1.pos.y, point.z - b1.pos.z};
		fvec3 w1 = cross(b1.rotv, r1);
		fvec3 cn1 = {b1.vel.x+w1.x, b1.vel.y+w1.y, b1.vel.z+w1.z};

		fvec3 r2 = {point.x - b2.pos.x, point.y - b2.pos.y, point.z - b2.pos.z};
		fvec3 w2 = cross(b2.rotv, r2);
		fvec3 cn2 = {b2.vel.x+w2.x, b2.vel.y+w2.y, b2.vel.z+w2.z};

		fvec3 vab = {cn2.x-cn1.x, cn2.y-cn1.y, cn2.z-cn1.z};
		float d = dot(vab, normal);
		float numerator = -(1+e)*d;
		fvec3 ro1 = 1/b1.In*(cross(cross(r1, normal), r1));
		fvec3 ro2 = 1/b2.In*(cross(cross(r2, normal), r2));
		fvec3 ro = {ro1.x+ro2.x, ro1.y+ro2.y, ro1.z+ro2.z};
		float denominator = 1/b1.mass+1/b2.mass+dot(ro, normal);
		float j = numerator/denominator;
		impulsesb1[i] = {j*normal.x, j*normal.y, j*normal.z};
		impulsesb2[i] = {-j*normal.x, -j*normal.y, -j*normal.z};
		directions[i] = r1;
	}
	for(uint i=0; i < points_count; ++i){
		apply_impulse(b1, directions[i], impulsesb1[i]);
	}
	for(uint i=0; i < points_count; ++i){
		apply_impulse(b2, directions[i], impulsesb2[i]);
	}
	return;
}
