#pragma once

#ifdef _DEBUG
//#pragma comment(lib, "libFolder\\Debug\\libName.lib")
#else
//#pragma comment(lib, "libFolder\\Release\\libName.lib")
#endif
//#define _CRT_SECURE_NO_WARNINGS

#include <array>
#include <vector>
#include <memory>
#include <iostream>
#include <functional>
#include <algorithm>
#include <unordered_map>
#include <string>
#include <random>
#include <fstream>
#include <filesystem>
#include <set>
#include <queue>
#include <map>
#include <unordered_set>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <directxtk/SimpleMath.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <nlohmann/json.hpp>

#pragma comment(lib, "d3d11.lib")

using Microsoft::WRL::ComPtr;

using Vector2 = DirectX::SimpleMath::Vector2;
using Vector3 = DirectX::SimpleMath::Vector3;
using Vector4 = DirectX::SimpleMath::Vector4;
using Matrix = DirectX::SimpleMath::Matrix;
using Quaternion = DirectX::SimpleMath::Quaternion;
using nlohmann::json;

#include "Common.h"
#include "Vertex.h"
#include "ConstantData.h"
#include "D3D11Utils.h"
#include "Texture2D.h"
#include "InputTypes.h"
#include "RenderBase.h"
#include "Utils.h"
#include "ScopedTimer.h"
