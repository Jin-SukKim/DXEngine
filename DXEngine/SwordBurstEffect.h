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
        bool NeedsExternalPreset() const override { return false; }
    private:
        SwordClashEffect m_clash2, m_clash3, m_clash4, m_clash5, m_clash6;
    };
}
