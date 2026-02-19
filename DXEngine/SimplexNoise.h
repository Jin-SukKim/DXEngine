#pragma once

namespace DE {

    class SimplexNoise
    {
    public:
        explicit SimplexNoise(unsigned int seed = 0);

        float Noise2D(float x, float y) const;
        float Noise3D(float x, float y, float z) const;

    private:
        int perm[512];

        static const int grad3[12][3];

        static float Dot(const int g[3], float x, float y, float z = 0.0f);
    };

} // namespace DE