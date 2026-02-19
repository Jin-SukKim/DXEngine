#include "pch.h"
#include "SimplexNoise.h"
#include <cmath>
#include <numeric>
#include <algorithm>
#include <random>

namespace DE {

    const int SimplexNoise::grad3[12][3] = {
        { 1, 1, 0}, {-1, 1, 0}, { 1,-1, 0}, {-1,-1, 0},
        { 1, 0, 1}, {-1, 0, 1}, { 1, 0,-1}, {-1, 0,-1},
        { 0, 1, 1}, { 0,-1, 1}, { 0, 1,-1}, { 0,-1,-1}
    };

    // 생성자 
    SimplexNoise::SimplexNoise(unsigned int seed)
    {
        int base[256];
        std::iota(base, base + 256, 0);
        std::shuffle(base, base + 256, std::mt19937(seed));
        for (int i = 0; i < 256; ++i)
            perm[i] = perm[i + 256] = base[i];
    }

    // 감쇠 함수 헬퍼 (반지름 r2 안에서만 기여)
    static float Contrib(float r2, const int* g, float x, float y, float z = 0.0f)
    {
        float t = r2 - x * x - y * y - z * z;
        if (t < 0.0f) return 0.0f;
        t *= t;
        return t * t * (g[0] * x + g[1] * y + g[2] * z);
    }

    // 2D Simplex Noise
    float SimplexNoise::Noise2D(float x, float y) const
    {
        constexpr float F2 = 0.5f * (1.7320508f - 1.0f);
        constexpr float G2 = (3.0f - 1.7320508f) / 6.0f;

        int i = (int)std::floor(x + (x + y) * F2);
        int j = (int)std::floor(y + (x + y) * F2);

        float t = (i + j) * G2;
        float x0 = x - (i - t), y0 = y - (j - t);

        int i1 = (x0 > y0) ? 1 : 0, j1 = 1 - i1;

        float x1 = x0 - i1 + G2, y1 = y0 - j1 + G2;
        float x2 = x0 - 1.0f + 2.0f * G2, y2 = y0 - 1.0f + 2.0f * G2;

        int ii = i & 255, jj = j & 255;
        int gi0 = perm[ii + perm[jj]] % 12;
        int gi1 = perm[ii + i1 + perm[jj + j1]] % 12;
        int gi2 = perm[ii + 1 + perm[jj + 1]] % 12;

        return 70.0f * (Contrib(0.5f, grad3[gi0], x0, y0)
            + Contrib(0.5f, grad3[gi1], x1, y1)
            + Contrib(0.5f, grad3[gi2], x2, y2));
    }

    // 3D Simplex Noise
    float SimplexNoise::Noise3D(float x, float y, float z) const
    {
        constexpr float F3 = 1.0f / 3.0f;
        constexpr float G3 = 1.0f / 6.0f;

        float s = (x + y + z) * F3;
        int i = (int)std::floor(x + s);
        int j = (int)std::floor(y + s);
        int k = (int)std::floor(z + s);

        float t = (i + j + k) * G3;
        float x0 = x - (i - t), y0 = y - (j - t), z0 = z - (k - t);

        int i1, j1, k1, i2, j2, k2;
        if (x0 >= y0) {
            if (y0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
            else if (x0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1; }
            else { i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1; }
        }
        else {
            if (y0 < z0) { i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1; }
            else if (x0 < z0) { i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1; }
            else { i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
        }

        float x1 = x0 - i1 + G3, y1 = y0 - j1 + G3, z1 = z0 - k1 + G3;
        float x2 = x0 - i2 + 2.0f * G3, y2 = y0 - j2 + 2.0f * G3, z2 = z0 - k2 + 2.0f * G3;
        float x3 = x0 - 1.0f + 3.0f * G3, y3 = y0 - 1.0f + 3.0f * G3, z3 = z0 - 1.0f + 3.0f * G3;

        int ii = i & 255, jj = j & 255, kk = k & 255;
        int gi0 = perm[ii + perm[jj + perm[kk]]] % 12;
        int gi1 = perm[ii + i1 + perm[jj + j1 + perm[kk + k1]]] % 12;
        int gi2 = perm[ii + i2 + perm[jj + j2 + perm[kk + k2]]] % 12;
        int gi3 = perm[ii + 1 + perm[jj + 1 + perm[kk + 1]]] % 12;

        return 32.0f * (Contrib(0.6f, grad3[gi0], x0, y0, z0)
            + Contrib(0.6f, grad3[gi1], x1, y1, z1)
            + Contrib(0.6f, grad3[gi2], x2, y2, z2)
            + Contrib(0.6f, grad3[gi3], x3, y3, z3));
    }

} // namespace DE