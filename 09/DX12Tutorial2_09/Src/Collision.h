/**
* @file Collision.h
*/
#ifndef DX12TUTORIAL_SRC_COLLISION_H_
#define DX12TUTORIAL_SRC_COLLISION_H_
#include <DirectXMath.h>

/**
* 当たり判定名前空間.
*/
namespace Collision {

/**
* 当たり判定の形.
*/
enum class ShapeType
{
	Circle, ///< 円形.
	Rectangle, ///< 長方形.
    Line, ///< 直線.
};

/**
* 当たり判定用の形状オブジェクト.
*
* MakeCircle, MakeRectangle関数で形状を作成できる.
*/
class Shape
{
	friend struct Detector;
	friend bool IsCollision(const Shape&, const DirectX::XMFLOAT2&, const Shape&, const DirectX::XMFLOAT2&);

public:
	struct Circle
	{
		float radius;
	};

	struct Rect
	{
		DirectX::XMFLOAT2 leftTop;
		DirectX::XMFLOAT2 rightBottom;
	};

	struct Line
	{
		DirectX::XMFLOAT2 start;
		DirectX::XMFLOAT2 end;
	};

	static Shape MakeCircle(float r);
	static Shape MakeRectangle(const DirectX::XMFLOAT2& lt, const DirectX::XMFLOAT2& rb);
	static Shape MakeLine(const DirectX::XMFLOAT2& s,const DirectX::XMFLOAT2& e);

	Shape(const Shape& src);
	Shape& operator=(const Shape& src);
	~Shape() = default;

	DirectX::XMVECTOR Aabb(float x, float y) const;

	ShapeType Type() const { return type; }
	const Circle& AsCircle() const {
		static const Circle dummy = {};
		return type == ShapeType::Circle ? circle : dummy;
	}
	const Rect& AsRect() const {
		static const Rect dummy = {};
		return type == ShapeType::Rectangle ? rect : dummy;
	}
	const Line& AsLine() const {
		static const Line dummy = {};
		return type == ShapeType::Line ? line : dummy;
	}

private:
	Shape() = default;

	ShapeType type;
	union {
		Circle circle;
		Rect rect;
		Line line;
	};
};

bool IsCollision(const Shape&, const DirectX::XMFLOAT2&, const Shape&, const DirectX::XMFLOAT2&);

} // namespace Collision

#endif // DX12TUTORIAL_SRC_COLLISION_H_