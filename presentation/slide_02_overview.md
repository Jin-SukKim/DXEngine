# 프로젝트 개요 & 타임라인

> GPU Compute Shader 기반 파티클 시스템 — 생성부터 렌더링까지 전 과정을 GPU에서 처리

---

## 목표

CPU 개입 없이 **GPU만으로** 파티클의 생성·시뮬레이션·정렬·렌더링을 수행하는 고성능 파티클 엔진 구축

## 핵심 기술 스택

| 기술 | 설명 |
|------|------|
| **Compute Shader** | 파티클 Spawn, Simulate, Sort, Batch Args 전부 GPU 처리 |
| **Indirect Drawing** | GPU가 직접 Draw 인자 결정 → CPU readback 제거 |
| **Memory Pool** | 통합 GPU 버퍼 + 블록 할당으로 메모리 효율화 |
| **Batch Rendering** | Material 기준 그룹핑으로 드로우콜 최소화 |
| **LDS Bitonic Sort** | groupshared memory 활용 3-Phase 최적화 정렬 |

---

## 시각화 레이아웃 — 가로 타임라인 바

```
┌──────────┬──────────┬──────────┬───────────────┬──────────┬──────────┐
│  1~2주   │   3주    │   4주    │    5~6주      │   7주    │   8주    │
│ 기초 GPU │ 3D Mesh  │ SubEmit  │ MemoryPool    │ 렌더링   │ 이펙트   │
│ 파티클   │ Spawn    │ DB       │ Batch         │ 품질     │ Sort     │
│ 모듈     │ Texture  │ StructBuf│ 최적화        │ LDS Sort │ Driven   │
│ JSON     │ Material │          │ GS제거        │ Bloom    │ 쇼케이스 │
└──────────┴──────────┴──────────┴───────────────┴──────────┴──────────┘
 Jan 6-18   Jan 19-25  Jan 26-    Feb 2-13        Feb 14-22  Feb 23-25
                       Feb 1
```

- 각 칸은 색상으로 구분 (기초=파랑, 확장=초록, 최적화=주황, 품질=보라, 마무리=빨강)
- 현재 슬라이드에서 다루는 영역을 하이라이트 표시하여 전체 진행 맥락 전달
