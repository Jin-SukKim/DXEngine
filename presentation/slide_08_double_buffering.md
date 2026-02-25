# Double Buffering & StructuredBuffer

> Ping-Pong 패턴으로 시뮬레이션/렌더링 분리 + StructuredBuffer로 대규모 데이터 전달

---

## 핵심 내용

- **Ping-Pong Alive Indices**: Frame N에서 읽는 버퍼와 쓰는 버퍼를 분리
- **시뮬레이션/렌더링 독립**: 시뮬레이션이 새 Alive를 기록하는 동안 렌더링은 이전 프레임 데이터 사용
- **ConstantBuffer → StructuredBuffer 전환**: 64KB 제한 해제, 수십 개 이미터 데이터 전달 가능

## WHY

- **레이스 컨디션 방지**: 시뮬레이션 중 렌더링이 같은 인덱스를 읽으면 데이터 불일치 발생
- **Ping-Pong 패턴**: 매 프레임 Read/Write 버퍼를 교환하여 안전하게 분리
- **StructuredBuffer**: Constant Buffer의 **64KB 제한** 없이 대규모 이미터 데이터 전달

---

## 시각화 레이아웃 1 — Ping-Pong 다이어그램

```
[Frame N]                    [Frame N+1]
┌──────────┐  ┌──────────┐   ┌──────────┐  ┌──────────┐
│Buffer A  │  │Buffer B  │   │Buffer A  │  │Buffer B  │
│(Read)    │  │(Write)   │   │(Write)   │  │(Read)    │
│시뮬레이션│  │새 Alive  │   │새 Alive  │  │시뮬레이션│
│입력      │  │기록      │   │기록      │  │입력      │
└────┬─────┘  └────┬─────┘   └────┬─────┘  └────┬─────┘
     │렌더링에     │다음           │             │렌더링에
     │사용         │프레임 입력    │             │사용
     ▼             ▼              ▼             ▼
◄────── SWAP ──────►        ◄────── SWAP ──────►
```

- Frame N과 Frame N+1을 **좌우 배치**
- Buffer A/B를 **색상 교차** (A=파랑, B=초록 → 다음 프레임에서 역전)
- SWAP 화살표를 **굵은 양방향 화살표**로 표시
- Read 버퍼에 "렌더링 사용" 라벨, Write 버퍼에 "새 데이터 기록" 라벨

## 시각화 레이아웃 2 — CB → SB 전환

```
[ConstantBuffer → StructuredBuffer 전환]

ConstantBuffer                  StructuredBuffer
┌─────────────────┐            ┌─────────────────────┐
│ 64KB 제한       │    ──▶    │ 크기 제한 없음       │
│ 이미터 수 제한  │            │ 수십 개 이미터 지원  │
│ 매 프레임 전체  │            │ 필요한 데이터만      │
│ 업로드          │            │ 인덱싱으로 접근      │
└─────────────────┘            └─────────────────────┘
```

- 좌측 CB: **빨간 테두리** (제한 강조)
- 우측 SB: **초록 테두리** (장점 강조)
- 중앙에 **화살표 + "전환"** 라벨
