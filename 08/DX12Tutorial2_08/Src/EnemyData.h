/**
* @file EnemyData.h
*/
#ifndef DX12TUTORIAL_SRC_ENEMYDATA_H_
#define DX12TUTORIAL_SRC_ENEMYDATA_H_
#include "Collision.h"
#include <string>

/**
* ìGÉfÅ[É^.
*/
struct EnemyData
{
	std::string name;
	Collision::Shape shape;
	int animation;
	int hp;
};

#endif // DX12TUTORIAL_SRC_ENEMYDATA_H_