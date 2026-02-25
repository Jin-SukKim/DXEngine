#pragma once
#include "EffectActor.h"
#include "SwordClashEffect.h"

namespace DE {
    class SwordBurstEffect : public EffectActor
    {
        using Super = EffectActor;
    public:
        SwordBurstEffect(const std::wstring& name);
        virtual ~SwordBurstEffect() override = default;

        void Initialize() override;
        void RepositionChildren();  // 부모 위치 확정 후 외부에서 호출
        void Update(const float& dt) override;
        void SpawnNext();
        bool NeedsExternalPreset() const override { return false; }
    private:
        SwordClashEffect m_clash1, m_clash2, m_clash3, m_clash4, m_clash5, m_clash6, m_clash7, m_clash8, m_clash9, m_clash10;
        float m_timer = 0.f;
        int   m_spawnIndex = 0;
        static constexpr float kSpawnInterval = 0.035f; // 각 충돌 사이 간격 (초)
        static constexpr int   kTotalClashes = 10;
    };
}
