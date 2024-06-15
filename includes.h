#pragma once

#pragma warning( disable : 4307 ) // '*': integral constant overflow
#pragma warning( disable : 4244 ) // possible loss of data
#pragma warning( disable : 4800 ) // forcing value to bool 'true' or 'false'
#pragma warning( disable : 4838 ) // conversion from '::size_t' to 'int' requires a narrowing conversion

// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.
#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

using ulong_t = unsigned long;

// windows / stl includes.
#include <Windows.h>
#include <cstdint>
#include <intrin.h>
#include <xmmintrin.h>
#include <array>
#include <vector>
#include <algorithm>
#include <cctype>
#include <string>
#include <chrono>
#include <thread>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <deque>
#include <functional>
#include <map>
#include <shlobj.h>
#include <filesystem>
#include <streambuf>
#include <corecrt_math.h>

// our custom wrapper.
#include "unique_vector.h"
#include "tinyformat.h"

// other includes.
#include "hash.h"
#include "xorstr.h"
#include "pe.h"
#include "winapir.h"
#include "address.h"
#include "util.h"
#include "modules.h"
#include "pattern.h"
#include "vmt.h"
#include "stack.h"
#include "nt.h"
#include "x86.h"
#include "syscall.h"
#include "clua.h"
#include "clua_hooks.h"
#include "imgui.h"
#include "imgui_freetype.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3dx9.lib")

// hack includes.
#include "interfaces.h"
#include "sdk.h"
#include "csgo.h"
#include "netvars.h"
#include "entoffsets.h"
#include "entity.h"
#include "client.h"
#include "gamerules.h"
#include "hooks.h"
#include "render.h"
#include "notify.h"
#include "IWS2000.h"
#include "iwsrecord.h"
#include "ray_tracer.h"
#include "rebuilt_animations.hpp"
#include "m200_resolver.hpp"
#include "bonesetup.h"

// gui includes.
#include "json.h"
#include "base64.h"
#include "config.h"
#include "cmenu.hpp"