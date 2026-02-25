# 모듈 시스템 & Data-Driven

> 독립 모듈 + JSON 선언적 조합 — 코드 수정 없이 이펙트 생성/수정

---

## 핵심 내용

- **Priority 기반 모듈 스택**: Spawn(1) → Visual(2) → Force(3) → Material(5) → Render(6) 순서로 실행
- **Factory 패턴**: 모듈 이름으로 동적 생성, 새 모듈 추가 시 기존 코드 수정 불필요
- **JSON 선언적 조합**: 이펙트마다 필요한 모듈만 JSON에서 지정
- **FileWatcher Hot-Reload**: 런타임 중 JSON 수정 → 즉시 반영

## WHY

- 이펙트마다 **다른 모듈 조합**이 필요 (불꽃=Gravity+Noise, 비=Gravity만)
- 코드 수정 없이 **JSON만으로** 새 이펙트 생성/수정 가능
- Hot-Reload로 **이터레이션 속도** 대폭 향상

---

## 시각화 레이아웃 — 모듈 스택 + JSON 연결도

```
[JSON 파일]              [Factory]              [Module Stack]
┌──────────────┐        ┌──────────┐     ┌─────────────────────┐
│ "Spawn":     │───────▶│ Module   │────▶│ Priority 6: Render  │
│   "Box"      │        │ Factory  │     │ Priority 5: Material│
│ "Force":     │───────▶│ Create() │────▶│ Priority 3: Force   │
│   "Gravity"  │        │          │     │ Priority 2: Visual  │
│ "Render":    │───────▶│          │────▶│ Priority 1: Spawn   │
│   "Billboard"│        └──────────┘     └─────────────────────┘
└──────────────┘
         │
  [FileWatcher]─── 파일 변경 감지 ──▶ 런타임 Hot-Reload
```

- 좌측: **JSON 파일** 아이콘 + 키-값 표시 (둥근 사각형, 연한 노란색 배경)
- 중앙: **Factory** 기어 아이콘 (회색 배경)
- 우측: **Module Stack** 세로 블록 (Priority 높을수록 위, 각 모듈 다른 색상)
- 하단: FileWatcher 화살표가 JSON에서 Module Stack으로 연결 (점선 + "Hot-Reload" 라벨)
- 각 JSON 키에서 Factory를 거쳐 해당 모듈로 연결되는 **화살표 3개**
