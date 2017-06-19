/**
* @file SpatialGrid.cpp
*/
#include "SpatialGrid.h"
#include <unordered_set>
#include <algorithm>

using namespace DirectX;

/**
* ��ԕ����������i�[���閼�O���.
*/
namespace SpatialGrid {

/**
* �R���X�g���N�^.
*
* @param gid �G���e�B�e�B����������O���[�v�ԍ�.
* @param al  �A�j���[�V�������X�g.
* @param pos �G���e�B�e�B�̍��W.
* @param s   �Փˌ`��.
*/
Entity::Entity(uint16_t gid, const AnimationList& al, const XMFLOAT3& p, const Collision::Shape& s) :
	Sprite::Sprite(al, p),
	groupId(gid),
	hasRemoveRequest(false),
	shape(s)
{
}

/**
* ��Ԃ��X�V����.
*
* @param delta �o�ߎ���.
*/
void Entity::Update(double delta)
{
	Sprite::Update(delta);
}

/**
* ��Ԃ̑傫���ƕ�������ݒ肷��.
*
* @param world       ��Ԃ̍L��.
* @param gird        ������.
* @param entityCount �i�[�\�ȃG���e�B�e�B�̐�. ���̐��l�܂ł͒ǉ��̃������m�ۂ��N����Ȃ�.
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
* �Փ˃n���h����o�^����.
*
* @param gid0    �ՓˑΏۂ̃O���[�v�ԍ�.
* @param gid1    ��ՓˑΏۂ̃O���[�v�ԍ�.
* @param handler �Փ˃n���h��.
*/
void World::RegisterHandler(uint32_t gid0, uint32_t gid1, const Handler& handler)
{
	handlerList[gid0 + (gid1 << 16)] = handler;
	if (gid0 != gid1) {
		handlerList[(gid0 << 16) + gid1] = handler;
	}
}

/**
* �Փ˃n���h���̓o�^�𖕏�����.
*
* @param gid0 �ՓˑΏۂ̃O���[�v�ԍ�.
* @param gid1 ��ՓˑΏۂ̃O���[�v�ԍ�.
*/
void World::DeregisterHandler(uint32_t gid0, uint32_t gid1)
{
	handlerList.erase(gid0 + (gid1 << 16));
	if (gid0 != gid1) {
		handlerList.erase((gid0 << 16) + gid1);
	}
}

/**
* �G���e�B�e�B��ǉ�����.
*
* @param groupId �G���e�B�e�B����������O���[�v�ԍ�.
* @param al      �A�j���[�V�������X�g.
* @param pos     �G���e�B�e�B�̍��W.
* @param s       �Փˌ`��.
*
* @return �ǉ������G���e�B�e�B�ւ̃|�C���^.
*         �ڍׂȃp�����[�^��ݒ肷��ꍇ�ȂǂɎg�����Ƃ��ł���.
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
* �폜�v���̂���G���e�B�e�B���폜����.
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
* ��Ԃ��X�V����.
*
* @param delta �o�ߎ���.
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
* �i�q���ԂɃG���e�B�e�B���֘A�t����.
*
* @param gridList �G���e�B�e�B���֘A�t����i�q����.
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
* �Փ˂��Ɖ��.
*
* @param gridList �Ɖ��i�q����.
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
