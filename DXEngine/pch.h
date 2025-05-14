#pragma once

#ifdef _DEBUG
//#pragma comment(lib, "libFolder\\Debug\\libName.lib")
#else
//#pragma comment(lib, "libFolder\\Release\\libName.lib")
#endif

#include <vector>
#include <memory>
#include <iostream>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include "Common.h"

#pragma comment(lib, "d3d11.lib")