/**
* @file Json.cpp
*
* JSON�f�[�^����͂���p�[�T.
*/
#include "Json.h"
#include <string.h>
#include <stdlib.h>

/**
* JSON�p�[�T.
*/
namespace Json {

/**
* �f�t�H���g�R���X�g���N�^.
*
* null�l�Ƃ��ď�����.
*/
Value::Value() : type(Type::Null) {}

/**
* �I�u�W�F�N�g�^�Ƃ��ăR���X�g���N�g����.
*
* @param o �I�u�W�F�N�g.
*/
Value::Value(const Object& o) : type(Type::Object) { new(&object) Object(o); }

/**
* �z��^�Ƃ��ăR���X�g���N�g����.
*
* @param a �z��.
*/
Value::Value(const Array& a) : type(Type::Array) { new(&array) Array(a); }

/**
* ������^�Ƃ��ăR���X�g���N�g����.
*
* @param s ������.
*/
Value::Value(const std::string& s) : type(Type::String) { new(&string) String(s); }

/**
* ���l�^�Ƃ��ăR���X�g���N�g����.
*
* @param d ���l.
*/
Value::Value(double d) : type(Type::Number) { new(&number) Number(d); }

/**
* �^�U�l�Ƃ��ăR���X�g���N�g����.
*
* @param b �^�U�l.
*/
Value::Value(bool b) : type(Type::Boolean) { new(&boolean) Boolean(b); }

/**
* �R�s�[�R���X�g���N�^.
*
* @param v �R�s�[���I�u�W�F�N�g.
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
* �f�X�g���N�^.
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
* �f�[�^�^���擾����.
*
* @return �f�[�^�̎��.
*/
Type Value::GetType() const
{
	return type;
}

/**
* ������Ƃ��ăA�N�Z�X.
*
* @return ������f�[�^.
*/
const String& Value::AsString() const
{
	static const String dummy;
	return type == Type::String ? string : dummy;
}

/**
* ���l�Ƃ��ăA�N�Z�X.
*
* @return ���l�f�[�^.
*/
Number Value::AsNumber() const
{
	return type == Type::Number ? number : 0;
}

/**
* �^�U�l�Ƃ��ăA�N�Z�X.
*
* @return �^�U�l�f�[�^.
*/
Boolean Value::AsBoolean() const
{
	return type == Type::Boolean ? boolean : false;
}

/**
* �I�u�W�F�N�g�Ƃ��ăA�N�Z�X.
*
* @return �I�u�W�F�N�g�f�[�^.
*/
const Object& Value::AsObject() const
{
	static const Object dummy;
	return type == Type::Object ? object : dummy;
}

/**
* �z��Ƃ��ăA�N�Z�X.
*
* @return �z��f�[�^.
*/
const Array& Value::AsArray() const
{
	static const Array dummy;
	return type == Type::Array ? array : dummy;
}

/**
* JSON�f�[�^��͊�.
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
	const char* data; ///< ��͒��̈ʒu�ւ̃|�C���^.
	const char* end; ///< JSON�f�[�^�̏I�[�������|�C���^.
	int line; ///< ��͒��̍s��.
	std::string error; ///< ���������G���[�̏��.
};

/**
* JSON�f�[�^����͂���.
*
* @param d JSON�f�[�^�̉�͊J�n�ʒu�������|�C���^.
* @param e JSON�f�[�^�̏I�[�������|�C���^.
*
* @return Result�^�̉�͌��ʃI�u�W�F�N�g.
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
		AddError(std::string("(Parse) ��͕s�\�ȕ���������܂�: '") + *data + "'");
	}
	return { value, error };
}

/**
* �G���[����ǉ�.
*
* @param err �G���[�̓��e������������.
*
* err�̐擪�ɍs�ԍ���t�^���A�G���[�o�b�t�@�ɒǉ�����.
*/
void Parser::AddError(const std::string& err)
{
	error += std::to_string(line) + ": " + err + "\n";
}

/**
* �󔒕������X�L�b�v����.
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
* �l����͂���.
*
* @return �l���i�[����Value�^�I�u�W�F�N�g.
*/
Value Parser::ParseValue()
{
	SkipSpace();
	if (data == end) {
		AddError("(ParseValue) ��͒��Ƀf�[�^�I�[�ɓ��B���܂���");
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
			AddError(std::string("(ParseValue) ��͕s�\�ȕ���������܂�: '") + *data + "'");
			return Value();
		}
		data = endPtr;
		return Value(d);
	}
}

/**
* ���������͂���.
*
* @return ��������i�[����Value�^�I�u�W�F�N�g.
*/
Value Parser::ParseString()
{
	++data; // skip first double quotation.

	std::string s;
	for (;;) {
		if (data == end) {
			AddError("(ParseString) ������̏I�[��'\"'������܂���");
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
* JSON�I�u�W�F�N�g����͂���.
*
* @return JSON�I�u�W�F�N�g���i�[����Value�^�I�u�W�F�N�g.
*/
Value Parser::ParseObject()
{
	++data; // skip first brace.
	SkipSpace();
	if (data == end) {
		AddError("(ParseObject) �I�u�W�F�N�g�̏I�[��'}'������܂���");
		return Value();
	} else if (*data == '}') {
		++data;
		return Value(Object());
	}

	Object obj;
	for (;;) {
		if (*data != '"') {
			AddError(std::string("(ParseObject) ������łȂ��L�[������܂�: '") + *data + "'");
			return Value();
		}
		const Value key = ParseString();

		SkipSpace();
		if (data == end) {
			AddError("(ParseObject) ':'���K�v�ł�");
			return Value();
		} else if (*data != ':') {
			AddError(std::string("(ParseObject) ':'���K�v�ł�: '") + *data + "'");
			return Value();
		}
		++data; // skip colon.
		
		SkipSpace();
		const Value value = ParseValue();
		obj.insert(std::make_pair(key.string, value));

		SkipSpace();
		if (data == end) {
			AddError("(ParseObject) �I�u�W�F�N�g�̏I�[��'}'������܂���");
			return Value();
		} else if (*data == '}') {
			++data; // skip last brace.
			break;
		} else if (*data != ',') {
			AddError(std::string("(ParseObject) ','���K�v�ł�: '") + *data + "'");
			return Value();
		}
		++data; // skip comma.
		SkipSpace();
	}
	return Value(obj);
}

/**
* JSON�z�����͂���.
*
* @return JSON�z����i�[����Value�^�I�u�W�F�N�g.
*/
Value Parser::ParseArray()
{
	++data; // skip first bracket.
	SkipSpace();
	if (data == end) {
		AddError("(ParseArray) �z��̏I�[��']'������܂���");
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
			AddError("(ParseArray) �z��̏I�[��']'������܂���");
			return Value();
		} else if (*data == ']') {
			++data; // skip last bracket.
			break;
		} else if (*data != ',') {
			AddError(std::string("(ParseArray) ','���K�v�ł�: '") + *data + "'");
			return Value();
		}
		++data; // skip comma.
		SkipSpace();
	}
	return Value(arr);
}

/**
* JSON�f�[�^����͂���.
*
* @param data JSON�f�[�^�̉�͊J�n�ʒu�������|�C���^.
* @param end  JSON�f�[�^�̏I�[�������|�C���^.
*
* @return Result�^�̉�͌��ʃI�u�W�F�N�g.
*/
Result Parse(const char* data, const char* end)
{
	Parser parser;
	return parser.Parse(data, end);
}

} // namespace Json
