/**
* @file Json.cpp
*
* JSONデータを解析するパーサ.
*/
#include "Json.h"

/**
* JSONパーサ.
*/
namespace Json {

/**
* デフォルトコンストラクタ.
*
* null値として初期化.
*/
Value::Value() : type(Type::Null) {}

/**
* オブジェクト型としてコンストラクトする.
*
* @param o オブジェクト.
*/
Value::Value(const Object& o) : type(Type::Object) { new(&object) Object(o); }

/**
* 配列型としてコンストラクトする.
*
* @param a 配列.
*/
Value::Value(const Array& a) : type(Type::Array) { new(&array) Array(a); }

/**
* 文字列型としてコンストラクトする.
*
* @param s 文字列.
*/
Value::Value(const std::string& s) : type(Type::String) { new(&string) String(s); }

/**
* 数値型としてコンストラクトする.
*
* @param d 数値.
*/
Value::Value(double d) : type(Type::Number) { new(&number) Number(d); }

/**
* 真偽値としてコンストラクトする.
*
* @param b 真偽値.
*/
Value::Value(bool b) : type(Type::Boolean) { new(&boolean) Boolean(b); }

/**
* コピーコンストラクタ.
*
* @param v コピー元オブジェクト.
*/
Value::Value(const Value& v) {
	type = v.type;
	switch (type) {
	case Type::Object: new(&object) Object(v.object); break;
	case Type::Array: new(&array) Array(v.array); break;
	case Type::String: new(&string) String(v.string); break;
	case Type::Number: new(&number) Number(v.number); break;
	case Type::Boolean: new(&boolean) Boolean(v.boolean); break;
	case Type::Null: break;
	}
}

/**
* デストラクタ.
*/
Value::~Value() {
	switch (type) {
	case Type::Object: object.~Object(); break;
	case Type::Array: array.~Array(); break;
	case Type::String: string.~String(); break;
	case Type::Number: number.~Number();  break;
	case Type::Boolean: boolean.~Boolean();  break;
	case Type::Null: break;
	}
}

/**
* 解析状態.
*/
struct Context
{
	/**
	* コンストラクタ.
	*
	* @param d JSONデータの解析開始位置を示すポインタ.
	* @param e  JSONデータの終端を示すポインタ.
	*/
	Context(const char* d, const char* e) : data(d), end(e), line(0) {}

	/**
	* エラー情報を追加.
	*
	* @param err エラーの内容を示す文字列.
	*
	* errの先頭に行番号を付与し、エラーバッファに追加する.
	*/
	void AddError(const std::string& err) { error += std::to_string(line) + ": " + err + "\n"; }

	const char* data; ///< 解析中の位置へのポインタ.
	const char* const end; ///< JSONデータの終端を示すポインタ.
	int line; ///< 解析中の行数.
	std::string error; ///< 発生したエラーの情報.
};

void SkipSpace(Context& context);
Value ParseString(Context& context);
Value ParseObject(Context& context);
Value ParseArray(Context& context);
Value ParseValue(Context& context);

/**
* 空白文字をスキップする.
*
* @param data JSONデータの解析位置を示すポインタ.
*/
void SkipSpace(Context& context)
{
	for (; context.data != context.end; ++context.data) {
		switch (*context.data) {
		case ' ':
		case '\t':
		case '\r':
			break;
		case '\n':
			++context.line;
			break;
		default:
			goto end;
		}
	}
end:
	return;
}

/**
* 文字列を解析する.
*
* @param data JSONデータの解析位置を示すポインタ.
*
* @return 文字列を格納したValue型オブジェクト.
*/
Value ParseString(Context& context)
{
	++context.data; // skip first double quotation.

	std::string s;
	for (; *context.data != '"'; ++context.data) {
		if (context.data == context.end) {
			context.AddError("(ParseString) 文字列の終端に'\"'がありません");
			return Value();
		}
		s.push_back(static_cast<char>(*context.data));
	}
	++context.data; // skip last double quotation.
	return Value(s);
}

/**
* JSONオブジェクトを解析する.
*
* @param data JSONデータの解析位置を示すポインタ.
*
* @return JSONオブジェクトを格納したValue型オブジェクト.
*/
Value ParseObject(Context& context)
{
	++context.data; // skip first brace.
	SkipSpace(context);
	if (*context.data == '}') {
		++context.data;
		return Value(Object());
	}

	Object obj;
	for (;;) {
		if (*context.data != '"') {
			context.AddError(std::string("(ParseObject) 文字列でないキーがあります: '") + *context.data + "'");
			return Value();
		}
		const Value key = ParseString(context);
		SkipSpace(context);
		if (*context.data != ':') {
			context.AddError(std::string("(ParseObject) ':'が必要です: '") + *context.data + "'");
			return Value();
		}
		++context.data; // skip colon.
		SkipSpace(context);
		const Value value = ParseValue(context);
		obj.insert(std::make_pair(key.string, value));

		SkipSpace(context);
		if (*context.data == '}') {
			++context.data; // skip last brace.
			break;
		}
		if (*context.data != ',') {
			context.AddError(std::string("(ParseObject) ','が必要です: '") + *context.data + "'");
			return Value();
		}
		++context.data; // skip comma.
		SkipSpace(context);
	}
	return Value(obj);
}

/**
* JSON配列を解析する.
*
* @param data JSONデータの解析位置を示すポインタ.
*
* @return JSON配列を格納したValue型オブジェクト.
*/
Value ParseArray(Context& context)
{
	++context.data; // skip first bracket.
	SkipSpace(context);
	if (*context.data == ']') {
		++context.data;
		return Value(Array());
	}

	Array arr;
	for (;;) {
		const Value value = ParseValue(context);
		arr.push_back(value);
		SkipSpace(context);
		if (*context.data == ']') {
			++context.data; // skip last bracket.
			break;
		}
		if (*context.data != ',') {
			context.AddError(std::string("(ParseArray) ','が必要です: '") + *context.data + "'");
			return Value();
		}
		++context.data; // skip comma.
		SkipSpace(context);
	}
	return Value(arr);
}

/**
* 入力文字に対応するJSONオブジェクトを解析する.
*
* @param data JSONデータの解析位置を示すポインタ.
*
* @return 入力文字に対応するJSONオブジェクトを格納したValue型オブジェクト.
*/
Value ParseValue(Context& context)
{
	SkipSpace(context);
	if (*context.data == '{') {
		return ParseObject(context);
	}
	if (*context.data == '[') {
		return ParseArray(context);
	}
	if (*context.data == '"') {
		return ParseString(context);
	}
	if (strcmp(context.data, "true") == 0) {
		context.data += 4;
		return Value(true);
	}
	if (strcmp(context.data, "false") == 0) {
		context.data += 5;
		return Value(false);
	}
	if (strcmp(context.data, "null") == 0) {
		context.data += 4;
		return Value();
	}
	{
		char* endPtr;
		const double d = strtod(context.data, &endPtr);
		if (context.data == endPtr) {
			context.AddError(std::string("(ParseValue) 解析不能な文字があります: '") + *context.data + "'");
			return Value();
		}
		context.data = endPtr;
		return Value(d);
	}
}

/**
* JSONデータを解析する.
*
* @param data JSONデータの解析開始位置を示すポインタ.
* @param end  JSONデータの終端を示すポインタ.
*
* @return 入力文字に対応するJSONオブジェクトを格納したValue型オブジェクト.
*/
Result Parse(const char* data, const char* end)
{
	Context context(data, end);
	Value value = ParseValue(context);
	SkipSpace(context);
	if (context.data != context.end) {
		context.AddError(std::string("(Parse) 解析不能な文字があります: '") + *context.data + "'");
	}
	return{ value, context.error };
}

} // namespace Json
