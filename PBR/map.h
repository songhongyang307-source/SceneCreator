/*==============================================================================

   map controller[map.h]
														 Author : Youhei Sato
														 Date   : 2025/07/03
--------------------------------------------------------------------------------

==============================================================================*/








#ifndef MAP_H
#define MAP_H


#include <DirectXMath.h>
#include "collision.h"


enum ObjectType
{
	GROUND,
	BLOCK,
	BLOCK_STONE,
	ROCK,
	TREE,
	GRASS,
	OBJECTTYPE_MAX
};

struct MapObject
{
	int Kindid;
	DirectX::XMFLOAT3 Position;
	AABB Collision_AABB;
};
void Map_Initialize();

void Map_Finalize();

//void Map_Update(double elapsed_time);

void Map_Draw();

const MapObject* Map_GetObjects(int index);

int Map_GetObjectsCount();

#endif // MAP_H
