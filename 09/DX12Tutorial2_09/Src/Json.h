/**
* @file Json.h
*/
#ifndef DX12TUTORIAL_SRC_JSON_H_
#define DX12TUTORIAL_SRC_JSON_H_
#include <string>
#include <vector>
#include <unordered_map>
#include <type_traits>

namespace Json {

class Value;
typedef std::string String;
typedef double Number;
typedef bool Boolean;
typedef std::unordered_map<String, Value> Object;
typedef std::vector<Value> Array;

/**
* Value�����ۂɕێ����Ă���^�����ʂ��邽�߂̗񋓌^.
*/
enum class Type
{
	Object, ///< �I�u�W�F�N�g�^.
	Array, ///< �z��^.
	String, ///< ������^.
	Number, ///< ���l�^.
	Boolean, ///< �^�U�l.
	Null, ///< null�l.
};

/**
* JSON�̒l���i�[����ėp�^.
*
* type�ɑΉ����������o�ϐ��ɃA�N�Z�X���邱�ƂŎ��ۂ̒l��������.
* type�ƈقȂ郁���o�ϐ��ɃA�N�Z�X�����ꍇ�̓���͖���`.
*/
class Value
{
	friend class Parser;
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
	* �R�s�[������Z�q.
	*
	* @param v �R�s�[���I�u�W�F�N�g.
	*/
	template<typename T>
	Value& operator=(const T& v) {
		(*this).~Value();
		new(this) Value(v);
		return *this;
	}

	Type GetType() const;
	const String& AsString() const;
	Number AsNumber() const;
	Boolean AsBoolean() const;
	const Object& AsObject() const;
	const Array& AsArray() const;

	/// �Z�p�^T�Ƃ��ăA�N�Z�X.
	template<typename T>
	T AsNumber() const {
		static_assert(std::is_arithmetic<T>::value, "T should be the arithmetic type.");
		return static_cast<T>(AsNumber());
	}

private:
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
* �p�[�X����.
*
* error����Ȃ�p�[�X����.
* ��łȂ���Ή��炩�̃G���[���������Ă���.
* error�ɂ͔��������G���[�̏�񂪍s�P�ʂŊi�[����Ă���̂ŁAOutputDebugStringA���ŏo�͂ł���.
*/
struct Result
{
	Value value; ///< JSON�I�u�W�F�N�g.
	std::string error; ///< �G���[���.
};

Result Parse(const char* data, const char* end);

} // namespace Json

#endif // DX12TUTORIAL_SRC_JSON_H_