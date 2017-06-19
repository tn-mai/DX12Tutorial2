/**
* @file File.h
*/
#ifndef DX12TUTORIAL_SRC_FILE_H_
#define DX12TUTORIAL_SRC_FILE_H_
#include <vector>

namespace File {

/// ファイル読み込みバッファ型.
typedef std::vector<char> BufferType;

bool Read(const wchar_t* filename, BufferType& buffer);

} // namespace File

#endif // DX12TUTORIAL_SRC_FILE_H_