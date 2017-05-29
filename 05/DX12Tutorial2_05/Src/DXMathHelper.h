/**
* @file DXMathHelper.h
*/
#ifndef DXTUTORIAL_SRC_DXMATHHELPER_H_INCLUDED
#define DXTUTORIAL_SRC_DXMATHHELPER_H_INCLUDED
#include <DirectXMath.h>

// XMVECTOR—p‰‰ŽZŽq‚È‚Ç
DirectX::XMVECTOR XXXX(DirectX::XMVECTOR v) { return DirectX::XMVectorSplatX(v); }
DirectX::XMVECTOR YYYY(DirectX::XMVECTOR v) { return DirectX::XMVectorSplatY(v); }
DirectX::XMVECTOR ZZZZ(DirectX::XMVECTOR v) { return DirectX::XMVectorSplatZ(v); }
DirectX::XMVECTOR WWWW(DirectX::XMVECTOR v) { return DirectX::XMVectorSplatW(v); }

#endif // DXTUTORIAL_SRC_DXMATHHELPER_H_INCLUDED