#pragma once
#include "Actor.h"

namespace DE {
    class LightActor : public Actor
    {
        using Super = Actor;

    public:

    private:
        float m_lightFov = 90.f;
        float m_nearZ = 0.1f;
        float m_farZ = 10.f;
        const float m_aspectRatio = 1.f;


    };

}