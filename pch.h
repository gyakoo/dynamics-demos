/**
*  @file    pch.cpp
*  @author  Manu Marin gyakoo@gmail.com
*
*  @brief PCH and Utils namespace declaration
*
*/
#pragma once

// Trying to include LEAN Microsft headers
#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOMCX
#define NOSERVICE
#define NOHELP
//#define NODRAWTEXT
//#define NOGDI
//#define NOBITMAP

// -- WINDOWS and STL
#include <wrl.h>
#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
//#include <collection.h>
#include <memory>
//#include <agile.h>
#include <concrt.h>
#include <algorithm>
#include <queue>
#include <iterator>
#include <utility>
#include <array>
#include <map>
#include <chrono>
#include <thread>
#include <ppl.h>
#include <atomic>
#include <concurrent_vector.h>
#include <ppltasks.h>	// For create_task
#include <locale>
#include <codecvt>
#include <functional> 
#include <cctype>
#include <iostream>
#include <sstream>
#include <random>

#pragma warning(pop)


namespace DX {}; // to compile using clausule below

// -- others
#include <d3d11.h>
#include <d3dcompiler.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_internal.h>
#include <SimpleMath.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <xmmintrin.h>

// -- GLOBAL USING 
using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;

#if !defined(_DEBUG) && defined(OutputDebugString)
#undef OutputDebugString
#define OutputDebugString(a) __debugbreak()
#endif

template<typename V>
inline V clamp(const V& v, const V& t, const V& q)
{
    return v < t ? t : (v>q ? q : v);
}
