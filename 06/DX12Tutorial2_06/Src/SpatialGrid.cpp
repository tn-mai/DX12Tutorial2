/**
* @file SpatialGrid.cpp
*/
#include "SpatialGrid.h"
#include <unordered_set>
#include <algorithm>

using namespace DirectX;

/**
* 空間分割処理を格納する名前空間.
*/
namespace SpatialGrid {

/**
* コンストラクタ.
*
* @param gid エンティティが所属するグループ番号.
* @param al  アニメーションリスト.
* @param pos エンティティの座標.
* @param s   衝突形状.
*/
Entity::Entity(uint16_t gid, const AnimationList& al, const XMFLOAT3& p, const Collision::Shape& s) :
	Sprite::Sprite(al, p),
	groupId(gid),
	hasRemoveRequest(false),
	shape(s)
{
}

/**
* 状態を更新する.
*
* @param delta 経過時間.
*/
void Entity::Update(double delta)
{
	Sprite::Update(delta);
}

/**
* 空間の大きさと分割数を設定する.
*
* @param world       空間の広さ.
* @param gird        分割数.
* @param entityCount 格納可能なエンティティの数. この数値までは追加のメモリ確保が起こらない.
*/
void World::SetWorldSize(const XMFLOAT2& world, const XMUINT2& grid, size_t entityCount)
{
	worldSize = XMVectorSet(world.x, world.y, 0, 0);
	gridCount = grid;
	reciprocalGridSize = XMVectorSwizzle<0, 1, 0, 1>(XMVectorReciprocal(worldSize) * XMLoadUInt2(&gridCount));
	entityBuffer.reserve(entityCount);
	activeList.reserve(entityCount);
	freeList.reserve(entityCount);
}

/**
* 衝突ハンドラを登録する.
*
* @param gid0    衝突対象のグループ番号.
* @param gid1    被衝突対象のグループ番号.
* @param handler 衝突ハンドラ.
*/
void World::RegisterHandler(uint32_t gid0, uint32_t gid1, const Handler& handler)
{
	handlerList[gid0 + (gid1 << 16)] = handler;
	if (gid0 != gid1) {
		handlerList[(gid0 << 16) + gid1] = handler;
	}
}

/**
* 衝突ハンドラの登録を抹消する.
*
* @param gid0 衝突対象のグループ番号.
* @param gid1 被衝突対象のグループ番号.
*/
void World::DeregisterHandler(uint32_t gid0, uint32_t gid1)
{
	handlerList.erase(gid0 + (gid1 << 16));
	if (gid0 != gid1) {
		handlerList.erase((gid0 << 16) + gid1);
	}
}

/**
* エンティティを追加する.
*
* @param groupId エンティティが所属するグループ番号.
* @param al      アニメーションリスト.
* @param pos     エンティティの座標.
* @param s       衝突形状.
*
* @return 追加したエンティティへのポインタ.
*         詳細なパラメータを設定する場合などに使うことができる.
*/
Entity* World::AddEntity(uint16_t groupId, const AnimationList& al, const XMFLOAT3& pos, const Collision::Shape& s)
{
	Entity* p;
	if (freeList.empty()) {
		entityBuffer.emplace_back(groupId, al, pos, s);
		decltype(entityBuffer) tmp;
		tmp = entityBuffer;
		p = &entityBuffer.back();
	} else {
		p = freeList.back();
		freeList.pop_back();
		*p = Entity(groupId, al, pos, s);
	}
	activeList.push_back(p);
	return p;
}

/**
* 削除要求のあるエンティティを削除する.
*/
void World::RemoveEntity()
{
	for (Entity* e : activeList) {
		if (e->HasRemoveRequest()) {
			freeList.push_back(e);
		}
	}
	activeList.erase(
		std::remove_if(activeList.begin(), activeList.end(), [](Entity* e) { return e->HasRemoveRequest(); }),
		activeList.end()
	);
}

/**
* 状態を更新する.
*
* @param delta 経過時間.
*/
void World::Update(double delta)
{
	GridList gridList;
	gridList.reserve(std::max(1U, static_cast<size_t>(gridCount.x * gridCount.y / 3)));
	PopulateGrid(gridList);
	QueryCollision(gridList);
	RemoveEntity();
	for (Entity* e : activeList) {
		e->Update(delta);
	}
}

/**
* 格子状空間にエンティティを関連付ける.
*
* @param gridList エンティティを関連付ける格子状空間.
*/
void World::PopulateGrid(GridList& gridList)
{
	const XMVECTOR gridLimit = XMVectorSwizzle<0, 1, 0, 1>(XMVectorSet(static_cast<float>(gridCount.x - 1), static_cast<float>(gridCount.y - 1), 0, 0));
	for (Entity* e : activeList) {
		if (!e || e->HasRemoveRequest()) {
			continue;
		}
		const XMVECTOR aabb = e->Shape().Aabb(e->pos.x, e->pos.y);
		const XMVECTOR ltlt = XMVectorPermute<0, 1, 4, 5>(worldSize, aabb);
		const XMVECTOR rbrb = XMVectorPermute<6, 7, 2, 3>(worldSize, aabb);
		uint32_t result = 0;
		XMVectorGreaterR(&result, ltlt, rbrb);
		if (!XMComparisonAnyTrue(result)) {
			continue;
		}
		XMINT4 region;
		XMStoreSInt4(&region, XMVectorMax(XMVectorZero(), XMVectorMin(aabb * reciprocalGridSize, gridLimit)));
		for (int y = region.y; y <= region.w; ++y) {
			for (int x = region.x; x <= region.z; ++x) {
				gridList[y * gridCount.x + x].push_back(e);
			}
		}
	}
}

/**
* 衝突を照会する.
*
* @param gridList 照会する格子状空間.
*/
void World::QueryCollision(GridList& gridList)
{
	std::unordered_set<uint64_t> colidedPair;
	colidedPair.reserve(1024);
	const Entity* const pStartEntity = entityBuffer.data();
	for (auto& pair : gridList) {
		EntityPtrDeque& e = pair.second;
		if (e.size() < 2) {
			continue;
		}
		const EntityPtrDeque::iterator endR = e.end();
		const EntityPtrDeque::iterator endL = endR - 1;
		for (EntityPtrDeque::iterator itrL = e.begin(); itrL != endL; ++itrL) {
			const uint64_t offsetL = static_cast<uint64_t>(*itrL - pStartEntity);
			Entity& entityL = **itrL;
			const XMFLOAT2 posL = { entityL.pos.x, entityL.pos.y };
			for (EntityPtrDeque::iterator itrR = itrL + 1; itrR != endR; ++itrR) {
				const uint64_t offsetR = static_cast<uint64_t>(*itrR - pStartEntity);
				const uint64_t id = offsetL + (offsetR << 32);
				if (colidedPair.find(id) != colidedPair.end()) {
					continue;
				}
				colidedPair.insert(id);
				colidedPair.insert(offsetR + (offsetL << 32));
				Entity& entityR = **itrR;
				const uint32_t gid = entityL.GroupId() + (entityR.GroupId() << 16);
				const auto itrHandler = handlerList.find(gid);
				if (itrHandler == handlerList.end()) {
					continue;
				}
				if (Collision::IsCollision(entityL.Shape(), posL, entityR.Shape(), { entityR.pos.x, entityR.pos.y })) {
					itrHandler->second(**itrL, **itrR);
				}
			}
		}
	}
}

} // namespace SpatialGrid
