# Effect Texture Reference

각 이펙트별 사용 텍스처 목록, 스프라이트시트 정보, 신규 제작 필요 여부를 정리한 문서.

---

## 범례

- ✅ **기존 텍스처** — 프로젝트에 이미 존재
- 🆕 **신규 제작 필요** — 새로 생성/구매 필요
- ⭕ **solidCircle 전용** — 텍스처 없이 프로시저럴 원형 렌더링

---

## 카테고리 1: Realistic

---

### R1 — Campfire
**System**: `Realistic/Campfire/System_Campfire.json`
**설정**: Looping=true, Duration=10.0, PreWarmTime=2.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Campfire_Flame.json` | ⭕ solidCircle 전용 | — | — | centerWhiteIntensity=0.6 |
| `Campfire_Embers.json` | ⭕ solidCircle 전용 | — | — | velocityStretch 사용 |
| `Campfire_Smoke.json` | ✅ `Textures/SmokeSprite.png` | 1024×1024 | PNG | **SpriteSheet 5×5, 25프레임** |
| `Campfire_Flicker.json` | ✅ `Textures/core1.png` | 256×256 | PNG | 빠른 깜박임 |

**SpriteSheet 상세 (Campfire_Smoke)**:
- 파일: `Textures/SmokeSprite.png`
- 타일: 5열 × 5행 = 25프레임
- 재생 방향: 좌→우, 위→아래 (기본)

---

### R2 — WaterFountain
**System**: `Realistic/WaterFountain/System_Fountain.json`
**설정**: Looping=true, Duration=10.0, PreWarmTime=1.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Fountain_Jet.json` | 🆕 `Textures/water_drop.png` | 256×256 | PNG | 원형, 가장자리 soft gradient |
| `Fountain_Mist.json` | ✅ `Textures/SmokeSprite2.png` | 512×512 | PNG | **SpriteSheet 4×4, 16프레임** |
| `Fountain_Splash.json` | 🆕 `Textures/water_splash.png` | 512×512 | PNG | 링 형태, normalBillboard용 |
| `Fountain_Droplets.json` | ⭕ solidCircle 전용 | — | — | 작은 물방울 |

**SpriteSheet 상세 (Fountain_Mist)**:
- 파일: `Textures/SmokeSprite2.png`
- 타일: 4열 × 4행 = 16프레임

**신규 텍스처 가이드**:
- `water_drop.png`: 흰색 원형 물방울, 중앙 밝고 가장자리 투명 (Alpha channel)
- `water_splash.png`: 위에서 본 파문 링, 투명 배경에 흰색 링 (normalBillboard 지면 정렬용)

---

### R3 — DustStorm
**System**: `Realistic/DustStorm/System_DustStorm.json`
**설정**: Looping=true, Duration=10.0, PreWarmTime=2.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `DustStorm_Body.json` | ✅ `Textures/SmokeSprite2.png` | 512×512 | PNG | SpriteSheet 4×4, overdrawControl |
| `DustStorm_Particles.json` | ⭕ solidCircle 전용 | — | — | 작은 먼지 입자 |
| `DustStorm_Ground.json` | 🆕 `Textures/dust.png` | 512×512 | PNG | grain 텍스처, normalBillboard용 |

**신규 텍스처 가이드**:
- `dust.png`: 모래/먼지 grain 패턴, 불규칙한 형태 여러 개, Alpha 포함

---

## 카테고리 2: Spectacular

---

### S1 — GalaxySwirl
**System**: `Spectacular/GalaxySwirl/System_Galaxy.json`
**설정**: Looping=true, Duration=10.0, PreWarmTime=3.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Galaxy_Core.json` | ⭕ solidCircle 전용 | — | — | centerWhiteIntensity=0.9 |
| `Galaxy_Stars.json` | ✅ `Textures/star.jpg` | 256×256 | JPG | Orbit 모듈, spawnInnerRatio=0.3 |
| `Galaxy_Nebula.json` | ✅ `Textures/core2.png` | 512×512 | PNG | 대형 파티클, AlphaBlend |
| `Galaxy_Stream.json` | ✅ `Textures/core1.png` | 256×256 | PNG | Vortex 모듈 |

---

### S2 — ArcaneCircle
**System**: `Spectacular/ArcaneCircle/System_ArcaneCircle.json`
**설정**: Looping=true, Duration=10.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Arcane_OuterRing.json` | ✅ `Textures/runes.png` | 512×512 | PNG | Orbit rate=2.0 |
| `Arcane_InnerRing.json` | ✅ `Textures/PurpleSpell.png` | 512×512 | PNG | Orbit rate=-3.5 (역방향), alphaClipThreshold=0.3 |
| `Arcane_Sparks.json` | ⭕ solidCircle 전용 | — | — | HollowSphere, velocityStretch=0.8 |
| `Arcane_Glow.json` | ✅ `Textures/core3.png` | 256×256 | PNG | SIN sizeCurve |

---

### S3 — LightningVortex
**System**: `Spectacular/LightningVortex/System_Lightning.json`
**설정**: Looping=true, Duration=10.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Lightning_ChargeOrbs.json` | ⭕ solidCircle 전용 | — | — | Orbit rate=8.0, HollowSphere inner=0.8 |
| `Lightning_Arc.json` | ✅ `Textures/thunder1.jpg` | 256×256 | JPG | Vortex + velocityStretch=2.5 |
| `Lightning_Residue.json` | ✅ `Textures/SmokeSprite2.png` | 512×512 | PNG | SpriteSheet 4×4, SIN sizeCurve |
| `Lightning_Ground.json` | ✅ `Textures/flare0.dds` | 256×256 | DDS | burst, disk expand |

---

## 카테고리 3: Basic

---

### B1 — Flame
**System**: `Basic/Flame/System_Flame.json`
**설정**: Looping=true, Duration=10.0, PreWarmTime=1.0

| 이미터 파일 | 텍스처 | 비고 |
|------------|--------|------|
| `Flame_Core.json` | ⭕ solidCircle 전용 | curlNoise + BEZIER alphaCurve |

---

### B2 — Smoke
**System**: `Basic/Smoke/System_Smoke.json`
**설정**: Looping=true, Duration=10.0, PreWarmTime=2.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Smoke_Main.json` | ✅ `Textures/SmokeSprite.png` | 1024×1024 | PNG | SpriteSheet 5×5, 25프레임, lowResolution |

---

### B3 — SparkShower
**System**: `Basic/SparkShower/System_SparkShower.json`
**설정**: Looping=true, Duration=10.0

| 이미터 파일 | 텍스처 | 비고 |
|------------|--------|------|
| `Spark_Main.json` | ⭕ solidCircle=false | World space, velocityStretch=1.5 |

---

### B4 — Confetti
**System**: `Basic/Confetti/System_Confetti.json`
**설정**: Looping=true, Duration=10.0, **PlayRate=1.5**

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Confetti_Fall.json` | ✅ `Textures/particle.png` | 64×64 | PNG | **blendMode=Modulate**, colorRandomness=1.0 |

---

## 카테고리 4: Unreal Quality

---

### U1 — NuclearExplosion
**System**: `UnrealQuality/NuclearExplosion/System_NuclearExplosion.json`
**설정**: Looping=false, Duration=8.0

**SubEmitter 체인**:
```
Nuke_InitialFlash (OnStart 기준점)
  ├── OnStart  → Nuke_Shockwave
  ├── OnStart  → Nuke_Debris
  ├── OnStart  → Nuke_Embers
  └── OnDurationEnd → Nuke_Fireball
                         └── OnDurationEnd → Nuke_Column
```

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Nuke_InitialFlash.json` | ⭕ solidCircle 전용 | — | — | burst=15, BEZIER sizeCurve |
| `Nuke_Shockwave.json` | ✅ `Textures/flare0.dds` | 256×256 | DDS | velocityStretch=3.0, 방사형 |
| `Nuke_Fireball.json` | ✅ `Textures/Explosion02HD/pngegg.png` | 2048×2048 | PNG | **SpriteSheet 5×4, 20프레임** |
| `Nuke_Column.json` | ✅ `Textures/ExplosionCloud/Explosion01-nofire_5x5.tga` | 2048×2048 | TGA | **SpriteSheet 5×5, 25프레임**, overdrawControl |
| `Nuke_Debris.json` | ✅ `Materials/debrie.json` | — | Material | **MeshRender Opaque** (defaultMesh=0) |
| `Nuke_Embers.json` | ⭕ solidCircle 전용 | — | — | velocityStretch=1.2 |

**SpriteSheet 상세**:
- `Nuke_Fireball`: 5열 × 4행 = 20프레임 (Explosion02HD/pngegg.png)
- `Nuke_Column`: 5열 × 5행 = 25프레임 (ExplosionCloud/Explosion01-nofire_5x5.tga)

---

### U2 — SkillImpact
**System**: `UnrealQuality/SkillImpact/System_SkillImpact.json`
**설정**: Looping=false, Duration=3.0

**SubEmitter 체인**:
```
Impact_Flash (OnStart 기준점)
  ├── OnStart    → Impact_Shockring
  ├── OnStart    → Impact_Sparks
  └── OnComplete → Impact_AfterGlow
```

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Impact_Flash.json` | ⭕ solidCircle 전용 | — | — | burst=5, BEZIER sizeCurve(0→2→0) |
| `Impact_Shockring.json` | ✅ `Textures/flare0.dds` | 256×256 | DDS | velocityStretch=2.0 |
| `Impact_Sparks.json` | ⭕ solidCircle 전용 | — | — | HollowSphere inner=0, velocityStretch=1.8 |
| `Impact_AfterGlow.json` | ✅ `Textures/core4.png` | 256×256 | PNG | **SIN sizeCurve (감쇠진동)** |

---

### U3 — MagmaEruption
**System**: `UnrealQuality/MagmaEruption/System_MagmaEruption.json`
**설정**: Looping=false, Duration=6.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Magma_GroundCrack.json` | ✅ `Textures/flame.png` | 512×512 | PNG | burst+spawn, curlNoise |
| `Magma_Eruption.json` | ✅ `Textures/flame.png` | 512×512 | PNG | burst=50, 상향 폭발 |
| `Magma_Bombs.json` | ⭕ MeshRender (defaultMesh=1) | — | — | **MeshRender AlphaBlend** (구형 메시) |
| `Magma_Steam.json` | ✅ `Textures/SmokeSprite2.png` | 512×512 | PNG | SpriteSheet 4×4, lowResolution |

---

## 신규 제작 필요 텍스처 목록

| 파일 경로 | 용도 | 권장 해상도 | 제작 가이드 |
|----------|------|------------|------------|
| `Textures/water_drop.png` | 물방울 (Fountain_Jet) | 256×256 | 흰색 원형, 중앙 밝고 가장자리 부드럽게 투명. Alpha 필수 |
| `Textures/water_splash.png` | 지면 파문 링 (Fountain_Splash, normalBillboard) | 512×512 | 위에서 본 링 모양. 투명 배경. 흰색 가장자리만 있는 원형 링 |
| `Textures/dust.png` | 먼지/모래 grain (DustStorm_Ground) | 512×512 | 불규칙한 작은 점들 여러 개. Alpha 포함. 부드러운 가장자리 |

---

## 기존 텍스처 사용 현황 요약

| 텍스처 파일 | 사용 이펙트 수 | 사용 이미터 |
|------------|--------------|------------|
| `Textures/SmokeSprite.png` | 2 | Campfire_Smoke, Smoke_Main |
| `Textures/SmokeSprite2.png` | 5 | Fountain_Mist, DustStorm_Body, Lightning_Residue, Magma_Steam + (SpriteSheet 4×4) |
| `Textures/core1.png` | 2 | Campfire_Flicker, Galaxy_Stream |
| `Textures/core2.png` | 1 | Galaxy_Nebula |
| `Textures/core3.png` | 1 | Arcane_Glow |
| `Textures/core4.png` | 1 | Impact_AfterGlow |
| `Textures/flare0.dds` | 3 | Nuke_Shockwave, Impact_Shockring, Lightning_Ground |
| `Textures/star.jpg` | 1 | Galaxy_Stars |
| `Textures/thunder1.jpg` | 1 | Lightning_Arc |
| `Textures/runes.png` | 1 | Arcane_OuterRing |
| `Textures/PurpleSpell.png` | 1 | Arcane_InnerRing |
| `Textures/flame.png` | 2 | Magma_GroundCrack, Magma_Eruption |
| `Textures/particle.png` | 1 | Confetti_Fall |
| `Textures/Explosion02HD/pngegg.png` | 1 | Nuke_Fireball (SpriteSheet 5×4) |
| `Textures/ExplosionCloud/Explosion01-nofire_5x5.tga` | 1 | Nuke_Column (SpriteSheet 5×5) |
| `Materials/debrie.json` | 1 | Nuke_Debris (MeshRender) |

---

## 기능 커버리지 검증표

| 기능 | 커버 이펙트 |
|------|-----------|
| Additive blend | 대부분 이펙트 |
| AlphaBlend + 깊이정렬 | Campfire_Smoke, Fountain_Mist, DustStorm_Body, Smoke_Main |
| Modulate blend | Confetti_Fall |
| lowResolution | DustStorm_Body, Fountain_Mist, Smoke_Main, Magma_Steam |
| solidCircle | Campfire_Flame, Galaxy_Core, Arcane_Sparks, Spark_Main, Flame_Core, Impact_Flash 외 다수 |
| centerWhiteIntensity | Campfire_Flame(0.6), Galaxy_Core(0.9), Arcane_Sparks, Lightning_ChargeOrbs(0.8) 외 |
| velocityStretchFactor | Campfire_Embers(1.2), Fountain_Jet(0.6), Lightning_Arc(2.5), Impact_Shockring(2.0) 외 |
| noiseUVDistort | Campfire_Smoke, Fountain_Mist |
| softDistance/softMaxDist | Fountain_Jet(0.5/2.0), Fountain_Mist(0.8), DustStorm_Body(1.0), Smoke_Main(0.5) |
| normalBillboard | Fountain_Splash, DustStorm_Ground |
| alphaClipThreshold | Arcane_InnerRing(0.3) |
| Vortex module | Galaxy_Stream, Lightning_Arc |
| Orbit module | Galaxy_Stars, Arcane_OuterRing, Arcane_InnerRing, Lightning_ChargeOrbs |
| curlNoise | Campfire_Flame, Campfire_Smoke, DustStorm_Body, Fountain_Mist, Flame_Core, Magma_GroundCrack, Magma_Steam |
| BEZIER curve | Campfire_Flame(alpha), Nuke_InitialFlash(size), Impact_Flash(size) 외 |
| SIN curve | Campfire_Flicker(size), Arcane_Glow(size), Lightning_Residue(size), Impact_AfterGlow(size) |
| LINEAR curve | DustStorm_Body(size), Campfire_Smoke(size), Smoke_Main(size), Fountain_Jet(velocity), Galaxy_Stream(velocity) |
| MeshRender + Opaque | Nuke_Debris |
| MeshRender + AlphaBlend | Magma_Bombs |
| Sprite sheet animation | Campfire_Smoke, Fountain_Mist, DustStorm_Body, Nuke_Fireball, Nuke_Column, Lightning_Residue |
| overdrawControl | DustStorm_Body, Nuke_Column |
| SubEmitters (OnStart) | Nuke_InitialFlash→Shockwave/Debris/Embers, Impact_Flash→Shockring/Sparks |
| SubEmitters (OnDurationEnd) | Nuke_InitialFlash→Fireball, Nuke_Fireball→Column |
| SubEmitters (OnComplete) | Impact_Flash→AfterGlow |
| World space spawn | Campfire_Embers, Fountain_Droplets, DustStorm_Body, DustStorm_Ground, Nuke_Shockwave 외 |
| HollowSphere (spawnInnerRatio) | Galaxy_Stars(0.3), Lightning_ChargeOrbs(0.8), Arcane_Sparks(0.0), Impact_Sparks(0.0) |
| Box shape | DustStorm_Body, Confetti_Fall, Arcane_OuterRing, Fountain_Splash 외 |
| burst | Nuke_InitialFlash(15), Impact_Flash(5), MagmaEruption_Eruption(50) 외 다수 |
| PreWarmTime | Campfire(2.0), GalaxySwirl(3.0), DustStorm(2.0), Flame(1.0), Smoke(2.0) |
| PlayRate | Confetti(1.5) |
| colorRandomness | Campfire_Embers(0.3), DustStorm_Body(0.2), Spark_Main(0.3), Nuke_Embers(0.3) |
| velocityCurve | Fountain_Jet(LINEAR 1→0), Galaxy_Stream(LINEAR 가속) |

---

# Phase 2 추가 이펙트

---

## 카테고리 1: Realistic (확장)

---

### R4 — Rain
**System**: `Realistic/Rain/System_Rain.json`
**설정**: Looping=true, Duration=10.0, PreWarmTime=3.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Rain_Drops.json` | ⭕ solidCircle 전용 | — | — | **dragCurve** LINEAR, **gravityCurve** LINEAR, **softNearDist=0.3**, velocityStretch=2.0 |
| `Rain_Splash.json` | 🆕 `Textures/water_splash.png` | 512×512 | PNG | normalBillboard, BEZIER sizeCurve (급팽창) |
| `Rain_Mist.json` | ✅ `Textures/SmokeSprite2.png` | 512×512 | PNG | SpriteSheet 4×4, curlNoise, lowResolution |

**Phase 2 기능 커버**: `dragCurve`, `gravityCurve`, `softNearDist`

---

### R5 — Snow
**System**: `Realistic/Snow/System_Snow.json`
**설정**: Looping=true, Duration=10.0, PreWarmTime=4.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Snow_Flakes.json` | ✅ `Textures/particle.png` | 64×64 | PNG | **COS sizeCurve** (흔들림), **dragCurve** BEZIER, **softNearDist=0.5** |
| `Snow_Ground.json` | ✅ `Textures/particle.png` | 64×64 | PNG | normalBillboard, **STEP alphaCurve** (즉시 나타남/소멸) |

**Phase 2 기능 커버**: `COS` curve type, `STEP` curve type, `dragCurve` BEZIER, `softNearDist`

---

### R6 — Tornado
**System**: `Realistic/Tornado/System_Tornado.json`
**설정**: Looping=true, Duration=10.0, PreWarmTime=2.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Tornado_Funnel.json` | ✅ `Textures/SmokeSprite2.png` | 512×512 | PNG | Vortex **strengthCurve** BEZIER, **noiseStrengthCurve** LINEAR, curlNoise |
| `Tornado_Debris.json` | ✅ `Textures/particle.png` | 64×64 | PNG | **Custom spawn** (12점 원형), **dragCurve** LINEAR, 3D rotation |
| `Tornado_DustBase.json` | ✅ `Textures/SmokeSprite2.png` | 512×512 | PNG | normalBillboard, overdrawControl, curlNoise |
| `Tornado_Lightning.json` | ✅ `Textures/thunder1.jpg` | 256×256 | JPG | periodic burst, velocityStretch=3.0, Additive |

**Phase 2 기능 커버**: Vortex `strengthCurve`, `noiseStrengthCurve`, `Custom` spawn, `dragCurve`

---

## 카테고리 2: Spectacular (확장)

---

### S4 — SolarFlare
**System**: `Spectacular/SolarFlare/System_SolarFlare.json`
**설정**: Looping=true, Duration=10.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Solar_Surface.json` | ⭕ solidCircle 전용 | — | — | HollowSphere(inner=0.9), **NOISE sizeCurve** (seed=42), centerWhiteIntensity=0.7 |
| `Solar_Prominence.json` | ✅ `Textures/flame.png` | 512×512 | PNG | Orbit **rateCurve** SIN, **animTime=0.5**, SpriteSheet 4×4, velocityStretch=1.5 |
| `Solar_Eruption.json` | ✅ `Textures/ember.png` | 256×256 | PNG | periodic burst(20 particles/3s), gravity fall |

**Phase 2 기능 커버**: `NOISE` curve type, Orbit `rateCurve`, `animTime`

---

### S5 — CrystalShatter
**System**: `Spectacular/CrystalShatter/System_CrystalShatter.json`
**설정**: Looping=false, Duration=3.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Crystal_Flash.json` | ⭕ solidCircle 전용 | — | — | burst=10, **STEP sizeCurve** (즉시 소멸), centerWhiteIntensity=1.0 |
| `Crystal_Fragments.json` | ✅ `Textures/particle.png` | 64×64 | PNG | **Custom spawn** (8점 사면체), **dragCurve** BEZIER, burst=30, 3D tumble rotation |
| `Crystal_Dust.json` | ✅ `Textures/SmokeSprite.png` | 1024×1024 | PNG | SpriteSheet 5×5, curlNoise, BEZIER alphaCurve, lowResolution |

**Phase 2 기능 커버**: `STEP` curve type, `Custom` spawn, `dragCurve` BEZIER

---

## 카테고리 3: Basic (확장)

---

### B5 — Bubble
**System**: `Basic/Bubble/System_Bubble.json`
**설정**: Looping=true, Duration=10.0, PreWarmTime=2.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Bubble_Rise.json` | ✅ `Textures/particle.png` | 64×64 | PNG | **gravityCurve** LINEAR(부력 감소), **COS sizeCurve** (떨림), colorRandomness=0.2 |

**Phase 2 기능 커버**: `gravityCurve`, `COS` curve type

---

### B6 — Firefly
**System**: `Basic/Firefly/System_Firefly.json`
**설정**: Looping=true, Duration=10.0, PreWarmTime=3.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Firefly_Glow.json` | ⭕ solidCircle 전용 | — | — | World space, **NOISE alphaCurve** (랜덤 깜박임), **COS sizeCurve**, curlNoise, centerWhiteIntensity=0.3 |

**Phase 2 기능 커버**: `NOISE` curve type, `COS` curve type

---

## 카테고리 4: Unreal Quality (확장)

---

### U4 — PortalGateway
**System**: `UnrealQuality/PortalGateway/System_PortalGateway.json`
**설정**: Looping=true, Duration=10.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Portal_Ring.json` | ⭕ solidCircle 전용 | — | — | Orbit **rateCurve** BEZIER(가속→안정), Vortex **strengthCurve** LINEAR, centerWhiteIntensity=0.6 |
| `Portal_Energy.json` | ✅ `Textures/core2.png` | 512×512 | PNG | curlNoise, **noiseStrengthCurve** NOISE(seed=13), BEZIER alphaCurve |
| `Portal_Sparks.json` | ⭕ solidCircle 전용 | — | — | HollowSphere(inner=0.8), velocityStretch=1.5 |
| `Portal_Flash.json` | ⭕ solidCircle 전용 | — | — | periodic burst(1/2.5s), BEZIER sizeCurve(0→3→0), centerWhiteIntensity=1.0 |

**Phase 2 기능 커버**: Orbit `rateCurve` BEZIER, Vortex `strengthCurve`, `noiseStrengthCurve` NOISE

---

## 카테고리 5: GameScenario (신규)

---

### G1 — HealAura
**System**: `GameScenario/HealAura/System_HealAura.json`
**설정**: Looping=true, Duration=10.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Heal_Glow.json` | ✅ `Textures/core3.png` | 256×256 | PNG | **COS sizeCurve** (맥동), solidCircle + centerWhiteIntensity=0.4 |
| `Heal_Particles.json` | ⭕ solidCircle 전용 | — | — | BEZIER alphaCurve(fade in→sustain→fade out), drag=0.5 |
| `Heal_Burst.json` | ✅ `Textures/core1.png` | 256×256 | PNG | periodic burst(8/1.5s), HollowSphere |

---

### G2 — ShieldBarrier
**System**: `GameScenario/ShieldBarrier/System_ShieldBarrier.json`
**설정**: Looping=true, Duration=10.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Shield_Shell.json` | ✅ `Textures/core2.png` | 512×512 | PNG | HollowSphere(inner=0.95), **STEP alphaCurve** (깜박임) |
| `Shield_Orbs.json` | ⭕ solidCircle 전용 | — | — | Orbit **rateCurve** LINEAR(2→4 가속), centerWhiteIntensity=0.6 |
| `Shield_Impact.json` | ⭕ solidCircle 전용 | — | — | burst=15, velocityStretch=1.0 |

**Phase 2 기능 커버**: `STEP` curve type, Orbit `rateCurve`

---

### G3 — LevelUp
**System**: `GameScenario/LevelUp/System_LevelUp.json`
**설정**: Looping=false, Duration=4.0

**SubEmitter 체인**:
```
LevelUp_Base (Duration=0.5)
  ├── OnStart      → LevelUp_Pillar
  ├── OnStart      → LevelUp_Sparks
  └── OnDurationEnd → LevelUp_Flash
```

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `LevelUp_Base.json` | ✅ `Textures/flare0.dds` | 256×256 | DDS | burst=20, BEZIER sizeCurve(expand), SubEmitter chain root |
| `LevelUp_Pillar.json` | ✅ `Textures/core1.png` | 256×256 | PNG | velocity[0,8,0], **STEP sizeCurve** (순간 등장/소멸) |
| `LevelUp_Sparks.json` | ⭕ solidCircle 전용 | — | — | **dragCurve** BEZIER(0.2→0.8→0.3), gravity fall |
| `LevelUp_Flash.json` | ⭕ solidCircle 전용 | — | — | burst=8, BEZIER sizeCurve(0→5→0), centerWhiteIntensity=1.0 |

**Phase 2 기능 커버**: SubEmitter 체인(OnStart + OnDurationEnd), `STEP` curve, `dragCurve` BEZIER

---

### G4 — Teleport
**System**: `GameScenario/Teleport/System_Teleport.json`
**설정**: Looping=false, Duration=3.0

**SubEmitter 체인**:
```
Teleport_Dissolve (Duration=2.0)
  └── OnDurationEnd → Teleport_Flash
```

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Teleport_Dissolve.json` | ✅ `Textures/core2.png` | 512×512 | PNG | Vortex **strengthCurve** BEZIER(0→1→0.5), SubEmitter→Flash |
| `Teleport_Energy.json` | ⭕ solidCircle 전용 | — | — | curlNoise(3.0), **NOISE alphaCurve** (seed=99) |
| `Teleport_Rings.json` | ✅ `Textures/core1.png` | 256×256 | PNG | Orbit **rateCurve** BEZIER(0→10 급가속), burst=6, velocityStretch=0.8 |
| `Teleport_Flash.json` | ⭕ solidCircle 전용 | — | — | burst=12, BEZIER sizeCurve(0→4→0), centerWhiteIntensity=1.0 |

**Phase 2 기능 커버**: Vortex `strengthCurve` BEZIER, `NOISE` alphaCurve, Orbit `rateCurve` BEZIER, SubEmitter

---

### G5 — DeathDissolve
**System**: `GameScenario/DeathDissolve/System_DeathDissolve.json`
**설정**: Looping=false, Duration=4.0

| 이미터 파일 | 텍스처 | 해상도 권장 | 포맷 | 비고 |
|------------|--------|------------|------|------|
| `Death_Particles.json` | ✅ `Textures/particle.png` | 64×64 | PNG | **gravityCurve** BEZIER(떨어지다가 위로 뜸: 0.5→-0.5→-1.0) |
| `Death_Smoke.json` | ✅ `Textures/SmokeSprite2.png` | 512×512 | PNG | SpriteSheet 4×4, curlNoise, **noiseStrengthCurve** LINEAR(0.5→2.0), lowResolution |
| `Death_Soul.json` | ⭕ solidCircle 전용 | — | — | **COS sizeCurve** (영혼 떨림), LINEAR alphaCurve(소멸), centerWhiteIntensity=0.3 |

**Phase 2 기능 커버**: `gravityCurve` BEZIER, `noiseStrengthCurve`, `COS` curve type

---

## Phase 2 기존 파일 수정 내역

| 파일 | 추가된 기능 | 설명 |
|------|-----------|------|
| `Campfire_Flame.json` | `dragCurve` LINEAR(0→0.3) | 불꽃이 위로 갈수록 감속 |
| `Campfire_Smoke.json` | `gravityCurve` LINEAR(-0.2→-0.5) | 연기 상승 가속 |
| `Fountain_Jet.json` | `softNearDist=0.2` | 카메라 근접 시 자연스러운 페이드 |
| `DustStorm_Body.json` | `noiseStrengthCurve` LINEAR(1→2) | 시간 경과에 따른 난류 강화 |
| `Galaxy_Stream.json` | Vortex `strengthCurve` SIN | 소용돌이 주기적 강도 변화 |
| `Arcane_OuterRing.json` | Orbit `rateCurve` SIN | 궤도 가감속 |
| `Lightning_Arc.json` | `noiseStrengthCurve` NOISE + curlNoise 추가 | 번개 불규칙성 강화 |
| `Nuke_Column.json` | `dragCurve` BEZIER(0→0.5→0.8) | 버섯구름 감속 효과 |

---

## Phase 2 텍스처 사용 현황 (추가분)

| 텍스처 파일 | Phase 2 사용 이미터 |
|------------|-------------------|
| `Textures/SmokeSprite2.png` | Rain_Mist, Tornado_Funnel, Tornado_DustBase, Death_Smoke |
| `Textures/SmokeSprite.png` | Crystal_Dust |
| `Textures/particle.png` | Snow_Flakes, Snow_Ground, Tornado_Debris, Crystal_Fragments, Bubble_Rise, Death_Particles |
| `Textures/core1.png` | Heal_Burst, LevelUp_Pillar, Teleport_Rings |
| `Textures/core2.png` | Shield_Shell, Portal_Energy, Teleport_Dissolve |
| `Textures/core3.png` | Heal_Glow |
| `Textures/thunder1.jpg` | Tornado_Lightning |
| `Textures/flame.png` | Solar_Prominence (SpriteSheet 4×4 + animTime) |
| `Textures/ember.png` | Solar_Eruption |
| `Textures/flare0.dds` | LevelUp_Base |
| `Textures/water_splash.png` | Rain_Splash |

---

## Phase 2 미사용 기능 커버리지 달성표

| 미사용 기능 | 커버 이펙트 | 상태 |
|------------|-----------|------|
| `dragCurve` | Rain_Drops, Snow_Flakes, Tornado_Debris, LevelUp_Sparks, Campfire_Flame*, Nuke_Column* | ✅ 완료 |
| `gravityCurve` | Rain_Drops, Bubble_Rise, Death_Particles, Campfire_Smoke* | ✅ 완료 |
| `noiseStrengthCurve` | Tornado_Funnel, Portal_Energy, Death_Smoke, DustStorm_Body*, Lightning_Arc* | ✅ 완료 |
| Vortex `strengthCurve` | Tornado_Funnel, Portal_Ring, Teleport_Dissolve, Galaxy_Stream* | ✅ 완료 |
| Orbit `rateCurve` | Solar_Prominence, Shield_Orbs, Portal_Ring, Teleport_Rings, Arcane_OuterRing* | ✅ 완료 |
| `COS` curve type | Snow_Flakes, Bubble_Rise, Heal_Glow, Death_Soul, Firefly_Glow | ✅ 완료 |
| `STEP` curve type | Snow_Ground, Shield_Shell, LevelUp_Pillar, Crystal_Flash | ✅ 완료 |
| `NOISE` curve type | Firefly_Glow, Solar_Surface, Portal_Energy, Teleport_Energy, Lightning_Arc* | ✅ 완료 |
| `softNearDist` | Rain_Drops, Snow_Flakes, Fountain_Jet* | ✅ 완료 |
| `animTime` | Solar_Prominence | ✅ 완료 |
| `Custom` spawn | Tornado_Debris, Crystal_Fragments | ✅ 완료 |

*\* = 기존 파일 수정*

---

*Phase 1 생성일: 2026-02-23*
*Phase 2 추가일: 2026-02-23*
