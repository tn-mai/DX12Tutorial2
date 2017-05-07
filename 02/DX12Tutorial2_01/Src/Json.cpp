/**
* @file Json.cpp
*
* JSON�f�[�^����͂���p�[�T.
*/
#include "Json.h"

/**
* JSON�p�[�U.
*/
namespace Json {

/**
* �f�t�H���g�R���X�g���N�^.
*
* null�l�Ƃ��ď�����.
*/
Value::Value() : type(Type::Null) {
}

/**
* �f�X�g���N�^.
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
* �R�s�[�R���X�g���N�^.
*
* @param v �R�s�[���I�u�W�F�N�g.
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

struct Context
{
	Context(const char* d, const char* e) : data(d), end(e), line(0) {}
	void AddError(const std::string& err) { error += std::to_string(line) + ": " + err + "\n"; }

	const char* data;
	const char* end;
	int line;
	std::string error;
};

void SkipSpace(Context& context);
Value ParseString(Context& context);
Value ParseObject(Context& context);
Value ParseArray(Context& context);
Value ParseValue(Context& context);

/**
* �󔒕������X�L�b�v����.
*
* @param data JSON�f�[�^�̉�͈ʒu�������|�C���^.
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
* ���������͂���.
*
* @param data JSON�f�[�^�̉�͈ʒu�������|�C���^.
*
* @return ��������i�[����Value�^�I�u�W�F�N�g.
*/
Value ParseString(Context& context)
{
	++context.data; // skip first double quotation.

	std::string s;
	for (; *context.data != '"'; ++context.data) {
		if (context.data == context.end) {
			context.AddError("(ParseString) ������̏I�[��'\"'������܂���");
			return Value();
		}
		s.push_back(static_cast<char>(*context.data));
	}
	++context.data; // skip last double quotation.
	return Value(s);
}

/**
* JSON�I�u�W�F�N�g����͂���.
*
* @param data JSON�f�[�^�̉�͈ʒu�������|�C���^.
*
* @return JSON�I�u�W�F�N�g���i�[����Value�^�I�u�W�F�N�g.
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
			context.AddError(std::string("(ParseObject) ������łȂ��L�[������܂�: '") + *context.data + "'");
			return Value();
		}
		const Value key = ParseString(context);
		SkipSpace(context);
		if (*context.data != ':') {
			context.AddError(std::string("(ParseObject) ':'���K�v�ł�: '") + *context.data + "'");
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
			context.AddError(std::string("(ParseObject) ','���K�v�ł�: '") + *context.data + "'");
			return Value();
		}
		++context.data; // skip comma.
		SkipSpace(context);
	}
	return Value(obj);
}

/**
* JSON�z�����͂���.
*
* @param data JSON�f�[�^�̉�͈ʒu�������|�C���^.
*
* @return JSON�z����i�[����Value�^�I�u�W�F�N�g.
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
			context.AddError(std::string("(ParseArray) ','���K�v�ł�: '") + *context.data + "'");
			return Value();
		}
		++context.data; // skip comma.
		SkipSpace(context);
	}
	return Value(arr);
}

/**
* ���͕����ɑΉ�����JSON�I�u�W�F�N�g����͂���.
*
* @param data JSON�f�[�^�̉�͈ʒu�������|�C���^.
*
* @return ���͕����ɑΉ�����JSON�I�u�W�F�N�g���i�[����Value�^�I�u�W�F�N�g.
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
			context.AddError(std::string("(ParseValue) ��͕s�\�ȕ���������܂�: '") + *context.data + "'");
			return Value();
		}
		context.data = endPtr;
		return Value(d);
	}
}

/**
* JSON�f�[�^����͂���.
*
* @param data JSON�f�[�^�̉�͊J�n�ʒu�������|�C���^.
*
* @return ���͕����ɑΉ�����JSON�I�u�W�F�N�g���i�[����Value�^�I�u�W�F�N�g.
*/
Result Parse(const char* data, const char* end)
{
	Context context(data, end);
	Value value = ParseValue(context);
	SkipSpace(context);
	if (context.data != context.end) {
		context.AddError(std::string("(Parse) ��͕s�\�ȕ���������܂�: '") + *context.data + "'");
	}
	return{ value, context.error };
}

} // namespace Json
