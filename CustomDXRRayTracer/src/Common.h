#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include <dxgi1_6.h>
#include <d3d12.h>
#include <dxc/dxcapi.h>
#include <dxc/dxcapi.use.h>

#include <string>
#include <vector>

#define NAME_D3D_RESOURCES 1
#define SAFE_RELEASE( x ) { if ( x ) { x->Release(); x = NULL; } }
#define SAFE_DELETE( x ) { if ( x ) delete x; x = NULL; }
#define SAFE_DELETE_ARRAY( x ) { if ( x ) delete[] x; x = NULL; }
#define	ALIGN(_alignement, _val) (((_val + _alignement - 1) / _alignement) * _alignement)