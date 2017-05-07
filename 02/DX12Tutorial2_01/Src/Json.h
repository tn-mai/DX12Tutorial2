/**
* @file Json.h
*/
#ifndef DX12TUTORIAL_SRC_JSON_H_
#define DX12TUTORIAL_SRC_JSON_H_
#include <string>
#include <vector>
#include <map>

namespace Json {

class Value;
typedef std::string String;
typedef double Number;
typedef bool Boolean;
typedef std::map<std::string, Value> Object;
typedef std::vector<Value> Array;

/**
* Valueが実際に保持している型を識別するための列挙型.
*/
enum class Type
{
	Object, ///< オブジェクト型.
	Array, ///< 配列型.
	String, ///< 文字列型.
	Number, ///< 数値型.
	Boolean, ///< 真偽値.
	Null, ///< null値.
};

/**
* JSONの値を格納する汎用型.
*
* typeに対応したメンバ変数にアクセスすることで実際の値が得られる.
* typeと異なるメンバ変数にアクセスした場合の動作は未定義.
*/
class Value
{
public:
	Value();
	Value(const Object& o);
	Value(const Array& a);
	Value(const std::string& s);
	Value(double d);
	Value(bool b);
	Value(const Value& v);
	~Value();

	/**
	* コピー代入演算子.
	*
	* @param v コピー元オブジェクト.
	*/
	template<typename T>
	Value& operator=(const T& v) {
		(*this).~Value();
		new(this) Value(v);
		return *this;
	}

public:
	Type type;
	union {
		String string;
		Number number;
		Boolean boolean;
		Object object;
		Array array;
	};
};

/**
* パース結果.
*
* errorが空ならパース成功.
* 空でなければ何らかのエラーが発生している.
* errorには発生したエラーの情報が行単位で格納されているので、OutputDebugStringA等で出力できる.
*/
struct Result
{
	Value value; ///< JSONオブジェクト.
	std::string error; ///< エラー情報.
};

Result Parse(const char* data, const char* end);

} // namespace Json

#endif // DX12TUTORIAL_SRC_JSON_H_