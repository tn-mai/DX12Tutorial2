/**
* @file Collision.cpp
*/
#include "Collision.h"
#include <algorithm>

using namespace DirectX;

namespace Collision {

/**
* 衝突検出クラス.
*/
struct Detector {
	static bool CircleCircle(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb);
	static bool RectCircle(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb);
	static bool CircleRect(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb);
	static bool LineCircle(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb);
	static bool CircleLine(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb);
	static bool RectRect(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb);
	static bool RectLine(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb);
	static bool LineRect(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb);
	static bool LineLine(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb);
};

/**
* 円と円の当たり判定.
*
* @param sa 左辺側の形状.
* @param pa 左辺側の座標.
* @param sb 右辺側の形状.
* @param pb 右辺側の座標.
*
* @retval true  衝突している.
* @retval false 衝突していない.
*/
bool Detector::CircleCircle(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb)
{
	const float distSq = XMVectorGetX(XMVector2LengthSq(XMLoadFloat2(&pa) - XMLoadFloat2(&pb)));
	const float radSum = sa.circle.radius + sb.circle.radius;
	return distSq < radSum * radSum;
}

/**
* 長方形と円の当たり判定.
*
* @param sa 左辺側の形状.
* @param pa 左辺側の座標.
* @param sb 右辺側の形状.
* @param pb 右辺側の座標.
*
* @retval true  衝突している.
* @retval false 衝突していない.
*/
bool Detector::RectCircle(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb)
{
	const XMVECTOR aLT = XMLoadFloat2(&pa) + XMLoadFloat2(&sa.rect.leftTop);
	const XMVECTOR aRB = XMLoadFloat2(&pa) + XMLoadFloat2(&sa.rect.rightBottom);
	const XMVECTOR c = XMLoadFloat2(&pb);
	const XMVECTOR q = XMVectorMin(XMVectorMax(c, aLT), aRB);
	const float distSq = XMVectorGetX(XMVector2LengthSq(q - c));
	const float rad = sb.circle.radius;
	return distSq < sb.circle.radius * sb.circle.radius;
}

/**
* 円と長方形の当たり判定.
*
* @param sa 左辺側の形状.
* @param pa 左辺側の座標.
* @param sb 右辺側の形状.
* @param pb 右辺側の座標.
*
* @retval true  衝突している.
* @retval false 衝突していない.
*/
bool Detector::CircleRect(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb)
{
	return RectCircle(sb, pb, sa, pa);
}

/**
* 長方形と長方形の当たり判定.
*
* @param sa 左辺側の形状.
* @param pa 左辺側の座標.
* @param sb 右辺側の形状.
* @param pb 右辺側の座標.
*
* @retval true  衝突している.
* @retval false 衝突していない.
*/
bool Detector::RectRect(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb)
{
	const XMFLOAT2 aLT(pa.x + sa.rect.leftTop.x, pa.y + sa.rect.leftTop.y);
	const XMFLOAT2 aRB(pa.x + sa.rect.rightBottom.x, pa.y + sa.rect.rightBottom.y);
	const XMFLOAT2 bLT(pb.x + sb.rect.leftTop.x, pb.y + sb.rect.leftTop.y);
	const XMFLOAT2 bRB(pb.x + sb.rect.rightBottom.x, pb.y + sb.rect.rightBottom.y);
	if (aRB.x < bLT.x || aLT.x > bRB.x) return false;
	if (aRB.y < bLT.y || aLT.y > bRB.y) return false;
	return true;
}

/**
* 直線と円の当たり判定.
*
* @param sa 左辺側の形状.
* @param pa 左辺側の座標.
* @param sb 右辺側の形状.
* @param pb 右辺側の座標.
*
* @retval true  衝突している.
* @retval false 衝突していない.
*/
bool Detector::LineCircle(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb)
{
	const XMVECTOR a = XMLoadFloat2(&sa.line.start) + XMLoadFloat2(&pa);
	const XMVECTOR b = XMLoadFloat2(&sa.line.end) + XMLoadFloat2(&pa);
	const XMVECTOR v = XMVector2Normalize(b - a);
	const XMVECTOR o = XMLoadFloat2(&pb);
	const XMVECTOR ao = o - a;
	const XMVECTOR bo = o - b;

	// 円の中心から直線までの距離が円の半径以上なら衝突していない.
	if (XMVectorGetX(XMVectorAbs(XMVector2Cross(v, ao))) > sb.circle.radius) {
		return false;
	}

	// 円の中心が線分の始点より手前にある場合、始点からの距離が半径以上なら衝突していない.
	if (XMVectorGetX(XMVector2Dot(ao, v)) < 0) {
		return XMVectorGetX(XMVector2LengthSq(ao)) < sb.circle.radius * sb.circle.radius;
	}

	// 円の中心が終点より奥にある場合、終点からの距離が半径以上なら衝突していない.
	if (XMVectorGetX(XMVector2Dot(bo, v)) > 0) {
		return XMVectorGetX(XMVector2LengthSq(bo)) < sb.circle.radius * sb.circle.radius;
	}
	return true;
}

/**
* 円と直線の当たり判定.
*
* @param sa 左辺側の形状.
* @param pa 左辺側の座標.
* @param sb 右辺側の形状.
* @param pb 右辺側の座標.
*
* @retval true  衝突している.
* @retval false 衝突していない.
*/
bool Detector::CircleLine(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb)
{
	return LineCircle(sb, pb, sa, pa);
}

/**
* 直線と長方形の当たり判定.
*
* @param sa 左辺側の形状.
* @param pa 左辺側の座標.
* @param sb 右辺側の形状.
* @param pb 右辺側の座標.
*
* @retval true  衝突している.
* @retval false 衝突していない.
*/
bool Detector::LineRect(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb)
{
	XMVECTORF32 bMin, bMax, p0, p1, d;
	bMin.v = XMLoadFloat2(&sb.rect.leftTop) + XMLoadFloat2(&pb);
	bMax.v = XMLoadFloat2(&sb.rect.rightBottom) + XMLoadFloat2(&pb);
	p0.v = XMLoadFloat2(&sa.line.start) + XMLoadFloat2(&pa);
	p1.v = XMLoadFloat2(&sa.line.end) + XMLoadFloat2(&pa);
	d.v = XMVector2Normalize(p1.v - p0.v);

	const float epsilon = FLT_EPSILON;
	float tmin = -FLT_MAX;
	float tmax = FLT_MAX;
	for (int i = 0; i < 2; ++i) {
		if (std::abs(d[i]) < epsilon) {
			if (p0[i] < bMin[i] || p0[i] > bMax[i]) {
				return false;
			}
		} else {
			const float invD = 1.0f / d[i];
			float t1 = (bMin[i] - p0[i]) * invD;
			float t2 = (bMax[i] - p0[i]) * invD;
			if (t1 > t2) { std::swap(t1, t2); }
			tmin = std::max(t1, tmin);
			tmax = std::min(t2, tmax);
			if (tmin > tmax) {
				return false;
			}
		}
	}
	const float distSq = XMVectorGetX(XMVector2LengthSq(p1.v - p0.v));
	tmax *= tmax;
	return tmin <= distSq && tmax >= 0;
}

/**
* 長方形と直線の当たり判定.
*
* @param sa 左辺側の形状.
* @param pa 左辺側の座標.
* @param sb 右辺側の形状.
* @param pb 右辺側の座標.
*
* @retval true  衝突している.
* @retval false 衝突していない.
*/
bool Detector::RectLine(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb)
{
	return LineRect(sb, pb, sa, pa);
}

/**
* 直線と直線の当たり判定.
*
* @param sa 左辺側の形状.
* @param pa 左辺側の座標.
* @param sb 右辺側の形状.
* @param pb 右辺側の座標.
*
* @retval true  衝突している.
* @retval false 衝突していない.
*/
bool Detector::LineLine(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb)
{
	const XMVECTOR a = XMLoadFloat2(&sa.line.start) + XMLoadFloat2(&pa);
	const XMVECTOR b = XMLoadFloat2(&sa.line.end) + XMLoadFloat2(&pa);
	const XMVECTOR c = XMLoadFloat2(&sb.line.start) + XMLoadFloat2(&pb);
	const XMVECTOR d = XMLoadFloat2(&sb.line.end) + XMLoadFloat2(&pb);

	const XMVECTOR ba = b - a;
	const XMVECTOR dc = d - c;
	const float denom = XMVectorGetX(XMVector2Cross(ba, dc));
	if (std::abs(denom) < FLT_EPSILON) {
		return false;
	}
	const XMVECTOR ca = c - a;
	const float r = XMVectorGetX(XMVector2Cross(ca, dc)) / denom;
	if (r < 0 || r > 1) {
		return false;
	}
	const float s = XMVectorGetX(XMVector2Cross(ca, ba)) / denom;
	if (s < 0 || s > 1) {
		return false;
	}
	return true;
}

/**
* コピーコンストラクタ.
*
* @param src コピー元のオブジェクト.
*/
Shape::Shape(const Shape& src) : type(src.type)
{
	switch (type) {
	case ShapeType::Circle:
		circle.radius = src.circle.radius;
		break;
	case ShapeType::Rectangle:
		rect.leftTop = src.rect.leftTop;
		rect.rightBottom = src.rect.rightBottom;
		break;
	case ShapeType::Line:
		line = src.line;
		break;
	}
}

/**
* コピー代入演算子.
*
* @param src コピー元のオブジェクト.
*
* @return 自分自身への参照.
*/
Shape& Shape::operator=(const Shape& src)
{
	this->~Shape();
	new(this) Shape(src);
	return *this;
}

/**
* AABBを計算する.
*
* @param x 形状のX座標.
* @param y 形状のY座標.
*
* @return 形状を完全に含むAABB.
*         AABBはXMVECTORに(x=左 y=上 z=右 w=下)のように格納される.
*/
XMVECTOR Shape::Aabb(float x, float y) const
{
	const XMVECTOR pos = XMVectorSet(x, y, 0, 0);
	switch (type) {
	case ShapeType::Circle: {
		const XMVECTOR r = XMVectorReplicate(circle.radius);
		return XMVectorPermute<0, 1, 4, 5>(pos - r, pos - r);
	}
	case ShapeType::Rectangle:
		return XMVectorPermute<0, 1, 4, 5>(pos + XMLoadFloat2(&rect.leftTop), pos + XMLoadFloat2(&rect.rightBottom));
	case Collision::ShapeType::Line:{
		const XMVECTOR start = XMLoadFloat2(&line.start);
		const XMVECTOR end = XMLoadFloat2(&line.end);
		return XMVectorPermute<0, 1, 4, 5>(pos + XMVectorMin(start, end), pos + XMVectorMax(start, end));
	}
	default:
		return XMVectorZero();
	}
}

/**
* 円形の形状オブジェクトを作成する.
*
* @param r 円の半径.
*
* @return 形状オブジェクト.
*/
Shape Shape::MakeCircle(float r)
{
	Shape shape;
	shape.type = ShapeType::Circle;
	shape.circle.radius = r;
	return shape;
}

/**
* 長方形の形状オブジェクトを作成する.
*
* @param lt 長方形の左上座標.
* @param rb 長方形の右下座標.
*
* @return 形状オブジェクト.
*
* ltとrbはオブジェクト中心からの相対座標で指定する.
*/
Shape Shape::MakeRectangle(const DirectX::XMFLOAT2& lt, const DirectX::XMFLOAT2& rb)
{
	Shape shape;
	shape.type = ShapeType::Rectangle;
	shape.rect.leftTop = lt;
	shape.rect.rightBottom = rb;
	return shape;
}

/**
* 線分オブジェクトを作成する.
*
* @param s 線分の始点.
* @param e 線分の終点.
*
* @return 形状オブジェクト.
*/
Shape Shape::MakeLine(const DirectX::XMFLOAT2& s, const DirectX::XMFLOAT2& e)
{
	Shape shape;
	shape.type = ShapeType::Line;
	shape.line.start = s;
	shape.line.end = e;
	return shape;
}

/**
* 当たり判定.
*
* @param sa 左辺側の形状.
* @param pa 左辺側の座標.
* @param sb 右辺側の形状.
* @param pb 右辺側の座標.
*
* @retval true  衝突している.
* @retval false 衝突していない.
*/
bool IsCollision(const Shape& sa, const XMFLOAT2& pa, const Shape& sb, const XMFLOAT2& pb)
{
	typedef bool(*FuncType)(const Shape&, const XMFLOAT2&, const Shape&, const XMFLOAT2&);
	static const FuncType funcList[3][3] = {
		{ Detector::CircleCircle, Detector::CircleRect, Detector::CircleLine },
		{ Detector::RectCircle, Detector::RectRect, Detector::RectLine },
		{ Detector::LineCircle, Detector::LineRect, Detector::LineLine },
	};

	return funcList[static_cast<int>(sa.type)][static_cast<int>(sb.type)](sa, pa, sb, pb);
}

} // namespace Collision
