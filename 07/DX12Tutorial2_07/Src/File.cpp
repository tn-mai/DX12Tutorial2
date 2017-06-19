/**
* @file File.cpp
*/
#include "File.h"
#include <Windows.h>

/**
* �t�@�C���𑀍삷��@�\���i�[���閼�O���.
*/
namespace File {

/**
* �t�@�C����ǂݍ���.
*
* @param filename �t�@�C����.
* @param buffer   �ǂݍ��ݐ�o�b�t�@.
*
* @retval true  �ǂݍ��ݐ���.
* @retval false �ǂݍ��ݎ��s.
*/
bool Read(const wchar_t* filename, BufferType& buffer)
{
	HANDLE h = CreateFileW(filename, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (h == INVALID_HANDLE_VALUE) {
		OutputDebugStringW((std::wstring(L"ERROR: '") + filename + L"'�̃I�[�v���Ɏ��s\n").c_str());
		return false;
	}
	LARGE_INTEGER size;
	if (!GetFileSizeEx(h, &size)) {
		CloseHandle(h);
		OutputDebugStringW((std::wstring(L"ERROR: '") + filename + L"'�̃t�@�C���T�C�Y�擾�Ɏ��s\n").c_str());
		return false;
	}
	buffer.resize(static_cast<size_t>(size.QuadPart));
	DWORD readBytes;
	const BOOL result = ReadFile(h, buffer.data(), buffer.size(), &readBytes, nullptr);
	CloseHandle(h);
	if (!result || readBytes != size.QuadPart) {
		OutputDebugStringW((std::wstring(L"ERROR: '") + filename + L"'�̓ǂݍ��݂Ɏ��s\n").c_str());
		return false;
	}
	return true;
}

} // namespace File