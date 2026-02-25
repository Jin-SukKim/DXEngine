# 배치 렌더링: 2-Pass Compute 파이프라인

> GPU가 "몇 개 그릴지" 계산하고 "인덱스를 한 줄로 합쳐" 단일 Draw Call 실행

---

## 핵심 내용

- **왜 2-Pass가 필요한가**: 각 이미터의 alive 파티클 수는 **GPU만 알고 있음** (CPU readback 없음!)
- **Pass 1 (BatchRenderArgsCS)**: 배치별 총 인스턴스 수 집계 + Indirect Draw Args 기록
- **Pass 2 (BuildAliveIndicesCS)**: 이미터별 alive index를 배치용 **평탄 배열**로 병렬 복사

## WHY

- CPU는 alive 수를 모르므로 **GPU가 스스로 Draw Args를 결정**해야 함
- 서로 다른 이미터의 인덱스가 **분산**되어 있으므로 **하나의 연속 배열**로 합쳐야 배치 가능
- **DrawIndexedInstancedIndirect**: GPU가 기록한 Args로 CPU 개입 없이 렌더링

---

## 시각화 레이아웃 — 2-Pass 흐름도

```
[Pass 1: BatchRenderArgsCS]  ─── "몇 개 그릴지 계산"
┌──────────────────────────────────────────────────────────┐
│ BatchGroup A (이미터 1,3,5):                             │
│   이미터1: alive 100개 │ offset: 0                       │
│   이미터3: alive 50개  │ offset: 100                     │
│   이미터5: alive 200개 │ offset: 150                     │
│   ─────────────────────────                              │
│   총 인스턴스: 350개                                     │
│                                                          │
│   → Indirect Draw Args: {indexCount, 350, ...}           │
└──────────────────────────────────────────────────────────┘
                            ▼
[Pass 2: BuildAliveIndicesCS]  ─── "인덱스를 한 줄로 합치기"
┌──────────────────────────────────────────────────────────┐
│ 이미터별 alive index (분산)    배치용 평탄 배열 (연속)   │
│                                                          │
│ 이미터1: [p5, p12, p8, ...]  ┐                          │
│ 이미터3: [p22, p31, ...]     ├──▶ [p5,p12,p8,...,p22,   │
│ 이미터5: [p40, p7, p55, ...] ┘     p31,...,p40,p7,p55]  │
│                                                          │
│ 각 스레드가 병렬로 자기 이미터의 인덱스를 복사           │
└──────────────────────────────────────────────────────────┘
                            ▼
[DrawIndexedInstancedIndirect]
GPU가 결정한 350개 인스턴스를 단일 Draw Call로 렌더링
```

- **3단계 세로 흐름**: Pass 1 → Pass 2 → Draw (각각 둥근 사각형)
- Pass 1: 이미터별 alive 수 + offset 계산 과정을 **표 형태**로 표시
- Pass 2: 좌측에 분산된 배열 3개 → 우측에 합쳐진 연속 배열 (화살표 합류)
- 최하단 Draw Call 블록에 **"단일 Draw Call"** 강조

---

## 시각화 레이아웃 2 — 렌더링 순서와 이유

```
1. Mesh 배치 ─── Opaque, 깊이 버퍼 확보, 정렬 불필요
     ▼
2. FullRes Billboard ─── 근거리/주요 이펙트, 선명도 중요
     ▼
3. LowRes Billboard ─── 원거리/보조, 저해상도 → Upsampling 합성
                         Overdraw 비용 절감
```

- **세로 순서도**: 3단계 블록 (Mesh=회색, FullRes=파랑, LowRes=초록)
- 각 단계 옆에 **이유** 라벨 (왜 이 순서인지)
- Mesh에서 "깊이 버퍼 확보" 화살표가 Billboard 단계로 연결 (Soft Particles 참조 암시)
