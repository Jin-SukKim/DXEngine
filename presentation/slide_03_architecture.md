# 전체 아키텍처

> CPU→GPU 단방향 파이프라인 — GPU readback 없는 순수 GPU 파티클 시스템

---

## 핵심 내용

- CPU는 **LOD 계산, Frustum Culling, BatchGroup 구성**만 수행
- GPU가 **Spawn → Simulate → Batch Args → Sort → Render** 전 과정 처리
- CPU→GPU 단방향 데이터 흐름, **GPU→CPU readback 완전 제거**

## WHY

- CPU-GPU 왕복은 파이프라인 동기 스톨 유발 (~0.3-0.5ms)
- GPU 내부에서 모든 데이터를 처리하면 레이턴시 최소화
- Indirect Drawing/Dispatch로 GPU가 자체적으로 작업량 결정

---

## 시각화 레이아웃 — 세로 파이프라인 흐름도 (슬라이드 전체 사용)

```
[CPU 영역] ─────────────────────────────────────────────────────────────
┌─────────────────┐   ┌──────────────────┐   ┌─────────────────────┐
│ LOD SpawnRate   │──▶│ Frustum Culling  │──▶│ BatchGroup 구성     │
│ 계산            │   │ (BoundingSphere) │   │ (Material 기준)     │
└─────────────────┘   └──────────────────┘   └─────────┬───────────┘
════════════════════════════ GPU Upload ════════════════╪════════════
[GPU 영역]                                              ▼
┌──────────┐   ┌───────────────────┐   ┌────────────────────────┐
│ Spawn CS │──▶│ Simulate CS       │──▶│ Batch Args CS          │
│(Consume/ │   │(물리+Compacting   │   │(Indirect Draw Args     │
│ Append)  │   │ 단일 패스)        │   │ + Write Offset 계산)   │
└──────────┘   └───────────────────┘   └──────────┬─────────────┘
                                                    ▼
┌──────────────────┐   ┌───────────────┐   ┌─────────────────────┐
│ Build Alive      │──▶│ AlphaBlend    │──▶│ Render              │
│ Indices CS       │   │ Sort (LDS     │   │ Mesh→FullRes→LowRes │
│(Index 평탄화)    │   │ Bitonic)      │   │ DrawIndirect        │
└──────────────────┘   └───────────────┘   └─────────────────────┘
```

- CPU 영역은 **회색 배경**, GPU 영역은 **파란 배경**으로 구분
- GPU Upload 경계선을 **굵은 점선**으로 표시
- 각 블록은 **둥근 사각형**, 화살표로 데이터 흐름 표현
- 하단에 "**CPU→GPU 단방향, GPU readback 없음**" 강조 텍스트
