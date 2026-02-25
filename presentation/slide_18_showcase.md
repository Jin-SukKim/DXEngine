# 이펙트 쇼케이스 & 전체 파이프라인

> 14종 이펙트 — GPU 파티클 시스템의 표현력과 성능 실증

---

## 이펙트 리스트

| 이펙트 | 특징 | 주요 기술 |
|--------|------|-----------|
| **Firework** | SubEmitter 연쇄 (UpFirework → Burst) | SubEmitter, Burst |
| **Rain** | 3이미터 구성 (Drop + Mist + Splash) | 다중 BlendMode, Soft Particles |
| **CrystalShatter** | Mesh 파편 + 먼지 + 플래시 | 3D Mesh Particle |
| **GalaxySwirl** | Core + Star 궤도 운동 | Curl Noise, Orbit |
| **PortalGateway** | Ring + Spark + Energy 다층 구조 | 4이미터, Additive 레이어링 |
| **Explosion** | SubEmitter 동시 발생 (4이미터) | Burst, Mesh Debris |
| **SwordClash** | Circle + Flash + Spark + Ember | Burst, Opaque Mesh |
| **Magic** | Charge + RuneCircle + Release + Ice | 5이미터, Mesh + Billboard 혼합 |
| **FireFly** | Noise 기반 유기적 움직임 | Curl Noise |
| **Fire** | 10,000+ 대량 파티클 (Fire + Smoke + Ember) | LOD, Off-Screen, AlphaBlend |
| **SparkBurst** | 10,000+ 대량 스트리밍 | Velocity Stretch, 대량 Additive |
| **Fog** | 부드러운 볼류메트릭 효과 | Soft Particles, AlphaBlend |
| **ArcaneCircle** | 마법진 패턴 | TextureSpawnBake |
| **BoxMesh** | 5,120 메시 파티클 | 3D Mesh, 대량 인스턴싱 |

---

## 시각화 레이아웃 1 — 이펙트 그리드 (스크린샷)

```
┌──────────┬──────────┬──────────┬──────────┐
│ Firework │  Rain    │ Crystal  │ Galaxy   │
│ SubEmit  │ 3이미터  │ Mesh파편 │ Noise    │
├──────────┼──────────┼──────────┼──────────┤
│ Portal   │Explosion │ Sword    │  Magic   │
│ Ring+Spark│SubEmit  │ Burst    │ Mesh+파티│
├──────────┼──────────┼──────────┼──────────┤
│ FireFly  │  Fire    │  Fog     │SparkBurst│
│ Noise    │ 10K+파티 │SoftPart  │ 대량스트 │
└──────────┴──────────┴──────────┴──────────┘
```

- **4×3 그리드**: 각 칸에 이펙트 스크린샷 + 이름 + 핵심 태그 1줄
- 각 이펙트 아래에 사용된 **핵심 기술 태그** (작은 뱃지 형태)
- 스크린샷 배경은 어두운 톤으로 통일하여 파티클 강조

---

## 시각화 레이아웃 2 — 전체 파이프라인 최종 요약

```
[CPU]                              [GPU]
┌─────────────┐                   ┌──────────────────────────────┐
│ LOD 계산    │                   │                              │
│ Frustum Cull│──GPU Upload──▶   │  Spawn CS                    │
│ BatchGroup  │                   │    ▼                         │
└─────────────┘                   │  Simulate CS + Compacting    │
                                  │    ▼                         │
                                  │  BatchRenderArgs CS          │
                                  │    ▼                         │
                                  │  BuildAliveIndices CS        │
                                  │    ▼                         │
                                  │  LDS Bitonic Sort            │
                                  │  (GPU-Driven Dispatch)       │
                                  │    ▼                         │
                                  │  Render (Indirect Draw)      │
                                  │  Mesh → FullRes → LowRes    │
                                  └──────────────────────────────┘
```

- CPU 영역(좌측, 작은 박스) → GPU 영역(우측, 큰 박스) 단방향 화살표
- GPU 내부 파이프라인을 **세로 흐름**으로 축약 표시
- **"CPU→GPU 단방향, readback 없음"** 최종 강조 메시지

---

## 프로젝트 정리

- **8주간** 기초 GPU 파티클부터 GPU-Driven Sort Dispatch까지 점진적 확장
- **핵심 원칙**: CPU 개입 최소화, GPU 자율 처리, 배치 효율 극대화
- **결과**: 수만 개 파티클을 실시간으로 시뮬레이션 + 정렬 + 렌더링하는 완전한 GPU 파티클 파이프라인
