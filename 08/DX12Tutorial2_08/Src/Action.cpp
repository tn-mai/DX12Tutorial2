/**
* @file Action.cpp
*
* スプライトの動作を制御するスクリプト.
* スプライトの移動、回転、消滅、他のスプライトの生成などの各要素について、
* 目標値、動作時間、間隔といったパラメータで制御する.
*
*/
#include "Action.h"
#include "Json.h"
#include "Sprite.h"
#include "DXMathHelper.h"
#include <algorithm>

using namespace DirectX;

namespace Action {

namespace B {

#ifndef ACTIION_ENABLE_SECTIONING_METHOD_COMPARISON
const size_t separationCount = 10;
const size_t sectionCount = 32;
#endif ACTIION_ENABLE_SECTIONING_METHOD_COMPARISON

/**
* 3次ベジェ曲線.
*/
struct Bezier3
{
	XMVECTOR b0;
	XMVECTOR b1;
	XMVECTOR b2;
	XMVECTOR b3;
};

/// ベジェ曲線の長さを測定する関数の型.
typedef float (XM_CALLCONV *LengthFuncType)(Bezier3, float, float, size_t);

/**
* tにおける3次ベジェ曲線の座標を計算する.
*
* @param b0 制御点0の座標.
* @param b1 制御点1の座標.
* @param b2 制御点2の座標.
* @param b3 制御点3の座標.
* @param t  曲線上の位置.
*
* @return tに対応する座標.
*/
XMVECTOR XM_CALLCONV CalcBezier(XMVECTOR b0, XMVECTOR b1, XMVECTOR b2, XMVECTOR b3, float t)
{
	const float u = 1.0f - t;
	const XMVECTOR coef = XMVectorSet(u * u * u, 3.0f * u * u * t, 3.0f * u * t * t, t * t * t);
	XMVECTOR p = XMVectorMultiply(b0, XXXX(coef));
	p = XMVectorMultiplyAdd(b1, YYYY(coef), p);
	p = XMVectorMultiplyAdd(b2, ZZZZ(coef), p);
	p = XMVectorMultiplyAdd(b3, WWWW(coef), p);
	return p;
/*
	return omt * omt * omt * b0 +
	3.0f * omt * omt * t * b1 +
	3.0f * omt * t * t * b2 +
	t * t * t * b3;
*/
}

/**
* tにおける3次ベジェ曲線の座標を計算する.
*
* @param bezier 3次ベジェ曲線.
* @param t      曲線上の位置.
*
* @return tに対応する座標.
*/
XMVECTOR XM_CALLCONV CalcBezier(Bezier3 bezier, float t)
{
	return CalcBezier(bezier.b0, bezier.b1, bezier.b2, bezier.b3, t);
}

/**
* ベジェ曲線の長さを計算する.
*
* @param bezier 3次ベジェ曲線.
* @param t0 区間の始点のt.
* @param t1 区間の終点のt.
* @param n  分割数.
*
* @return t0-t1間の長さ.
*
* 台形補間法によって3次ベジェ曲線の長さを求める.
*/
float XM_CALLCONV CalcBezierLengthLinear(Bezier3 bezier, float t0, float t1, size_t n)
{
	const float delta = (t1 - t0) / static_cast<float>(n);

	XMVECTOR total = XMVectorZero();
	XMVECTOR current = CalcBezier(bezier, t0);
	for (size_t i = 1; i <= n; ++i) {
		const XMVECTOR next = CalcBezier(bezier, t0 + delta * i);
		total += XMVector2Length(next - current);
		current = next;
	}
	return XMVectorGetX(total);
}

/**
* tにおけるベジェ曲線の接線を計算する.
*
* @param bezier 3次ベジェ曲線.
* @param t  曲線上の位置.
*
* @return tにおける接線.
*
* ド・カステリョのアルゴリズムを整理し、微分すると以下の式を得る. 
* <pre>
*   dU = -3t^2(b0 - 3(b1 - b2) - b3) + 6t(b0 - 2b1 + b2) - 3(b0 - b1)
*
* [導出]
* u ^ 3 * b0 + 3u ^ 2 * t*b1 + 3u * t ^ 2 * b2 + t ^ 3 * b3;
* |
* V
* f(x) = (-t^3+3t^2-3t+1)*b0 +
* 3*(t^3-2t^2+t)*b1 +
* 3*(t^2-t^3)*b2 +
* t^3*b3
* |
* V
* f(x) = t^3(-b0+3*b1-3*b2+b3) +
* t^2(3*b0-6*b1+3*b2) +
* t(-3*b0+3*b1)) +
* b0
* |
* V
* f'(x) = 3At^2+2Bt+C
* </pre>
*/
XMVECTOR XM_CALLCONV CalcDerivativeLength(Bezier3 bezier, float t)
{
	const XMVECTOR coef0 = XMVectorSet(-3 * t * t, 3, 6 * t, 2);
	XMVECTOR dU = XXXX(coef0) * (bezier.b0 - YYYY(coef0) * (bezier.b1 - bezier.b2) - bezier.b3);
	dU += ZZZZ(coef0) * (bezier.b0 - WWWW(coef0) * bezier.b1 + bezier.b2);
	dU += YYYY(coef0) * (bezier.b1 - bezier.b0);
	return XMVector2Length(dU);
}

#ifdef ACTIION_ENABLE_SECTIONING_METHOD_COMPARISON
/**
* ベジェ曲線の長さを計算する.
*
* @param bezier 3次ベジェ曲線.
* @param t0 区間の始点のt.
* @param t1 区間の終点のt.
* @param n  分割数(偶数でなければならない).
*
* @return t0-t1間の長さ.
*
* シンプソン法によって3次ベジェ曲線の長さを求める.
*
* @note シンプソン法は始点・中間点・終点の3点(=2区間)単位で計算を行うため、nは偶数でなければならない.
*/
float XM_CALLCONV CalcBezierLengthSimpson(Bezier3 bezier, float t0, float t1, size_t n)
{
	const float h = (t1 - t0) / static_cast<float>(n);

	XMVECTOR x4 = XMVECTORF32{ 0 };
	for (size_t i = 1; i < n; i += 2) {
		x4 += CalcDerivativeLength(bezier, t0 + h * i);
	}

	XMVECTOR x2 = XMVECTORF32{ 0 };
	for (size_t i = 2; i < n; i += 2) {
		x2 += CalcDerivativeLength(bezier, t0 + h * i);
	}
	const XMVECTOR coef = XMVECTORF32{ 4, 2 };
	XMVECTOR endPoint = CalcDerivativeLength(bezier, t0) + CalcDerivativeLength(bezier, t1);
	return h * (1.0f / 3.0f) * XMVectorGetX(endPoint + XXXX(coef) * x4 + YYYY(coef) * x2);
}
#endif // ACTIION_ENABLE_SECTIONING_METHOD_COMPARISON

/**
* 距離に対応するtの値を求める.
*
* @param bezier 3次ベジェ曲線.
* @param d      始点からの距離.
* @param length 3次ベジェ曲線の長さ.
*
* @return dに対応するtの値.
*
* ニュートン・ラフソン法によって距離dに対応するtを求める.
*/
float XM_CALLCONV FindTValue(Bezier3 bezier, float d, float length, int n, LengthFuncType Func=&CalcBezierLengthLinear)
{
	const float err = 0.001f; // 許容する誤差.
	float t = d / length;
	for (int i = 0; i < 1000; ++i) {
		const float tn = t - (Func(bezier, 0, t, n) - d) / XMVectorGetX(CalcDerivativeLength(bezier, t));
		if (std::abs(tn - t) < err) {
			break;
		}
		t = tn;
	}
	return t;
}

/**
* ベジェ曲線を等分し、各区間の始点のtと区間の単位tを計算する. 
*
* @param bezier 3次ベジェ曲線.
* @param count 分割数.
*
* @param tと単位距離に対するtの増分のリスト.
*/
std::vector<BezierSection> XM_CALLCONV CreateSectionList(Bezier3 bezier, size_t count, size_t n, LengthFuncType func=&CalcBezierLengthLinear)
{
	std::vector<BezierSection> buf;
	buf.reserve(count + 1);
	buf.push_back({ 0, 0 });
	const float length = func(bezier, 0, 1, n);
	const float section = length / static_cast<float>(count);
	for (size_t i = 1; i < count; ++i) {
		const float t = FindTValue(bezier, static_cast<float>(i) * section, length, n, func);
		buf.back().unitT = (t - buf.back().t) / section; 
		buf.push_back({t, 0});
	}
	buf.back().unitT = (1 - buf.back().t) / section; 
	buf.push_back({ 1, 0 });
	return buf;
}

/**
* コンストラクタ.
*/
Controller::Controller() : pattern(nullptr)
{
}

/**
* 移動パターンを指定する.
*
* @param p      設定する移動パターンへのポインタ.
*/
void Controller::SetPattern(const Pattern* p)
{
	pattern = p;
	codeCounter = 0;
	time = 0;
}

/**
* 状態の更新.
*
* @param sprite 移動パターンを適用するスプライト.
* @param delta  前回更新してからの経過時間.
*/
void Controller::Update(Sprite::Sprite& sprite, double delta)
{
	if (!pattern || codeCounter >= pattern->data.size()) {
		return;
	}
	if (codeCounter == 0 && time == 0) {
		basePos = XMLoadFloat3(&sprite.pos);
		startPos = XMLoadFloat3(&sprite.pos);
	}
	time += delta;
	const Code* pCode = pattern->data.data() + codeCounter;
	switch (pCode[0].opcode) {
	case Type::Move: {
		const XMVECTOR relativeTargetPos = XMVectorSet(pCode[1].operand, pCode[2].operand, 0, 0);
		const float speed = pCode[3].operand;
		const XMVECTOR targetPos = XMVectorAdd(relativeTargetPos, basePos);
		const XMVECTOR targetVec = XMVectorSubtract(targetPos, startPos);
		const XMVECTOR unitVec = XMVector2Normalize(targetVec);
		const float currentLength = speed * static_cast<float>(time);
		const XMVECTOR currentVec = XMVectorMultiply(unitVec, XMVectorReplicate(currentLength));
		const uint32_t isOverRun = XMVectorGetIntX(XMVectorGreaterOrEqual(XMVector2LengthSq(currentVec), XMVector2LengthSq(targetVec)));
		if (!isOverRun) {
			XMStoreFloat3(&sprite.pos, XMVectorAdd(startPos, currentVec));
		} else {
			XMStoreFloat3(&sprite.pos, targetPos);
			startPos = targetPos;
			codeCounter += 4;
			time = 0;
		}
		break;
	}
	case Type::Stop:
		if (time >= pCode[1].operand) {
			codeCounter += 2;
			time -= pCode[1].operand;
		}
		break;
	case Type::Bezier:
	{
		const XMVECTOR pos1 = XMVectorAdd(XMVectorSet(pCode[1].operand, pCode[2].operand, 0, 0), basePos);
		const XMVECTOR pos2 = XMVectorAdd(XMVectorSet(pCode[3].operand, pCode[4].operand, 0, 0), basePos);
		const XMVECTOR pos3 = XMVectorAdd(XMVectorSet(pCode[5].operand, pCode[6].operand, 0, 0), basePos);
		const float totalTime = pCode[7].operand;
		if (time < totalTime) {
#if 1
			const Bezier3 bezier = { startPos, pos1, pos2, pos3 };
			if (sectionList.empty()) {
#ifdef ACTIION_ENABLE_SECTIONING_METHOD_COMPARISON
				const LengthFuncType lengthFunc = useSimpsonsRule ? CalcBezierLengthSimpson : CalcBezierLengthLinear;
#else
				const LengthFuncType lengthFunc = CalcBezierLengthLinear;
#endif // ACTIION_ENABLE_SECTIONING_METHOD_COMPARISON
				totalLength = lengthFunc(bezier, 0, 1, separationCount);
				sectionList = CreateSectionList(bezier, sectionCount, separationCount, lengthFunc);
			}
			const float ratio = static_cast<float>(time) / totalTime;
			const size_t index = static_cast<size_t>(ratio * sectionCount);
			const float d = ratio * totalLength;
			const float rd = d - static_cast<float>(index) * totalLength * (1.0f / sectionCount);
			const XMVECTOR p = CalcBezier(bezier, sectionList[index].t + rd * sectionList[index].unitT);
			XMStoreFloat3(&sprite.pos, p);
#else
//			const XMVECTOR p = CalcBezier(startPos, pos1, pos2, pos3, time / totalTime);
//			XMStoreFloat3(&sprite.pos, p);
#endif
		} else {
			XMStoreFloat3(&sprite.pos, pos3);
			startPos = pos3;
			codeCounter += 8;
			time = 0;
			sectionList.clear();
		}
		break;
	}
	}
}

/**
* 移動を終了しているか調べる.
*
* @retval true 移動終了、または移動データが未設定.
* @retval false 移動中.
*/
bool Controller::IsFinished() const
{
	return pattern && codeCounter >= pattern->data.size();
}

/**
* テスト用パターンリストを作成する.
*/
PatternList CreateDefaultPatternList()
{
	PatternList ptnList;
	ptnList.resize(6);
	ptnList[0].name = "MoveTest";
	ptnList[0].data.push_back(Code(Type::Move));
	ptnList[0].data.push_back(Code(-100));
	ptnList[0].data.push_back(Code(100));
	ptnList[0].data.push_back(Code(100));
	ptnList[0].data.push_back(Code(Type::Stop));
	ptnList[0].data.push_back(Code(1));
	ptnList[0].data.push_back(Code(Type::Move));
	ptnList[0].data.push_back(Code(100));
	ptnList[0].data.push_back(Code(100));
	ptnList[0].data.push_back(Code(200));
	ptnList[1].name = "BezierTest";
	ptnList[1].data.push_back(Code(Type::Bezier));
	ptnList[1].data.push_back(Code(-400));
	ptnList[1].data.push_back(Code(600));
	ptnList[1].data.push_back(Code(400));
	ptnList[1].data.push_back(Code(600));
	ptnList[1].data.push_back(Code(0));
	ptnList[1].data.push_back(Code(0));
	ptnList[1].data.push_back(Code(4));

	ptnList[2].name = "WaveL";
	ptnList[2].data.push_back(Code(Type::Move));
	ptnList[2].data.push_back(Code(-100));
	ptnList[2].data.push_back(Code(100));
	ptnList[2].data.push_back(Code(200));
	ptnList[2].data.push_back(Code(Type::Move));
	ptnList[2].data.push_back(Code(-110));
	ptnList[2].data.push_back(Code(150));
	ptnList[2].data.push_back(Code(150));

	ptnList[2].data.push_back(Code(Type::Move));
	ptnList[2].data.push_back(Code(-100));
	ptnList[2].data.push_back(Code(200));
	ptnList[2].data.push_back(Code(175));
	ptnList[2].data.push_back(Code(Type::Move));
	ptnList[2].data.push_back(Code(100));
	ptnList[2].data.push_back(Code(400));
	ptnList[2].data.push_back(Code(200));
	ptnList[2].data.push_back(Code(Type::Move));
	ptnList[2].data.push_back(Code(110));
	ptnList[2].data.push_back(Code(450));
	ptnList[2].data.push_back(Code(150));

	ptnList[2].data.push_back(Code(Type::Move));
	ptnList[2].data.push_back(Code(100));
	ptnList[2].data.push_back(Code(500));
	ptnList[2].data.push_back(Code(175));
	ptnList[2].data.push_back(Code(Type::Move));
	ptnList[2].data.push_back(Code(-100));
	ptnList[2].data.push_back(Code(700));
	ptnList[2].data.push_back(Code(200));

	ptnList[3].name = "WaveR";
	ptnList[3].data.push_back(Code(Type::Move));
	ptnList[3].data.push_back(Code(100));
	ptnList[3].data.push_back(Code(150));
	ptnList[3].data.push_back(Code(200));
	ptnList[3].data.push_back(Code(Type::Move));
	ptnList[3].data.push_back(Code(-100));
	ptnList[3].data.push_back(Code(300));
	ptnList[3].data.push_back(Code(200));
	ptnList[3].data.push_back(Code(Type::Move));
	ptnList[3].data.push_back(Code(100));
	ptnList[3].data.push_back(Code(450));
	ptnList[3].data.push_back(Code(200));
	ptnList[3].data.push_back(Code(Type::Move));
	ptnList[3].data.push_back(Code(-100));
	ptnList[3].data.push_back(Code(600));
	ptnList[3].data.push_back(Code(200));
	ptnList[3].data.push_back(Code(Type::Move));
	ptnList[3].data.push_back(Code(100));
	ptnList[3].data.push_back(Code(750));
	ptnList[3].data.push_back(Code(200));
	ptnList[4].name = "ZigzagL";
	ptnList[4].data.push_back(Code(Type::Move));
	ptnList[4].data.push_back(Code(0));
	ptnList[4].data.push_back(Code(200));
	ptnList[4].data.push_back(Code(200));
	ptnList[4].data.push_back(Code(Type::Move));
	ptnList[4].data.push_back(Code(150));
	ptnList[4].data.push_back(Code(100));
	ptnList[4].data.push_back(Code(150));
	ptnList[4].data.push_back(Code(Type::Move));
	ptnList[4].data.push_back(Code(150));
	ptnList[4].data.push_back(Code(500));
	ptnList[4].data.push_back(Code(200));
	ptnList[4].data.push_back(Code(Type::Move));
	ptnList[4].data.push_back(Code(150));
	ptnList[4].data.push_back(Code(-100));
	ptnList[4].data.push_back(Code(300));
	ptnList[5].name = "ZigzagR";
	ptnList[5].data.push_back(Code(Type::Move));
	ptnList[5].data.push_back(Code(0));
	ptnList[5].data.push_back(Code(200));
	ptnList[5].data.push_back(Code(200));
	ptnList[5].data.push_back(Code(Type::Move));
	ptnList[5].data.push_back(Code(-150));
	ptnList[5].data.push_back(Code(100));
	ptnList[5].data.push_back(Code(150));
	ptnList[5].data.push_back(Code(Type::Move));
	ptnList[5].data.push_back(Code(-150));
	ptnList[5].data.push_back(Code(500));
	ptnList[5].data.push_back(Code(200));
	ptnList[5].data.push_back(Code(Type::Move));
	ptnList[5].data.push_back(Code(-150));
	ptnList[5].data.push_back(Code(-100));
	ptnList[5].data.push_back(Code(300));

	return ptnList;
}

} // namespcee B

/**
* アクションデータの種類.
*/
enum class Type
{
	Move,
	Accel,
	Wait,
	Path,
	ControlPoint,
	Vanishing,
	Generation,
	Animation,
	ManualControl,
};

/**
* 補間方法.
*
* 現在未実装.
*/
enum InterporationType
{
	InterporationType_Step,
	InterporationType_Linear,
	InterporationType_Ease,
	InterporationType_EaseIn,
	InterporationType_EaseOut,
};

enum MoveParamId
{
	MoveParamId_DirectionDegree,
	MoveParamId_Speed,
};

enum AccelParamId
{
	AccelParamId_DirectionDegree,
	AccelParamId_Accel,
};

enum WaitParamId
{
	WaitParamId_Time,
};

enum PathParamId
{
	PathParamId_Time,
	PathParamId_Count,
	PathParamId_Interporation,
};

enum ControlPointId
{
	ControlPointId_X,
	ControlPointId_Y,
};

enum GenParamId
{
	GenParamId_Speed,
	GenParamId_DirectionDegree,
	GenParamId_Option,
};

enum AnimeParamId
{
	AnimeParamId_Id,
};

/**
* アクションデータ.
*
* 個々のアクションを保持する.
*/
struct Data
{
	Type type; ///< アクションの種類.
	float param[3]; ///< パラメータ配列.
};

/**
* アクションシーケンス.
*
* アクションデータの配列.
*/
typedef std::vector<Data> Sequence;

/**
* アクションシーケンスのリスト.
*/
struct List {
	std::string name; ///< リスト名.
	std::vector<Sequence> list; ///< シーケンスの配列.
};
bool operator<(const List& lhs, const List& rhs) { return lhs.name < rhs.name; }
bool operator<(const List& lhs, const char* rhs) { return lhs.name.compare(rhs) < 0; }
bool operator<(const char* lhs, const List& rhs) { return rhs.name.compare(lhs) > 0; }

/**
* ド・ブーアのアルゴリズムによるBスプライン座標の生成.
*
* @param k      再帰呼び出しの深度. degreeで開始し、再帰呼出し毎にデクリメントされる. 0になったとき再帰は終了する.
* @param degree 次数.
* @param i      座標生成に使用する最初のコントロールポイントのインデックス.
* @param x      生成される座標の位置. 最初のコントロールポイントを0、最後のコントロールポイントをNとしたとき、
*               0～Nの値を取る.
* @param points コントロールポイントの配列.
*
* @return xに対応するBスプラインカーブ上の座標.
*/
XMVECTOR DeBoorI(int k, int degree, int i, float x, const std::vector<XMFLOAT2>& points) {
	if (k == 0) {
		return XMLoadFloat2(&points[std::max(0, std::min<int>(i, points.size() - 1))]);
	}
	const float alpha = (x - static_cast<float>(i)) / static_cast<float>(degree + 1 - k);
	const XMVECTOR a = DeBoorI(k - 1, degree, i - 1, x, points);
	const XMVECTOR b = DeBoorI(k - 1, degree, i, x, points);
	const XMVECTORF32 t{ alpha, (1.0f - alpha), 0.0f, 0.0f };
	return XMVectorAdd(
		XMVectorMultiply(a, XMVectorSwizzle(t, 1, 1, 1, 1)),
		XMVectorMultiply(b, XMVectorSwizzle(t, 0, 0, 0, 0)));
}

/**
*ド・ブーアのアルゴリズムによるBスプライン座標の生成.
*
* @param degree 次数.
* @param x      生成される座標の位置. 最初のコントロールポイントを0、最後のコントロールポイントをNとしたとき、
*               0～Nの値を取る.
* @param points コントロールポイントの配列.
*
* @return xに対応するBスプラインカーブ上の座標.
*/
XMVECTOR DeBoor(int degree, float x, const std::vector<XMFLOAT2>& points) {
	const int i = static_cast<int>(x);
	return DeBoorI(degree, degree, i, x, points);
}

/**
* 3次Bスプライン曲線を生成する.
*
* @param points        コントロールポイントの配列. 少なくとも3つのコントロールポイントを含んでいなければならない.
* @param numOfSegments 生成する中間点の数. pointsの数以上の値でなければならない.
*
* @return 生成された中間点の配列.
*/
std::vector<XMFLOAT2> CreateBSpline(const std::vector<XMFLOAT2>& points, int numOfSegments) {
	std::vector<XMFLOAT2> v;
	v.reserve(numOfSegments);
	const float n = static_cast<float>(points.size() + 1);
	for (int i = 0; i < numOfSegments - 1; ++i) {
		const float ratio = static_cast<float>(i) / static_cast<float>(numOfSegments - 1);
		const float x = ratio * n + 1;
		XMVECTORF32 pos;
		pos.v = DeBoor(3, x, points);
		v.emplace_back(pos.f[0], pos.f[1]);
	}
	v.push_back(points.back());

	// 同じベクトルを持つ区間を統合する.
	std::vector<XMVECTOR> vectorList;
	vectorList.reserve(v.size() - 1);
	for (size_t i = 1; i < v.size(); ++i) {
		vectorList.push_back(XMVector2Normalize(XMVectorSubtract(XMLoadFloat2(&v[i]), XMLoadFloat2(&v[i - 1]))));
	}
	std::vector<XMFLOAT2> ret;
	ret.reserve((v.size() + 1) / 2);
	ret.push_back(v.front());
	XMVECTOR vec = vectorList[0];
	for (size_t i = 1; i < vectorList.size(); ++i) {
		float r;
		XMStoreFloat(&r, XMVector2Dot(vec, vectorList[i]));
		if (r < 0.999f) {
			ret.push_back(v[i]);
			vec = vectorList[i];
		} else {
			vec = XMVector2Normalize(XMVectorSubtract(XMLoadFloat2(&v[i]), XMLoadFloat2(&ret.back())));
		}
	}
	ret.push_back(v.back());

	return ret;
}

/**
* 度数法の値を弧度法に変換する.
*
* @param degree 度数法の値.
*
* @return 弧度法におけるdegreeに対応する値.
*/
float DegreeToRadian(float degree)
{
	return (3.14159265359f * degree / 180.0f);
}

/**
* 
*
* @param rad 回転角(radian).
* @param mag ベクトルの大きさ.
*
* @return 2次元ベクトル(mag, 0)を0度とし、反時計回りにradだけ回転させたベクトルを返す.
*/
XMVECTOR RadianToVector(float rad, float mag)
{
	XMFLOAT2A tmp;
	XMScalarSinCos(&tmp.y, &tmp.x, rad);
	tmp.y *= -1.0f;
	return XMVectorMultiply(XMLoadFloat2A(&tmp), XMVectorSwizzle(XMLoadFloat(&mag), 0, 0, 0, 0));
}

/**
* コンストラクタ.
*/
Controller::Controller()
{
	SetList(nullptr);
}

/**
* コンストラクタ.
*
* @param p  設定するアクションリストへのポインタ.
* @param no 再生するアクション番号.
*/
Controller::Controller(const List* p, uint32_t no)
{
	SetList(p, no);
}

/**
* アクションリストを設定する.
*
* @param p  設定するアクションリストへのポインタ.
* @param no 再生するアクション番号.
*/
void Controller::SetList(const List* p, uint32_t no)
{
	list = p;
	seqIndex = 0;
	dataIndex = 0;
	currentTime = 0;
	totalTime = 0;
	isGeneratorActive = false;
	type = Type::ManualControl;
	move = XMFLOAT2(0, 0);
	accel = XMFLOAT2(0, 0);
	if (!list || no >= list->list.size() || dataIndex >= list->list[no].size()) {
		return;
	}
	seqIndex = no;
	Init();
}

/**
* アクションの現在の状態でコントローラを更新する.
*/
void Controller::Init(Sprite::Sprite* pSprite)
{
	if (!list || seqIndex >= list->list.size() || type == Type::Vanishing) {
		return;
	}
	for (; dataIndex < list->list[seqIndex].size(); ++dataIndex) {
		const Data& data = list->list[seqIndex][dataIndex];
		switch (data.type) {
		case Type::Move:
			type = Type::Move;
			XMStoreFloat2(&move, RadianToVector(DegreeToRadian(data.param[MoveParamId_DirectionDegree]), data.param[MoveParamId_Speed]));
			break;
		case Type::Accel:
			type = Type::Move;
			XMStoreFloat2(&accel, RadianToVector(DegreeToRadian(data.param[AccelParamId_DirectionDegree]), data.param[AccelParamId_Accel]));
			break;
		case Type::Wait:
			type = Type::Move;
			totalTime = data.param[WaitParamId_Time];
			return;
		case Type::Path: {
			type = Type::Path;
			totalTime = data.param[PathParamId_Time];
			path.type = static_cast<InterporationType>(static_cast<int>(data.param[PathParamId_Interporation]));
			std::vector<XMFLOAT2> controlPoints;
			controlPoints.reserve(static_cast<size_t>(data.param[PathParamId_Count]));
			for (int i = 0; i < data.param[PathParamId_Count]; ++i) {
				if (++dataIndex >= list->list[seqIndex].size()) {
					break;
				}
				const Data& cp = list->list[seqIndex][dataIndex];
				if (cp.type != Type::ControlPoint) {
					break;
				}
				controlPoints.emplace_back(cp.param[ControlPointId_X], cp.param[ControlPointId_Y]);
			}
			std::vector<XMFLOAT2> tmpPoints = CreateBSpline(controlPoints, controlPoints.size() * 16);
			std::vector<float> distances;
			path.cp.resize(tmpPoints.size());
			for (size_t i = 0; i < tmpPoints.size(); ++i) {
				path.cp[i].pos = tmpPoints[i];
			}
			path.cp[0].t = 0.0f;
			for (size_t i = 0; i < tmpPoints.size() - 1; ++i) {
				const XMVECTOR vec = XMVector2Length(XMVectorSubtract(XMLoadFloat2(&tmpPoints[i]), XMLoadFloat2(&tmpPoints[i + 1])));
				XMStoreFloat(&path.cp[i + 1].t, vec);
				path.cp[i + 1].t += path.cp[i].t;
			}
			const float factor = totalTime / path.cp.back().t;
			for (auto& e : path.cp) {
				e.t *= factor;
			}
			return;
		}
		case Type::Vanishing:
			type = Type::Vanishing;
			return;
		case Type::Animation:
			if (pSprite && data.param[AnimeParamId_Id] >= 0) {
				pSprite->SetSeqIndex(static_cast<uint32_t>(data.param[AnimeParamId_Id]));
			}
			break;
		case Type::Generation:
			if (pSprite && generator) {
				isGeneratorActive = true;
				generator(0.0f, pSprite, data.param[GenParamId_Speed], data.param[GenParamId_DirectionDegree]);
			}
			break;
		}
	}
}

/**
* アクションをスプライトに反映する.
*
* @param delta 更新時間(秒).
* @param pSprite アクションを反映するスプライトへのポインタ.
*/
void Controller::UpdateSub(float delta, Sprite::Sprite* pSprite)
{
	switch (type) {
	case Type::Move: {
		const XMVECTOR d = XMVectorSwizzle(XMLoadFloat(&delta), 0, 0, 1, 1);
		const XMVECTOR m = XMLoadFloat2(&move);
		const XMVECTOR a = XMLoadFloat2(&accel);
		XMStoreFloat3(&pSprite->pos, XMVectorAdd(XMLoadFloat3(&pSprite->pos), XMVectorMultiply(m, d)));
		XMStoreFloat2(&move, XMVectorAdd(m, XMVectorMultiply(a, d)));
		break;
	}
	case Type::Path: {
		const float t = currentTime + delta;
		if (t >= totalTime) {
			pSprite->pos.x = path.cp.back().pos.x;
			pSprite->pos.y = path.cp.back().pos.y;
		} else {
			const auto itr = std::upper_bound(path.cp.begin(), path.cp.end(), Point{ {}, t });
			if (itr != path.cp.end()) {
				const Point& p0 = *(itr - 1);
				const Point& p1 = *itr;
				const float length = p1.t - p0.t;
				const float distance = t - p0.t;
				const float ratio = distance / length;
				pSprite->pos.x = p0.pos.x * (1.0f - ratio) + p1.pos.x * ratio;
				pSprite->pos.y = p0.pos.y * (1.0f - ratio) + p1.pos.y * ratio;
			}
		}
		break;
	}
	default:
		break;
	}
}

/**
* アクションの状態を更新し、スプライトに反映する.
*
* @param delta 更新時間(秒).
* @param pSprite アクションを反映するスプライトへのポインタ.
*/
void Controller::Update(float delta, Sprite::Sprite* pSprite)
{
	if (type != Type::Vanishing && isGeneratorActive && generator) {
		generator(delta, pSprite, 0, 0);
	}
	if (type == Type::ManualControl) {
		const XMVECTOR d = XMVectorSwizzle(XMLoadFloat(&delta), 0, 0, 1, 1);
		XMStoreFloat3(&pSprite->pos, XMVectorAdd(XMLoadFloat3(&pSprite->pos), XMVectorMultiply(d, XMLoadFloat2(&move))));
		XMStoreFloat2(&move, XMVectorAdd(XMLoadFloat2(&move), XMVectorMultiply(d, XMLoadFloat2(&accel))));
		if (currentTime < totalTime) {
			currentTime += delta;
		}
		return;
	}
	if (!list || seqIndex >= list->list.size() || dataIndex >= list->list[seqIndex].size() || type == Type::Vanishing) {
		return;
	}
	UpdateSub(std::min(std::max(0.0f, totalTime - currentTime), delta), pSprite);
	currentTime += delta;
	while (currentTime >= totalTime) {
		currentTime -= totalTime;
		totalTime = 0.0f;
		delta = currentTime;
		++dataIndex;
		if (dataIndex >= list->list[seqIndex].size()) {
			return;
		}
		Init(pSprite);
		UpdateSub(std::min(std::max(0.0f, totalTime - currentTime), delta), pSprite);
	}
}

/**
* 再生するアクションシーケンスを指定する.
*
* @param no 再生するアクションシーケンスのインデックス.
*           保持しているシーケンス数以上の値は無視される(シーケンスは変更されない).
*/
void Controller::SetSeqIndex(uint32_t no)
{
	if (!list || no >= list->list.size()) {
		return;
	}
	seqIndex = no;
	dataIndex = 0;
	const Data& data = list->list[seqIndex][dataIndex];
	type = data.type;
	currentTime = 0.0f;
	totalTime = 0.0f;
	Init();
}

/**
* マニュアルモードの移動速度を設定する.
*
* @param degree 移動方向(0-360).
* @param speed  移動速度(pixels/s).
*/
void Controller::SetManualMove(float degree, float speed)
{
	type = Type::ManualControl;
	XMStoreFloat2(&move, RadianToVector(DegreeToRadian(degree), speed));
}

/**
* マニュアルモードの加速度を設定する.
*
* @param degree 加速方向(0-360).
* @param speed  加速度(pixels/s).
*/
void Controller::SetManualAccel(float degree, float speed)
{
	type = Type::ManualControl;
	XMStoreFloat2(&accel, RadianToVector(DegreeToRadian(degree), speed));
}

/**
* マニュアルモードの動作時間を設定する.
*
* @param time 動作時間.
*/
void Controller::SetTime(float time)
{
	type = Type::ManualControl;
	currentTime = 0.0f;
	totalTime = time;
}

/**
* 消滅状態になっているかどうか.
*
* @retval true  消滅状態になっている.
* @retval false 消滅状態になっていない.
*
* アクションシーケンスでType::Vanishingが指定されると消滅状態になり、アクションの再生が停止される.
*/
bool Controller::IsDeletable() const
{
	return type == Type::Vanishing;
}

/**
* Fileインターフェイスの実装クラス.
*/
class FileImpl : public File
{
public:
	FileImpl() {}
	virtual ~FileImpl() {}
	virtual const List* Get(uint32_t no) const {
		if (no >= actList.size()) {
			return nullptr;
		}
		return &actList[no];
	}
	virtual size_t Size() const { return actList.size(); }

	std::vector<List> actList;
};

/**
* ファイルからアクションリストを読み込む.
*
* @param filename ファイル名.
*
* @return 読み込んだアクションリスト.
*         読み込み失敗の場合はnullptrを返す.
*
* JSONフォーマットは次のとおり:
* <pre>
* [
*   {
*     "name" : "アクションリスト名",
*     "list" :
*     [
*       [
*         {
*           "type" : "アクションの種類を示す文字列",
*           "args" : [arg0, arg1, arg2]
*         },
*         ...
*       ],
*       ...
*     ]
*   },
*   ...
* ]
* </pre>
*/
FilePtr LoadFromJsonFile(const wchar_t* filename)
{
	struct HandleHolder {
		explicit HandleHolder(HANDLE h) : handle(h) {}
		~HandleHolder() { if (handle != INVALID_HANDLE_VALUE) { CloseHandle(handle); } }
		HANDLE handle;
		operator HANDLE() { return handle; }
		operator HANDLE() const { return handle; }
	};

	std::shared_ptr<FileImpl> af(new FileImpl);

	HandleHolder h(CreateFileW(filename, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
	if (h == INVALID_HANDLE_VALUE) {
		return af;
	}
	LARGE_INTEGER size;
	if (!GetFileSizeEx(h, &size)) {
		return af;
	}
	if (size.QuadPart > std::numeric_limits<size_t>::max()) {
		return af;
	}
	std::vector<char> buffer;
	buffer.resize(static_cast<size_t>(size.QuadPart));
	DWORD readBytes;
	if (!ReadFile(h, &buffer[0], buffer.size(), &readBytes, nullptr)) {
		return af;
	}
	const Json::Result result = Json::Parse(buffer.data(), buffer.data() + buffer.size());
	if (!result.error.empty()) {
		OutputDebugStringA(result.error.c_str());
		return af;
	}
	const Json::Array& json = result.value.AsArray();
	for (const Json::Value& e : json) {
		const Json::Object& object = e.AsObject();
		List al;

		const Json::Object::const_iterator itrName = object.find("name");
		if (itrName != object.end()) {
			al.name = itrName->second.AsString();
		}
		const Json::Object::const_iterator itrList = object.find("list");
		if (itrList == object.end()) {
			break;
		}
		const Json::Array& seqList = itrList->second.AsArray();
		for (const Json::Value& valSeq : seqList) {
			const Json::Array& seq = valSeq.AsArray();
			Sequence as;
			for (const Json::Value& data : seq) {
				const Json::Object& obj = data.AsObject();
				Data ad = {};
				static const struct {
					const char* const str;
					Type type;
					bool operator==(const std::string& s) const { return s == str; }
				} typeMap[] = {
					{ "Move", Type::Move },
					{ "Accel", Type::Accel },
					{ "Wait", Type::Wait },
					{ "Generate", Type::Generation },
					{ "Animation", Type::Animation },
					{ "Delete", Type::Vanishing },
				};
				const auto itrTypePair = std::find(typeMap, typeMap + _countof(typeMap), obj.find("type")->second.AsString());
				if (itrTypePair == typeMap + _countof(typeMap)) {
					return af;
				}
				ad.type = itrTypePair->type;
				const Json::Array& array = obj.find("args")->second.AsArray();
				for (size_t i = 0; i < 3 && i < array.size(); ++i) {
					ad.param[i] = static_cast<float>(array[i].AsNumber());
				}
				as.push_back(ad);
			}
			al.list.push_back(as);
		}
		af->actList.push_back(al);
	}

	return af;
}

} // namespace Action 
