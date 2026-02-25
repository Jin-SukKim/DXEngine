#include "pch.h"
#include "SwordBurstEffect.h"
#include "ParticleManager.h"
#include "TransformComponent.h"

namespace DE {

    static constexpr wchar_t kSwordJson[] =
        L"Particles\\Effects\\Combination\\SwordClash\\System_SwordClash.json";

    static const Vector3 kOffsets[10] = {
        { 0.0f,  0.0f, 0.f },   // 0번 - 원점 (m_clash1)
        { 0.5f,  0.2f, 0.f },   // 1번
        { 1.0f, -0.1f, 0.f },   // 2번
        { 1.5f,  0.05f, 0.f },   // 3번
        { 2.0f, -0.2f, 0.f },   // 4번
        { 2.5f,  0.1f, 0.f },   // 5번
        { 3.0f, -0.2f, 0.f },   // 6번
        { 3.5f,  0.0f, 0.f },   // 7번
        { 4.0f, 0.1f, 0.f },   // 8번
        { 4.5f, -0.05f, 0.f },   // 9번
    };

    SwordBurstEffect::SwordBurstEffect(const std::wstring& name) : Super(name),
        m_clash1(L"SwordBurst_1"),
        m_clash2(L"SwordBurst_2"),
        m_clash3(L"SwordBurst_3"),
        m_clash4(L"SwordBurst_4"),
        m_clash5(L"SwordBurst_5"),
        m_clash6(L"SwordBurst_6"),
        m_clash7(L"SwordBurst_7"),
        m_clash8(L"SwordBurst_8"),
        m_clash9(L"SwordBurst_9"),
        m_clash10(L"SwordBurst_10")
    {
        // 1번 충돌 (m_particle) — EffectActor 멤버에 할당
        m_particle = ParticleManager::Get().CreateSystem(kSwordJson);
        m_particle->SetTarget(this);
        // 2~6번 충돌 — SwordClashEffect 생성자에서 자동 처리
    }

    void SwordBurstEffect::Initialize()
    {
        Super::Initialize();

        SwordClashEffect* clashes[] = { &m_clash1, &m_clash2, &m_clash3, &m_clash4, &m_clash5, &m_clash6, &m_clash7, &m_clash8, &m_clash9, &m_clash10 };
        for (auto* c : clashes)
            c->Initialize();
        //RepositionChildren();

        // 초기에는 전부 비활성화
        for (auto* c : clashes)
            c->Stop();

    }

    void SwordBurstEffect::RepositionChildren()
    {
        auto* tr = GetComponent<TransformComponent>();
        Vector3 myPos = tr ? tr->GetPos() : Vector3(0.f, 0.f, 0.f);

        SwordClashEffect* clashes[] = { &m_clash1, &m_clash2, &m_clash3, &m_clash4, &m_clash5, &m_clash6, &m_clash7, &m_clash8, &m_clash9, &m_clash10 };
        for (int i = 0; i < 10; ++i)
        {
            auto* childTr = clashes[i]->GetComponent<TransformComponent>();
            if (childTr) childTr->SetPos(myPos + kOffsets[i]);
        }
    }

    void SwordBurstEffect::Update(const float& dt)
    {
        Super::Update(dt);

        if (m_spawnIndex >= kTotalClashes)
            return;

        m_timer += dt;
        if (m_timer >= kSpawnInterval)
        {
            m_timer = 0;
            SpawnNext();
        }
    }

    void SwordBurstEffect::SpawnNext()
    {
        if (m_spawnIndex >= kTotalClashes)
            return;

        SwordClashEffect* clashes[] = {
            &m_clash1, &m_clash2, &m_clash3,
            &m_clash4, &m_clash5, &m_clash6, &m_clash7,& m_clash8,& m_clash9,& m_clash10
        };

        SwordClashEffect* target = clashes[m_spawnIndex];

        // 위치 설정 후 활성화
        auto* tr = GetComponent<TransformComponent>();
        Vector3 myPos = tr ? tr->GetPos() : Vector3(0.f, 0.f, 0.f);

        auto* childTr = target->GetComponent<TransformComponent>();
        if (childTr)
            childTr->SetPos(myPos + kOffsets[m_spawnIndex]);

        target->Play();

        ++m_spawnIndex;
    }
}
