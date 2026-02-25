# GPU-Driven Sort Dispatch

> CPU readback + per-dispatch CB 오버헤드 완전 제거 — 순수 GPU 정렬 파이프라인

---

## 핵심 내용

- **PrepareSortDispatchCS**: GPU에서 직접 sort 파라미터 집계 + IndirectArgs 기록
- **Pre-allocated Constant Buffers**: 초기화 시 모든 step의 CB를 사전 할당 → 런타임 Map/Unmap **제로**
- **minSortSize 자동 스킵**: DispatchIndirect(0,1,1)은 GPU에서 자동 no-op → 불필요한 단계 자동 건너뜀
- **MAX_SORT_SIZE = 131,072** → 최대 **28 sort steps** 사전 계산

## WHY

- **GPU→CPU readback 스톨**: 기존 방식은 `Download()`로 파이프라인 동기 스톨 발생 (~0.3-0.5ms)
- **Per-dispatch CB 오버헤드**: 매 step마다 Map/Unmap + CSSetConstantBuffers (28 steps × ~50μs)
- GPU 내부에서 모든 것을 처리하면 **CPU-GPU 동기화 지점 제로**

---

## 시각화 레이아웃 — 기존 vs 개선 (좌우 비교)

```
[기존 방식]                          [GPU-Driven 방식]
┌──────────┐                        ┌──────────────────────┐
│ GPU 연산 │                        │ PrepareSortDispatch  │
└────┬─────┘                        │ CS (GPU에서 직접     │
     ▼                              │ params 집계 +        │
┌──────────────┐ ← 스톨!           │ IndirectArgs 기록)   │
│ GPU→CPU      │ (~0.3-0.5ms)      └──────────┬───────────┘
│ Download     │                               ▼
└────┬─────────┘                    ┌──────────────────────┐
     ▼                              │ DispatchIndirect     │
┌──────────────┐                    │ × N steps            │
│ CPU에서      │                    │ (Pre-allocated CB    │
│ sort params  │                    │  런타임 Upload 제로) │
│ 계산         │                    │                      │
└────┬─────────┘                    │ groups=0이면         │
     ▼                              │ 자동 no-op (스킵)    │
┌──────────────┐                    └──────────────────────┘
│ 매 step마다  │
│ CB Upload    │ × 28 steps
│ + Dispatch   │ (~50μs each)
└──────────────┘
```

- **좌측(기존)**: 빨간 배경, CPU-GPU 왕복 화살표 + "스톨!" 경고 라벨
  - GPU → CPU Download (빨간 점선)
  - CPU 계산 (회색 블록)
  - CPU → GPU Upload × 28 (빨간 반복 화살표)
- **우측(개선)**: 초록 배경, GPU 내부에서 완결
  - PrepareSortDispatchCS (파란 블록)
  - DispatchIndirect × N (초록 블록, "Pre-allocated CB" 라벨)
  - "groups=0 → no-op" 라벨 (자동 스킵)
- 중앙에 **큰 화살표** + "스톨 제거 + CB 오버헤드 제거 = 순수 GPU 파이프라인"

---

## 시각화 레이아웃 2 — 전체 GPU-Driven Sort 파이프라인

```
[GPU] PrepareSortDispatchCS (1,1,1)
  → batchSortParams 집계 → sortSize 계산 → IndirectArgs 기록
  ↓ (UAV barrier)
[GPU] DispatchIndirect: GenerateSortKeysCS
  → gpuSortParams에서 파라미터 읽기 → 키 생성
  ↓ (UAV barrier)
[GPU] DispatchIndirect × N: BitonicSort steps
  → Pre-allocated CB + Phase별 셰이더 → groups=0이면 자동 스킵
  ↓ (UAV barrier)
[GPU] DispatchIndirect: CopySortedIndicesCS
  → 정렬된 인덱스를 batchAliveIndices에 복사
```

- **세로 파이프라인**: 4단계 블록, 각 단계 사이에 "UAV barrier" 라벨
- 모든 블록에 **[GPU]** 태그로 CPU 개입 없음을 강조
- 전체 파이프라인이 **GPU 내부에서 완결**됨을 배경색으로 표현
