# SubEmitter 시스템

> 생명주기 이벤트 기반 연쇄 파티클 — 폭발→연기, 불꽃→잔불

---

## 핵심 내용

- **3가지 이벤트 트리거**: OnStart, OnDurationEnd, OnComplete
- **위치 상속**: 부모 파티클/이미터 위치에서 자식 이미터 발생
- **JSON 선언적 정의**: SubEmitter 경로, 트리거 타입, 상속 옵션을 JSON으로 지정
- **계층적 구조**: 자식 이미터가 다시 SubEmitter를 가질 수 있음

## WHY

- 자연스러운 **연쇄 이펙트** 표현: 폭발 → 연기, 충돌 → 스파크
- **이벤트 기반**이므로 타이밍을 선언적으로 제어 가능
- 하나의 이펙트 시스템에서 **복잡한 다단계 연출** 구현

---

## 시각화 레이아웃 — 이벤트 타임라인 (Firework 예시)

```
[Firework 예시]
시간 ──────────────────────────────────────────────▶

UpFirework    ■■■■■■■■■■■
(OnStart)     ↑시작       ↑Duration 종료
                           │
Burst         ·············■■■■■■■■■■■■■■■■■
(OnDurationEnd)            ↑부모 위치에서 폭발

──────────────────────────────────────────────────
이벤트:  OnStart ──── OnDurationEnd ──── OnComplete
```

- 가로축: **시간 흐름** (좌→우)
- 각 이미터를 **가로 바**로 표시 (UpFirework=파랑, Burst=빨강)
- 이벤트 발생 시점에 **세로 점선** + 이벤트명 라벨
- UpFirework 바 끝(Duration 종료)에서 Burst 바 시작으로 **화살표 연결**
- 하단에 3가지 이벤트(OnStart, OnDurationEnd, OnComplete) **아이콘 범례**

### 추가 예시

```
[Explosion 이펙트]
OnStart    → Explosion(Additive, burst 10)
OnStart    → Smoke(AlphaBlend, burst 10)
OnStart    → Spark(Additive, burst 50)
OnStart    → Debris(Opaque Mesh, burst 15)
```

- 하나의 트리거에서 **다수의 SubEmitter**가 동시 발생하는 패턴
