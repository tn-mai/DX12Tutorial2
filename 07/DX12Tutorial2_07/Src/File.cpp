/**
* @file File.cpp
*/
#include "File.h"
#include <Windows.h>

/**
* ファイルを操作する機能を格納する名前空間.
*/
namespace File {

/**
* ファイルを読み込む.
*
* @param filename ファイル名.
* @param buffer   読み込み先バッファ.
*
* @retval true  読み込み成功.
* @retval false 読み込み失敗.
*/
bool Read(const wchar_t* filename, BufferType& buffer)
{
	HANDLE h = CreateFileW(filename, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (h == INVALID_HANDLE_VALUE) {
		OutputDebugStringW((std::wstring(L"ERROR: '") + filename + L"'のオープンに失敗\n").c_str());
		return false;
	}
	LARGE_INTEGER size;
	if (!GetFileSizeEx(h, &size)) {
		CloseHandle(h);
		OutputDebugStringW((std::wstring(L"ERROR: '") + filename + L"'のファイルサイズ取得に失敗\n").c_str());
		return false;
	}
	buffer.resize(static_cast<size_t>(size.QuadPart));
	DWORD readBytes;
	const BOOL result = ReadFile(h, buffer.data(), buffer.size(), &readBytes, nullptr);
	CloseHandle(h);
	if (!result || readBytes != size.QuadPart) {
		OutputDebugStringW((std::wstring(L"ERROR: '") + filename + L"'の読み込みに失敗\n").c_str());
		return false;
	}
	return true;
}

} // namespace File