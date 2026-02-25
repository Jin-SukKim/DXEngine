# LDS Bitonic Sort

> 64-bit 계층적 키 기반 3-Phase LDS Bitonic Sort — 디스패치 횟수 최대 66배 감소

---

## 핵심 내용

- **64-bit Key**: Batch ID(Major) + Depth(Minor) — 같은 Material끼리 연속 + 각 Material 내 back-to-front
- **3-Phase 아키텍처**: LDS 블록 정렬 → Global Merge → LDS Inner Merge
- **groupshared memory(LDS)**: 블록 내부 정렬을 빠른 shared memory에서 수행 → global memory 접근 대폭 감소

## WHY

- **AlphaBlend 파티클**은 뒤에서 앞으로(back-to-front) 정렬 필수
- 단순 depth 정렬 시 **서로 다른 Material이 섞여** 렌더링마다 상태 전환 → Draw Call 폭증
- **Batch ID를 Major Key**로 하면 같은 Material이 연속 → 배치 렌더링 유지
- 일반 Bitonic Sort: 매 (k,j) 단계마다 global memory 접근 → **디스패치 횟수 폭증**
- LDS 활용: 블록 내부는 shared memory에서 완료 → **디스패치 횟수 대폭 감소**

---

## 시각화 레이아웃 1 — 64-bit Key 구조

```
┌─────────────────────────────┬─────────────────────────────┐
│      key.x (32bit)         │      key.y (32bit)         │
│      Batch ID (Major)      │      Depth (Minor)          │
│      같은 Material끼리     │      Material 내에서        │
│      연속 배치             │      back-to-front          │
└─────────────────────────────┴─────────────────────────────┘
정렬 결과: [BatchA-먼것, BatchA-가까운것, BatchB-먼것, BatchB-가까운것]
```

- **가로 바** 2칸: key.x(파랑) + key.y(초록)
- 각 칸 내부에 역할 설명
- 하단에 **정렬 결과 예시** 배열

## 시각화 레이아웃 2 — 3-Phase 아키텍처

```
Phase 1: LDS 블록 정렬          Phase 2: Global Merge
┌──────────┐ ┌──────────┐      ┌─────────────────────────┐
│ 블록 0   │ │ 블록 1   │      │ 블록 간 비교/교환      │
│ LDS에서  │ │ LDS에서  │ ───▶ │ Global Memory 사용     │
│ 완전 정렬│ │ 완전 정렬│      │ (j ≥ BLOCK_SIZE)       │
└──────────┘ └──────────┘      └──────────┬──────────────┘
1회 디스패치로 블록 내부 완료              ▼
                                Phase 3: LDS Inner Merge
                                ┌─────────────────────────┐
                                │ 블록 내부 나머지 정렬   │
                                │ 다시 LDS에서 수행       │
                                │ 1회 디스패치            │
                                └─────────────────────────┘
```

- **3개 Phase 블록**: Phase 1(초록) → Phase 2(주황) → Phase 3(파랑)
- Phase 1: 두 개의 작은 블록이 LDS 안에서 정렬됨 (블록 내부 화살표)
- Phase 2: 블록 간 교환 (Global Memory 화살표, 점선)
- Phase 3: 다시 LDS로 돌아와 내부 정렬 완료
- 각 Phase에 **"LDS"** 또는 **"Global Memory"** 라벨

## 시각화 레이아웃 3 — 디스패치 비교 차트

```
파티클 수     기존      LDS 3-Phase    개선
─────────────────────────────────────────
2,048        66회       1회          66배 ↓
4,096        78회       3회          26배 ↓
8,192        91회       6회          15배 ↓
65,536      136회      21회         6.5배 ↓
```

- **가로 막대 그래프**: 기존(빨강) vs LDS(초록) 막대 비교
- 각 행에 **개선 배수** 볼드 표시
- 막대 길이 차이가 시각적으로 극적인 효과를 주도록 배치
