#include "pch.h"
#include "SwordBurstEffect.h"
#include "ParticleManager.h"
#include "TransformComponent.h"

namespace DE {

    static constexpr wchar_t kSwordJson[] =
        L"Particles\\Effects\\Combination\\SwordClash\\System_SwordClash.json";

    static const Vector3 kOffsets[5] = {
        { -1.0f, 0.f,  0.5f },
        {  1.0f, 0.f, -0.5f },
        { -0.5f, 0.f, -1.2f },
        {  0.5f, 0.f,  1.0f },
        { -1.2f, 0.f, -0.8f },
    };

    SwordBurstEffect::SwordBurstEffect(const std::wstring& name) : Super(name),
        m_clash2(L"SwordBurst_2"),
        m_clash3(L"SwordBurst_3"),
        m_clash4(L"SwordBurst_4"),
        m_clash5(L"SwordBurst_5"),
        m_clash6(L"SwordBurst_6")
    {
        // 1번 충돌 (m_particle) — EffectActor 멤버에 할당
        m_particle = ParticleManager::Get().CreateSystem(kSwordJson);
        m_particle->SetTarget(this);
        // 2~6번 충돌 — SwordClashEffect 생성자에서 자동 처리
    }

    void SwordBurstEffect::Initialize()
    {
        Super::Initialize();

        SwordClashEffect* clashes[] = { &m_clash2, &m_clash3, &m_clash4, &m_clash5, &m_clash6 };
        for (auto* c : clashes)
            c->Initialize();

        RepositionChildren();
    }

    void SwordBurstEffect::RepositionChildren()
    {
        auto* tr = GetComponent<TransformComponent>();
        Vector3 myPos = tr ? tr->GetPos() : Vector3(0.f, 0.f, 0.f);

        SwordClashEffect* clashes[] = { &m_clash2, &m_clash3, &m_clash4, &m_clash5, &m_clash6 };
        for (int i = 0; i < 5; ++i)
        {
            auto* childTr = clashes[i]->GetComponent<TransformComponent>();
            if (childTr) childTr->SetPos(myPos + kOffsets[i]);
        }
    }
}
