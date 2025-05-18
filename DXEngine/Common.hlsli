#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

cbuffer GlobalConsts : register(b0) {
    matrix view;
    matrix proj;
};

#endif // __COMMON_HLSLI__