#pragma once

namespace DE {
class SimplexNoise
{
public:
    // 3D Simplex noise, returns [-1, 1]
    static float Noise3D(float x, float y, float z);

private:
    static const int perm[512];
    static const int grad3[12][3];
    static float Dot(const int g[3], float x, float y, float z);
};
}
