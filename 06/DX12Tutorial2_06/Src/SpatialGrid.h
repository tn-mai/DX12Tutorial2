/**
* @file SpatialGrid.h
*/
#ifndef DX12TUTORIAL_SRC_SPATIALGRID_H_
#define DX12TUTORIAL_SRC_SPATIALGRID_H_
#include "Sprite.h"
#include "Collision.h"
#include <DirectXMath.h>
#include <unordered_map>
#include <functional>
#include <deque>

namespace SpatialGrid {

/**
* 衝突判定を持つオブジェクト.
*/
class Entity : public Sprite::Sprite
{
public:
	Entity(uint16_t gid, const AnimationList& al, const DirectX::XMFLOAT3& p, const Collision::Shape& s);
	Entity(const Entity& src) = default;
	~Entity() = default;
	Entity& operator=(const Entity& src) = default;

	void Update(double delta);

	uint16_t GroupId() const { return groupId; }
	const Collision::Shape& Shape() const { return shape; }
	bool HasRemoveRequest() const { return hasRemoveRequest; }
	void SetGroupId(int gid) { groupId = static_cast<uint16_t>(gid); }
	void SetShape(const Collision::Shape& s) { shape = s; }
	void RequestRemove() { hasRemoveRequest = true; }

private:
	uint16_t groupId;
	bool hasRemoveRequest;
	Collision::Shape shape;
};

/**
* 衝突判定を行う空間.
*/
class World
{
private:
	typedef std::vector<Entity*> EntityPtrVector;
	typedef std::deque<Entity*> EntityPtrDeque;
	typedef std::unordered_map<uint32_t, EntityPtrDeque> GridList;

public:
	typedef std::function<void(Entity&, Entity&)> Handler;

	/// イテレータテンプレート.
	template<typename T>
	class IteratorBase : public std::iterator<std::random_access_iterator_tag, std::remove_pointer_t<typename T::value_type>>
	{
		friend class World;
	public:
		IteratorBase() = default;
		~IteratorBase() = default;
		template<typename U> IteratorBase(const IteratorBase<U>& src) : itr(src.itr) {}
		template<typename U> IteratorBase& operator=(const IteratorBase<U>& src) { itr = src.itr; return *this; }
		pointer operator->() const { return *itr; }
		reference operator*() const { return **itr; }
		friend bool operator==(const IteratorBase& lhs, const IteratorBase& rhs) {
			return lhs.itr == rhs.itr;
		}
		friend bool operator!=(const IteratorBase& lhs, const IteratorBase& rhs) { return !(lhs == rhs); }
		IteratorBase& operator++() { ++itr; return *this; }
		IteratorBase operator++(int) { IteratorBase tmp(*this); ++(*this); return tmp; }
		IteratorBase& operator--() { --itr; return *this; }
		IteratorBase operator--(int) { IteratorBase tmp(*this); --(*this); return tmp; }
		IteratorBase& operator+=(difference_type n) { itr += n; return *this; }
		IteratorBase& operator-=(difference_type n) { itr -= n; return *this; }
		IteratorBase operator+(difference_type n) const { return IteratorBase(*this) += n; }
		IteratorBase operator-(difference_type n) const {return IteratorBase(*this) -= n; }

	private:
		IteratorBase(T i) : itr(i) {}
		T itr;
	};
	typedef IteratorBase<EntityPtrVector::const_iterator> const_iterator; ///< 定数イテレータ.
	typedef IteratorBase<EntityPtrVector::iterator> iterator; ///< イテレータ.

	World() = default;
	~World() = default;
	World(const World&) = delete;
	World& operator=(const World&) = delete;

	void SetWorldSize(const DirectX::XMFLOAT2& world, const DirectX::XMUINT2& grid, size_t entityCount);
	void RegisterHandler(uint32_t gid0, uint32_t gid1, const Handler& handler);
	void DeregisterHandler(uint32_t gid0, uint32_t gid1);
	Entity* AddEntity(uint16_t groupId, const AnimationList& al, const DirectX::XMFLOAT3& p, const Collision::Shape&);
	void RemoveEntity();
	void Update(double delta);

	iterator Begin() { return iterator(activeList.begin()); }
	iterator End() { return iterator(activeList.end()); }
	const_iterator Begin() const { return const_iterator(activeList.begin()); }
	const_iterator End() const { return const_iterator(activeList.end()); }

private:
	void PopulateGrid(GridList&);
	void QueryCollision(GridList&);

private:
	DirectX::XMVECTOR worldSize;
	DirectX::XMVECTOR reciprocalGridSize;
	DirectX::XMUINT2 gridCount;
	std::vector<Entity> entityBuffer;
	EntityPtrVector freeList;
	EntityPtrVector activeList;
	std::unordered_map<uint32_t, Handler> handlerList;
};

} // namespace SpatialGrid

#endif // DX12TUTORIAL_SRC_SPATIALGRID_H_