/**
* @file Json.cpp
*
* JSONデータを解析するパーサ.
*/
#include "Json.h"
#include <string.h>
#include <stdlib.h>

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
* データ型を取得する.
*
* @return データの種類.
*/
Type Value::GetType() const
{
	return type;
}

/**
* 文字列としてアクセス.
*
* @return 文字列データ.
*/
const String& Value::AsString() const
{
	static const String dummy;
	return type == Type::String ? string : dummy;
}

/**
* 数値としてアクセス.
*
* @return 数値データ.
*/
Number Value::AsNumber() const
{
	return type == Type::Number ? number : 0;
}

/**
* 真偽値としてアクセス.
*
* @return 真偽値データ.
*/
Boolean Value::AsBoolean() const
{
	return type == Type::Boolean ? boolean : false;
}

/**
* オブジェクトとしてアクセス.
*
* @return オブジェクトデータ.
*/
const Object& Value::AsObject() const
{
	static const Object dummy;
	return type == Type::Object ? object : dummy;
}

/**
* 配列としてアクセス.
*
* @return 配列データ.
*/
const Array& Value::AsArray() const
{
	static const Array dummy;
	return type == Type::Array ? array : dummy;
}

/**
* JSONデータ解析器.
*/
class Parser
{
public:
	Parser() = default;
	~Parser() = default;
	Parser(const Parser&) = delete;
	Parser& operator=(const Parser&) = delete;

	Result Parse(const char* d, const char* e);

private:
	void AddError(const std::string& err);
	void SkipSpace();
	Value ParseValue();
	Value ParseString();
	Value ParseObject();
	Value ParseArray();

private:
	const char* data; ///< 解析中の位置へのポインタ.
	const char* end; ///< JSONデータの終端を示すポインタ.
	int line; ///< 解析中の行数.
	std::string error; ///< 発生したエラーの情報.
};

/**
* JSONデータを解析する.
*
* @param d JSONデータの解析開始位置を示すポインタ.
* @param e JSONデータの終端を示すポインタ.
*
* @return Result型の解析結果オブジェクト.
*/
Result Parser::Parse(const char* d, const char* e)
{
	data = d;
	end = e;
	line = 0;
	error.clear();

	Value value = ParseValue();
	SkipSpace();
	if (data != end) {
		AddError(std::string("(Parse) 解析不能な文字があります: '") + *data + "'");
	}
	return { value, error };
}

/**
* エラー情報を追加.
*
* @param err エラーの内容を示す文字列.
*
* errの先頭に行番号を付与し、エラーバッファに追加する.
*/
void Parser::AddError(const std::string& err)
{
	error += std::to_string(line) + ": " + err + "\n";
}

/**
* 空白文字をスキップする.
*/
void Parser::SkipSpace()
{
	for (; data != end; ++data) {
		const char c = *data;
		if (c == ' ' || c == '\t' || c == '\r') {
			/* EMPTY */
		} else if (c == '\n') {
			++line;
		} else {
			break;
		}
	}
}

/**
* 値を解析する.
*
* @return 値を格納したValue型オブジェクト.
*/
Value Parser::ParseValue()
{
	SkipSpace();
	if (data == end) {
		AddError("(ParseValue) 解析中にデータ終端に到達しました");
		return Value();
	} else if (*data == '{') {
		return ParseObject();
	} else if (*data == '[') {
		return ParseArray();
	} else if (*data == '"') {
		return ParseString();
	} else if (strcmp(data, "true") == 0) {
		data += 4;
		return Value(true);
	} else if (strcmp(data, "false") == 0) {
		data += 5;
		return Value(false);
	} else if (strcmp(data, "null") == 0) {
		data += 4;
		return Value();
	} else {
		char* endPtr;
		const double d = strtod(data, &endPtr);
		if (data == endPtr) {
			AddError(std::string("(ParseValue) 解析不能な文字があります: '") + *data + "'");
			return Value();
		}
		data = endPtr;
		return Value(d);
	}
}

/**
* 文字列を解析する.
*
* @return 文字列を格納したValue型オブジェクト.
*/
Value Parser::ParseString()
{
	++data; // skip first double quotation.

	std::string s;
	for (;;) {
		if (data == end) {
			AddError("(ParseString) 文字列の終端に'\"'がありません");
			return Value();
		} else if (*data == '"') {
			++data; // skip last double quotation.
			break;
		}
		s.push_back(static_cast<char>(*data));
		++data;
	}
	return Value(s);
}

/**
* JSONオブジェクトを解析する.
*
* @return JSONオブジェクトを格納したValue型オブジェクト.
*/
Value Parser::ParseObject()
{
	++data; // skip first brace.
	SkipSpace();
	if (data == end) {
		AddError("(ParseObject) オブジェクトの終端に'}'がありません");
		return Value();
	} else if (*data == '}') {
		++data;
		return Value(Object());
	}

	Object obj;
	for (;;) {
		if (*data != '"') {
			AddError(std::string("(ParseObject) 文字列でないキーがあります: '") + *data + "'");
			return Value();
		}
		const Value key = ParseString();

		SkipSpace();
		if (data == end) {
			AddError("(ParseObject) ':'が必要です");
			return Value();
		} else if (*data != ':') {
			AddError(std::string("(ParseObject) ':'が必要です: '") + *data + "'");
			return Value();
		}
		++data; // skip colon.
		
		SkipSpace();
		const Value value = ParseValue();
		obj.insert(std::make_pair(key.string, value));

		SkipSpace();
		if (data == end) {
			AddError("(ParseObject) オブジェクトの終端に'}'がありません");
			return Value();
		} else if (*data == '}') {
			++data; // skip last brace.
			break;
		} else if (*data != ',') {
			AddError(std::string("(ParseObject) ','が必要です: '") + *data + "'");
			return Value();
		}
		++data; // skip comma.
		SkipSpace();
	}
	return Value(obj);
}

/**
* JSON配列を解析する.
*
* @return JSON配列を格納したValue型オブジェクト.
*/
Value Parser::ParseArray()
{
	++data; // skip first bracket.
	SkipSpace();
	if (data == end) {
		AddError("(ParseArray) 配列の終端に']'がありません");
		return Value();
	} else if (*data == ']') {
		++data;
		return Value(Array());
	}

	Array arr;
	for (;;) {
		const Value value = ParseValue();
		arr.push_back(value);
		SkipSpace();
		if (data == end) {
			AddError("(ParseArray) 配列の終端に']'がありません");
			return Value();
		} else if (*data == ']') {
			++data; // skip last bracket.
			break;
		} else if (*data != ',') {
			AddError(std::string("(ParseArray) ','が必要です: '") + *data + "'");
			return Value();
		}
		++data; // skip comma.
		SkipSpace();
	}
	return Value(arr);
}

/**
* JSONデータを解析する.
*
* @param data JSONデータの解析開始位置を示すポインタ.
* @param end  JSONデータの終端を示すポインタ.
*
* @return Result型の解析結果オブジェクト.
*/
Result Parse(const char* data, const char* end)
{
	Parser parser;
	return parser.Parse(data, end);
}

} // namespace Json
