# 배치 렌더링: 개념과 BatchGroup

> 같은 Material 이미터를 하나의 Draw Call로 묶어 드로우콜 대폭 감소

---

## 핵심 내용

- **BatchGroup**: 같은 Material + BlendMode + RenderType을 공유하는 이미터 묶음
- **드로우콜 감소**: 30개 이미터 → 3~5회 Draw Call (기존 30회에서 **~90% 감소**)
- **전제 조건**: Memory Pool 통합 버퍼 — 하나의 SRV로 모든 이미터 파티클 접근 가능

## WHY

- 개별 드로우콜마다 **상태 전환**(셰이더, 텍스처 바인딩)이 발생 → GPU 파이프라인 플러시
- Material이 같은 이미터는 **상태 전환 없이** 연속 렌더링 가능
- Memory Pool의 **통합 버퍼** 덕분에 서로 다른 이미터의 파티클도 같은 SRV로 접근

---

## 시각화 레이아웃 1 — Before vs After (드로우콜 비교)

```
[Before: 개별 드로우콜]
Emitter 1 (Material A) → Draw Call 1 + 상태 전환
Emitter 2 (Material B) → Draw Call 2 + 상태 전환
Emitter 3 (Material A) → Draw Call 3 + 상태 전환
Emitter 4 (Material C) → Draw Call 4 + 상태 전환
Emitter 5 (Material A) → Draw Call 5 + 상태 전환
... (30개 이미터 → 30회 Draw Call + 30회 상태 전환)

[After: 배치 렌더링]
BatchGroup 1 (Material A): Emitter 1,3,5,... → Draw Call 1
BatchGroup 2 (Material B): Emitter 2,...     → Draw Call 2
BatchGroup 3 (Material C): Emitter 4,...     → Draw Call 3
... (30개 이미터 → 3~5회 Draw Call!)
```

- **좌측(Before)**: 이미터별 박스가 각각 Draw Call 박스로 연결 (빨간 배경)
- **우측(After)**: 같은 Material의 이미터들이 하나의 Draw Call 박스로 합쳐짐 (초록 배경)
- Material A/B/C를 **다른 색상**으로 표시하여 그룹핑 시각화
- 하단에 **"30회 → 3~5회"** 대형 숫자 비교

## 시각화 레이아웃 2 — BatchGroup 구성 기준

```
┌─────────────────────────────────────────────┐
│              BatchGroup 분류 기준            │
│                                              │
│  ┌──────────┐  ┌──────────┐  ┌───────────┐ │
│  │Material  │  │BlendMode │  │RenderType │ │
│  │Key       │  │          │  │           │ │
│  │(텍스처+  │  │Additive  │  │Billboard  │ │
│  │ 셰이더)  │  │AlphaBlend│  │Mesh       │ │
│  │          │  │Opaque    │  │           │ │
│  └────┬─────┘  └────┬─────┘  └─────┬─────┘ │
│       └──────────────┼──────────────┘       │
│                      ▼                       │
│           3가지 모두 같아야 같은 배치        │
└─────────────────────────────────────────────┘
```

- **벤다이어그램** 또는 **3개 기둥이 합쳐지는 다이어그램**으로 표현
- 각 기준 블록에 해당하는 옵션들을 나열
- 하단에 "3가지 모두 같아야 같은 배치" **강조 텍스트**

### Memory Pool 연결

```
Memory Pool (Slide 09)
  └── 통합 버퍼 → 하나의 SRV로 모든 이미터 접근
        └── 배치 렌더링 가능!
```

- Slide 09의 Memory Pool에서 이어지는 **연결 화살표** 표시
