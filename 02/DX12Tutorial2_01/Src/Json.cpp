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

void SkipSpace(const char*& data, const char* end);
Result ParseString(const char*& data, const char* end);
Result ParseNumber(const char*& data, const char* end);
Result ParseBoolean(const char*& data, const char* end);
Result ParseNull(const char*& data, const char* end);
Result ParseObject(const char*& data, const char* end);
Result ParseArray(const char*& data, const char* end);
Result ParseValue(const char*& data, const char* end);

/**
* �󔒕������X�L�b�v����.
*
* @param data JSON�f�[�^�̉�͈ʒu�������|�C���^.
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
* �g�[�N���̏I�[�����킩�ǂ������ׂ�.
*/
bool IsEndOfValidToken(const char* p, const char* end)
{
	SkipSpace(p, end);
	return p == end || *p == ',' || *p == '}' || *p == ']';
}

/**
* ���������͂���.
*
* @param data JSON�f�[�^�̉�͈ʒu�������|�C���^.
*
* @return ��������i�[����Value�^�I�u�W�F�N�g.
*/
Result ParseString(const char*& data, const char* end)
{
	++data; // skip first double quotation.

	std::string s;
	for (; *data != '"'; ++data) {
		if (data == end) {
			return { Value(), "�\�����Ȃ�EOF" };
		}
		s.push_back(static_cast<char>(*data));
	}
	++data; // skip last double quotation.
	return { Value(s) };
}

/**
* ���l����͂���.
*
* @param data JSON�f�[�^�̉�͈ʒu�������|�C���^.
*
* @return ���l���i�[����Value�^�I�u�W�F�N�g.
*/
Result ParseNumber(const char*& data, const char* end)
{
	char* endPtr;
	const double d = strtod(data, &endPtr);
	if (IsEndOfValidToken(endPtr, end)) {
		data = endPtr;
		return { Value(d) };
	}
	return { Value(), "��͕s�\�ȃg�[�N��" };
}

/**
* JSON�I�u�W�F�N�g����͂���.
*
* @param data JSON�f�[�^�̉�͈ʒu�������|�C���^.
*
* @return JSON�I�u�W�F�N�g���i�[����Value�^�I�u�W�F�N�g.
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
			return { Value(), std::string("�s���ȃg�[�N��: ") + *data };
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
			return { Value(), std::string("�s���ȃg�[�N��: ") + *data };
		}
		++data; // skip comma.
		SkipSpace(data, end);
	}
	return { Value(obj) };
}

/**
* JSON�z�����͂���.
*
* @param data JSON�f�[�^�̉�͈ʒu�������|�C���^.
*
* @return JSON�z����i�[����Value�^�I�u�W�F�N�g.
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
			return { Value(), std::string("�\�����Ȃ��g�[�N��: ") + *data };
		}
		++data; // skip comma.
		SkipSpace(data, end);
	}
	return { Value(arr) };
}

/**
* ���͕����ɑΉ�����JSON�I�u�W�F�N�g����͂���.
*
* @param data JSON�f�[�^�̉�͈ʒu�������|�C���^.
*
* @return ���͕����ɑΉ�����JSON�I�u�W�F�N�g���i�[����Value�^�I�u�W�F�N�g.
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
* JSON�f�[�^����͂���.
*
* @param data JSON�f�[�^�̉�͊J�n�ʒu�������|�C���^.
*
* @return ���͕����ɑΉ�����JSON�I�u�W�F�N�g���i�[����Value�^�I�u�W�F�N�g.
*/
Result Parse(const char* data, const char* end)
{
	return ParseValue(data, end);
}

} // namespace Json
