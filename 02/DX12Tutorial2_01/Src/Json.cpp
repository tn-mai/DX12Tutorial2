/**
* @file Json.cpp
*
* JSONデータを解析するパーサ.
*/
#include "Json.h"

/**
* JSONパーザ.
*/
namespace Json {

/**
* デフォルトコンストラクタ.
*
* null値として初期化.
*/
Value::Value() : type(Type::Null) {
}

/**
* デストラクタ.
*/
Value::~Value() {
	switch (type) {
	case Type::String: string.~basic_string(); break;
	case Type::Number: break;
	case Type::Boolean: break;
	case Type::Null: break;
	case Type::Object: object.~map(); break;
	case Type::Array: array.~vector(); break;
	}
}

/**
* コピーコンストラクタ.
*
* @param v コピー元オブジェクト.
*/
Value::Value(const Value& v) {
	type = v.type;
	switch (type) {
	case Type::String: new(&string) String(v.string); break;
	case Type::Number: new(&number) Number(v.number); break;
	case Type::Boolean: new(&boolean) Boolean(v.boolean); break;
	case Type::Null: break;
	case Type::Object: new(&object) Object(v.object); break;
	case Type::Array: new(&array) Array(v.array); break;
	}
}

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

void SkipSpace(const char*& data, const char* end);
Result ParseString(const char*& data, const char* end);
Result ParseNumber(const char*& data, const char* end);
Result ParseBoolean(const char*& data, const char* end);
Result ParseNull(const char*& data, const char* end);
Result ParseObject(const char*& data, const char* end);
Result ParseArray(const char*& data, const char* end);
Result ParseValue(const char*& data, const char* end);

/**
* 空白文字をスキップする.
*
* @param data JSONデータの解析位置を示すポインタ.
*/
void SkipSpace(const char*& data, const char* end)
{
	for (; data != end; ++data) {
		switch (*data) {
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			break;
		default:
			goto end;
		}
	}
end:
	return;
}

/**
* トークンの終端が正常かどうか調べる.
*/
bool IsEndOfValidToken(const char* p, const char* end)
{
	SkipSpace(p, end);
	return p == end || *p == ',' || *p == '}' || *p == ']';
}

/**
* 文字列を解析する.
*
* @param data JSONデータの解析位置を示すポインタ.
*
* @return 文字列を格納したValue型オブジェクト.
*/
Result ParseString(const char*& data, const char* end)
{
	++data; // skip first double quotation.

	std::string s;
	for (; *data != '"'; ++data) {
		if (data == end) {
			return { Value(), "予期しないEOF" };
		}
		s.push_back(static_cast<char>(*data));
	}
	++data; // skip last double quotation.
	return { Value(s) };
}

/**
* 数値を解析する.
*
* @param data JSONデータの解析位置を示すポインタ.
*
* @return 数値を格納したValue型オブジェクト.
*/
Result ParseNumber(const char*& data, const char* end)
{
	char* endPtr;
	const double d = strtod(data, &endPtr);
	if (IsEndOfValidToken(endPtr, end)) {
		data = endPtr;
		return { Value(d) };
	}
	return { Value(), "解析不能なトークン" };
}

/**
* JSONオブジェクトを解析する.
*
* @param data JSONデータの解析位置を示すポインタ.
*
* @return JSONオブジェクトを格納したValue型オブジェクト.
*/
Result ParseObject(const char*& data, const char* end)
{
	++data; // skip first brace.
	SkipSpace(data, end);
	if (*data == '}') {
		++data;
		return { Value(Object()) };
	}

	Object obj;
	for (;;) {
		const Result result = ParseString(data, end);
		if (!result.error.empty()) {
			return result;
		}
		const std::string key = result.value.string;
		SkipSpace(data, end);
		if (*data != ':') {
			return { Value(), std::string("不正なトークン: ") + *data };
		}
		++data; // skip colon.
		SkipSpace(data, end);
		const Result value = ParseValue(data, end);
		if (!value.error.empty()) {
			return value;
		}
		obj.insert(std::make_pair(key, value.value));

		SkipSpace(data, end);
		if (*data == '}') {
			++data; // skip last brace.
			break;
		}
		if (*data != ',') {
			return { Value(), std::string("不正なトークン: ") + *data };
		}
		++data; // skip comma.
		SkipSpace(data, end);
	}
	return { Value(obj) };
}

/**
* JSON配列を解析する.
*
* @param data JSONデータの解析位置を示すポインタ.
*
* @return JSON配列を格納したValue型オブジェクト.
*/
Result ParseArray(const char*& data, const char* end)
{
	++data; // skip first bracket.
	SkipSpace(data, end);
	if (*data == ']') {
		++data;
		return { Value(Array()) };
	}

	Array arr;
	for (;;) {
		const Result result = ParseValue(data, end);
		if (!result.error.empty()) {
			return result;
		}
		arr.push_back(result.value);
		SkipSpace(data, end);
		if (*data == ']') {
			++data; // skip last bracket.
			break;
		}
		if (*data != ',') {
			return { Value(), std::string("予期しないトークン: ") + *data };
		}
		++data; // skip comma.
		SkipSpace(data, end);
	}
	return { Value(arr) };
}

/**
* 入力文字に対応するJSONオブジェクトを解析する.
*
* @param data JSONデータの解析位置を示すポインタ.
*
* @return 入力文字に対応するJSONオブジェクトを格納したValue型オブジェクト.
*/
Result ParseValue(const char*& data, const char* end)
{
	SkipSpace(data, end);
	if (*data == '{') {
		return ParseObject(data, end);
	}
	if (*data == '[') {
		return ParseArray(data, end);
	}
	if (*data == '"') {
		return ParseString(data, end);
	}
	if (strcmp(data, "true") == 0) {
		const char* tmp = data + 4;
		SkipSpace(tmp, end);
		if (tmp == end || *tmp == ',' || *tmp == '}' || *tmp == ']') {
			data = tmp;
			return { Value(true) };
		}
	}
	if (strcmp(data, "false") == 0) {
		const char* tmp = data + 5;
		SkipSpace(tmp, end);
		if (tmp == end || *tmp == ',' || *tmp == '}' || *tmp == ']') {
			data = tmp;
			return { Value(false) };
		}
	}
	if (strcmp(data, "null") == 0) {
		const char* tmp = data + 4;
		SkipSpace(tmp, end);
		if (tmp == end || *tmp == ',' || *tmp == '}' || *tmp == ']') {
			data = tmp;
			return { Value() };
		}
	}
	return ParseNumber(data, end);
}

/**
* JSONデータを解析する.
*
* @param data JSONデータの解析開始位置を示すポインタ.
*
* @return 入力文字に対応するJSONオブジェクトを格納したValue型オブジェクト.
*/
Result Parse(const char* data, const char* end)
{
	return ParseValue(data, end);
}

} // namespace Json
