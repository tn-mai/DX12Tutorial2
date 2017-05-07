/**
* @file Json.cpp
*
* JSON�f�[�^����͂���p�[�T.
*/
#include "Json.h"

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
* JSON�f�[�^��͊�.
*/
class Parser
{
public:
	Parser(const char* d, const char* e);
	void AddError(const std::string& err);
	void SkipSpace();
	Value ParseString();
	Value ParseObject();
	Value ParseArray();
	Value ParseValue();
	Result Parse();

private:
	const char* data; ///< ��͒��̈ʒu�ւ̃|�C���^.
	const char* const end; ///< JSON�f�[�^�̏I�[�������|�C���^.
	int line; ///< ��͒��̍s��.
	std::string error; ///< ���������G���[�̏��.
};

/**
* �R���X�g���N�^.
*
* @param d JSON�f�[�^�̉�͊J�n�ʒu�������|�C���^.
* @param e  JSON�f�[�^�̏I�[�������|�C���^.
*/
Parser::Parser(const char* d, const char* e) : data(d), end(e), line(0)
{
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
* ���������͂���.
*
* @return ��������i�[����Value�^�I�u�W�F�N�g.
*/
Value Parser::ParseString()
{
	++data; // skip first double quotation.

	std::string s;
	for (; *data != '"'; ++data) {
		if (data == end) {
			AddError("(ParseString) ������̏I�[��'\"'������܂���");
			return Value();
		}
		s.push_back(static_cast<char>(*data));
	}
	++data; // skip last double quotation.
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
	if (*data == '}') {
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
		if (*data != ':') {
			AddError(std::string("(ParseObject) ':'���K�v�ł�: '") + *data + "'");
			return Value();
		}
		++data; // skip colon.
		SkipSpace();
		const Value value = ParseValue();
		obj.insert(std::make_pair(key.string, value));

		SkipSpace();
		if (*data == '}') {
			++data; // skip last brace.
			break;
		}
		if (*data != ',') {
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
	if (*data == ']') {
		++data;
		return Value(Array());
	}

	Array arr;
	for (;;) {
		const Value value = ParseValue();
		arr.push_back(value);
		SkipSpace();
		if (*data == ']') {
			++data; // skip last bracket.
			break;
		}
		if (*data != ',') {
			AddError(std::string("(ParseArray) ','���K�v�ł�: '") + *data + "'");
			return Value();
		}
		++data; // skip comma.
		SkipSpace();
	}
	return Value(arr);
}

/**
* ���͕����ɑΉ�����JSON�I�u�W�F�N�g����͂���.
*
* @return ���͕����ɑΉ�����JSON�I�u�W�F�N�g���i�[����Value�^�I�u�W�F�N�g.
*/
Value Parser::ParseValue()
{
	SkipSpace();
	if (*data == '{') {
		return ParseObject();
	}
	if (*data == '[') {
		return ParseArray();
	}
	if (*data == '"') {
		return ParseString();
	}
	if (strcmp(data, "true") == 0) {
		data += 4;
		return Value(true);
	}
	if (strcmp(data, "false") == 0) {
		data += 5;
		return Value(false);
	}
	if (strcmp(data, "null") == 0) {
		data += 4;
		return Value();
	}
	{
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
* JSON�f�[�^����͂���.
*/
Result Parser::Parse()
{
	Value value = ParseValue();
	SkipSpace();
	if (data != end) {
		AddError(std::string("(Parse) ��͕s�\�ȕ���������܂�: '") + *data + "'");
	}
	return { value, error };
}
/**
* JSON�f�[�^����͂���.
*
* @param data JSON�f�[�^�̉�͊J�n�ʒu�������|�C���^.
* @param end  JSON�f�[�^�̏I�[�������|�C���^.
*
* @return ���͕����ɑΉ�����JSON�I�u�W�F�N�g���i�[����Value�^�I�u�W�F�N�g.
*/
Result Parse(const char* data, const char* end)
{
	Parser parser(data, end);
	return parser.Parse();
}

} // namespace Json
