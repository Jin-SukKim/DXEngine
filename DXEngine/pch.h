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

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <directxtk/SimpleMath.h>

#pragma comment(lib, "d3d11.lib")

using Microsoft::WRL::ComPtr;

using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Quaternion;

#include "Common.h"
#include "Vertex.h"
#include "ConstantData.h"
#include "D3D11Utils.h"
#include "InputTypes.h"

