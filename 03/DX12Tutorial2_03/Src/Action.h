/**
* @file Action.h
*/
#ifndef DX12TUTORIAL_SRC_ACTION_H_
#define DX12TUTORIAL_SRC_ACTION_H_
#include <DirectXMath.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace Sprite {
struct Sprite;
} // namespace Sprite

/**
* スプライトの座標を制御するための、アクションと呼ばれる機能に関する名前空間.
*/
namespace Action {

namespace B {

/**
* 行動の種類.
*/
enum class Type : uint32_t
{
	Move,
	Stop,
	Bezier,
};

/**
* 移動データ型.
*/
union Code
{
	Code() {}
	explicit Code(Type t) : opcode(t) {}
	explicit Code(float f) : operand(f) {}
	Code(const Code&) = default;
	~Code() = default;
	Code& operator=(const Code&) = default;

	Type opcode;
	float operand;
};

/**
* 移動パターン.
*/
struct Pattern {
	std::string name; ///< 移動パターン名.
	std::vector<Code> data; ///< 移動データ配列.
};

/// 移動パターンリスト.
typedef std::vector<Pattern> PatternList;

/**
* 移動制御クラス.
*/
class Controller
{
public:
	Controller();
	Controller(const Controller&) = default;
	~Controller() = default;
	Controller& operator=(const Controller&) = default;

	void SetPattern(const Pattern* p);
	void Update(Sprite::Sprite& sprite, double delta);

private:
	const Pattern* pattern;
	size_t codeCounter;
	DirectX::XMVECTOR basePos;
	DirectX::XMVECTOR startPos;
	double time;
};

} // namespace B

struct List;
enum class Type;
enum InterporationType;

/**
* オブジェクト生成関数型.
*/
typedef std::function<void(float, Sprite::Sprite*, float, float)> GeneratorType;

/**
* パス生成のためのコントロールポイント型.
*/
struct Point {
	DirectX::XMFLOAT2 pos;
	float t;
	bool operator<(const Point& p) const { return t < p.t; }
};

/**
* アクションを制御するクラス.
*/
class Controller
{
public:
	Controller();
	Controller(const List* l, uint32_t no = 0);
	void SetList(const List*, uint32_t no = 0);
	void SetGenerator(GeneratorType gen) { generator = gen; }
	void Update(float delta, Sprite::Sprite*);
	void SetSeqIndex(uint32_t no);
	void SetManualMove(float degree, float speed);
	void SetManualMove(const DirectX::XMFLOAT2& m) { move = m; }
	void SetManualAccel(float degree, float accel);
	void SetTime(float time);
	float GetCurrentTime() const { return currentTime; }
	float GetTotalTime() const { return totalTime; }
	uint32_t GetSeqIndex() const { return seqIndex; }
	bool IsDeletable() const;

private:
	void Init(Sprite::Sprite* = nullptr);
	void UpdateSub(float delta, Sprite::Sprite*);

private:
	const List* list;
	uint32_t seqIndex;
	uint32_t dataIndex;
	Type type;

	float totalTime;
	float currentTime;

	DirectX::XMFLOAT2 move;
	DirectX::XMFLOAT2 accel;
	struct PathParam {
		std::vector<Point> cp;
		InterporationType type;
	} path;

	bool isGeneratorActive;
	GeneratorType generator;
};

/**
* 複数のアクションリストをまとめたオブジェクトを操作するためのインターフェイスクラス.
*
* LoadFromJsonFile()関数を使ってインターフェイスに対応したオブジェクトを取得し、Get()で
* 個々のアクションリストにアクセスする.
*/
class File
{
public:
	File() = default;
	File(const File&) = delete;
	File& operator=(const File&) = delete;
	virtual ~File() {}

	virtual const List* Get(uint32_t no) const = 0;
	virtual size_t Size() const = 0;
};
typedef std::shared_ptr<File> FilePtr;

FilePtr LoadFromJsonFile(const wchar_t*);

} // namespace Action

#endif // DX12TUTORIAL_SRC_ACTION_H_