# DXEngine 파티클 시스템 가이드

## 목차

1. [아키텍처 개요](#1-아키텍처-개요)
2. [파일 구조](#2-파일-구조)
3. [System JSON 스키마](#3-system-json-스키마)
4. [Emitter JSON 스키마](#4-emitter-json-스키마)
5. [모듈 레퍼런스](#5-모듈-레퍼런스)
   - [5.1 Spawn](#51-spawn-모듈)
   - [5.2 Visual](#52-visual-모듈)
   - [5.3 Force](#53-force-모듈)
   - [5.4 Vortex](#54-vortex-모듈)
   - [5.5 Orbit](#55-orbit-모듈)
   - [5.6 Material](#56-material-모듈)
   - [5.7 BillboardRender](#57-billboardrender-모듈)
   - [5.8 MeshRender](#58-meshrender-모듈)
6. [커브 시스템](#6-커브-시스템)
7. [서브 이미터](#7-서브-이미터)
8. [오버드로우 제어](#8-오버드로우-제어)
9. [Material JSON 스키마](#9-material-json-스키마)
10. [이펙트 예제](#10-이펙트-예제)
11. [팁과 모범 사례](#11-팁과-모범-사례)

---

## 1. 아키텍처 개요

### 3계층 구조

```
ParticleSystem          ← 최상위 컨테이너 (Duration, Looping, PlayRate)
 └─ ParticleEmitter[]   ← 독립 이미터 (각각 고유 모듈 세트)
     └─ ParticleModule[] ← 기능 모듈 (Spawn, Visual, Force, Render 등)
```

- **ParticleSystem**: 하나 이상의 Emitter를 관리하는 최상위 단위. 재생 상태, 루핑, 속도 배율을 제어한다.
- **ParticleEmitter**: 독립적인 파티클 방출기. 각 이미터는 자체 모듈 조합을 가진다.
- **ParticleModule**: 동작을 정의하는 플러그인 단위. JSON 키 이름이 곧 모듈 타입이다.

### 모듈 팩토리 패턴

Emitter JSON의 각 최상위 키가 `ParticleModuleFactory::Create(key)`를 통해 해당 모듈 인스턴스로 변환된다. 예약된 키(`Name`, `Duration`, `CompletionDelay`, `SubEmitters`, `overdrawControl`)는 이미터 수준에서 별도 처리된다.

### GPU 파이프라인

```
SpawnCS → ParticleCS → BitonicSort → Render
  (생성)    (시뮬레이션)   (깊이 정렬)   (드로우)
```

- **SpawnCS**: 새 파티클 생성 (위치, 수명, 초기 속성)
- **ParticleCS**: 물리 시뮬레이션 (Force, Vortex, Orbit 등)
- **BitonicSort**: 카메라 깊이 기준 back-to-front 정렬 (AlphaBlend 시 필요)
- **Render**: Billboard 또는 Mesh로 최종 렌더링

---

## 2. 파일 구조

### 디렉토리 레이아웃

```
Assets/
├── Particles/
│   ├── Systems/         ← System JSON (*.json)
│   └── Emitters/        ← Emitter JSON (*.json)
├── Materials/           ← PBR Material JSON (*.json)
└── Textures/            ← 텍스처 파일 (*.png, *.dds 등)
```

### JSON 파일 관계

```
System JSON
 └─ "Emitters": ["Particles/Emitters/fire.json", ...]
                       │
                       ▼
                 Emitter JSON
                  └─ "Material": { "materials": ["Materials/fire_mat.json"] }
                                        │
                                        ▼
                                  Material JSON
                                   └─ "Textures": { "albedo": "Textures/fire.png" }
```

모든 경로는 `Assets/` 디렉토리를 기준으로 한 상대 경로이다.

---

## 3. System JSON 스키마

> 소스: `ParticleSystem.cpp` — `ParticleSystem::LoadFromJson`

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `Name` | string | — | 시스템 이름 |
| `Looping` | bool | true | 루핑 여부 |
| `Duration` | float | — | 재생 시간 (초) |
| `PlayRate` | float | 1.0 | 재생 속도 배율 |
| `PreWarmTime` | float | 0.0 | 사전 시뮬레이션 시간 (초) |
| `State` | string | "Play" | 초기 상태: `"Play"` / `"Pause"` / `"Stop"` |
| `Emitters` | string[] | [] | Emitter JSON 파일 경로 배열 |

```json
{
  "Name": "CampFire",
  "Looping": true,
  "Duration": 5.0,
  "PlayRate": 1.0,
  "PreWarmTime": 1.0,
  "State": "Play",
  "Emitters": [
    "Particles/Emitters/fire_core.json",
    "Particles/Emitters/fire_smoke.json"
  ]
}
```

---

## 4. Emitter JSON 스키마

> 소스: `ParticleLoader.cpp` — `ParticleLoader::ApplyJsonTo<ParticleEmitter>`

### 이미터 수준 필드

| 필드 | 타입 | 설명 |
|------|------|------|
| `Name` | string | 문서화/가독성 용도. 파싱 시 명시적으로 스킵되며 이미터에 적용되지 않음 |
| `Duration` | float | 이미터 방출 지속 시간 (초). 생략하거나 음수면 무한 방출 |
| `CompletionDelay` | float | 방출 종료 후 파티클 소멸까지 대기 시간 |
| `SubEmitters` | object[] | 서브 이미터 배열 ([7절](#7-서브-이미터) 참조) |
| `overdrawControl` | object | 오버드로우 제어 ([8절](#8-오버드로우-제어) 참조) |

위 필드를 제외한 나머지 JSON 키는 모듈 팩토리로 전달되어 해당 모듈을 생성한다.

> **Duration 무한 방출 동작**: `Duration` 필드를 생략하거나 음수 값(-1 등)을 지정하면 이미터가 영구히 방출을 계속한다. 이 경우 `OnDurationEnd`와 `OnComplete` 서브 이미터 트리거가 **영구히 발생하지 않는다**. 이 트리거를 사용하는 서브 이미터를 활성화하려면 반드시 양수 `Duration`을 설정해야 한다.

```json
{
  "Name": "FireCore",
  "Duration": 3.0,
  "CompletionDelay": 1.0,
  "Spawn": { ... },
  "Visual": { ... },
  "Force": { ... },
  "Material": { ... },
  "BillboardRender": { ... }
}
```

---

## 5. 모듈 레퍼런스

### 5.1 Spawn 모듈

> 소스: `SpawnModule.cpp` — `SpawnModule::LoadFromJson`
> JSON 키: `"Spawn"`

파티클 생성 조건과 영역을 정의한다.

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `space` | string | "Local" | 시뮬레이션 공간: `"Local"` / `"World"` |
| `localPos` | [x,y,z] | [0,0,0] | 이미터 로컬 오프셋 위치 |
| `shape` | string | "Box" | 스폰 형태 (아래 표 참조) |
| `spawnVolume` | [x,y,z] | [0.05, 0.15, 0.05] | 스폰 영역 크기 (Box: 반길이, Sphere: 반지름) |
| `spawnInnerRatio` | float | 0.0 | 내부 빈 비율 (0~1, Sphere에서 속이 빈 구) |
| `spawnRate` | float | 50.0 | 초당 스폰 사이클 횟수 |
| `burst` | uint | 0 | 1회성 버스트 파티클 수 |
| `particlesPerSpawn` | uint | 10 | 사이클당 생성 파티클 수 |
| `maxParticles` | uint | 1024 | 최대 활성 파티클 수 |
| `lifeRange` | [min,max] | [0.1, 1.5] | 파티클 수명 범위 (초) |
| `bakedPath` | string | — | 텍스처에서 베이크된 위치 데이터 경로 (Texture shape용) |
| `positions` | [[x,y,z],...] | — | 커스텀 스폰 위치 배열 (Custom shape용) |

#### Shape 종류

| 값 | 내부 코드 | 설명 |
|----|-----------|------|
| `"Box"` | 0 | 박스 영역 내 랜덤 |
| `"Sphere"` | 1 | 구 영역 내 랜덤 (`spawnInnerRatio`로 속이 빈 구 가능) |
| `"Vertex"` | 2 | 메쉬 버텍스 위치에서 스폰 |
| `"Surface"` | 3 | 메쉬 표면에서 스폰 |
| `"Texture"` | 4 | 텍스처에서 베이크된 위치 (`bakedPath` 필요) |
| `"Custom"` | 5 | 직접 지정한 위치 배열 (`positions` 필요) |

```json
"Spawn": {
  "space": "World",
  "localPos": [0, 0, 0],
  "shape": "Sphere",
  "spawnVolume": [2, 2, 2],
  "spawnInnerRatio": 0.3,
  "spawnRate": 10.0,
  "particlesPerSpawn": 5,
  "maxParticles": 2048,
  "lifeRange": [1.0, 3.0]
}
```

---

### 5.2 Visual 모듈

> 소스: `VisualModule.cpp` — `VisualModule::LoadFromJson`
> JSON 키: `"Visual"`

파티클의 색상, 크기, 회전을 제어한다.

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `startColor` | [r,g,b,a] | [1,1,1,1] | 시작 색상 (RGBA, 0~1) |
| `endColor` | [r,g,b,a] | [1,1,1,0] | 종료 색상 |
| `sizeRange` | [start,end] | [1,1] | 시작/종료 크기 |
| `sizeRandomness` | float | 0.0 | 크기 랜덤 변동 정도 |
| `colorRandomness` | float | 0.0 | 색상 랜덤 변동 정도 |
| `rotation` | object | — | 회전 설정 (아래 참조) |
| `colorCurve` | CurveData | — | 시간에 따른 색상 변화 커브 |
| `alphaCurve` | CurveData | — | 시간에 따른 알파 변화 커브 |
| `sizeCurve` | CurveData | — | 시간에 따른 크기 변화 커브 |

#### rotation 오브젝트

| 필드 | 타입 | 설명 |
|------|------|------|
| `minRotation` | [x,y,z] | 최소 초기 회전 (라디안) |
| `maxRotation` | [x,y,z] | 최대 초기 회전 |
| `minRotSpeed` | [x,y,z] | 최소 회전 속도 (rad/s) |
| `maxRotSpeed` | [x,y,z] | 최대 회전 속도 |

```json
"Visual": {
  "startColor": [1.0, 0.6, 0.1, 1.0],
  "endColor": [1.0, 0.2, 0.0, 0.0],
  "sizeRange": [0.5, 2.0],
  "sizeRandomness": 0.3,
  "rotation": {
    "minRotation": [0, 0, -3.14],
    "maxRotation": [0, 0, 3.14],
    "minRotSpeed": [0, 0, -1.0],
    "maxRotSpeed": [0, 0, 1.0]
  },
  "alphaCurve": {
    "type": "BEZIER",
    "keyframes": [
      { "key": 0.0, "value": 0.0, "outTangent": 2.0 },
      { "key": 0.2, "value": 1.0 },
      { "key": 1.0, "value": 0.0, "inTangent": -1.0 }
    ]
  }
}
```

---

### 5.3 Force 모듈

> 소스: `ForceModule.cpp` — `ForceModule::LoadFromJson`
> JSON 키: `"Force"`

파티클에 작용하는 물리 힘을 정의한다.

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `velocity` | [x,y,z] | [0,0,0] | 초기 방출 속도 |
| `speedRange` | [min,max] | [1,1] | 속도 크기 범위 (랜덤) |
| `randomDir` | [x,y,z] | [0,0,0] | 축별 랜덤 방향 강도 |
| `gravity` | [x,y,z] | [0,0,0] | 중력 가속도 |
| `drag` | float | 0.0 | 공기 저항 계수 |
| `curlNoiseEnabled` | bool | false | Curl Noise 활성화 |
| `curlNoiseFrequency` | float | 1.0 | Curl Noise 주파수 |
| `curlNoiseStrength` | float | 1.0 | Curl Noise 강도 |
| `curlNoiseScrollSpeed` | [x,y,z] | [0,0,0] | Curl Noise 스크롤 속도 |
| `velocityCurve` | CurveData | — | 시간에 따른 속도 배율 커브 |
| `dragCurve` | CurveData | — | 시간에 따른 드래그 커브 |
| `gravityCurve` | CurveData | — | 시간에 따른 중력 커브 |
| `noiseStrengthCurve` | CurveData | — | 시간에 따른 노이즈 강도 커브 |

```json
"Force": {
  "velocity": [0, 3, 0],
  "speedRange": [0.8, 1.5],
  "randomDir": [0.5, 0.2, 0.5],
  "gravity": [0, -9.8, 0],
  "drag": 0.5,
  "curlNoiseEnabled": true,
  "curlNoiseFrequency": 2.0,
  "curlNoiseStrength": 0.8,
  "curlNoiseScrollSpeed": [0, 0.5, 0]
}
```

---

### 5.4 Vortex 모듈

> 소스: `VortexModule.cpp` — `VortexModule::LoadFromJson`
> JSON 키: `"Vortex"`

파티클에 회오리(소용돌이) 운동을 적용한다.

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `center` | [x,y,z] | [0,0,0] | 소용돌이 중심점 |
| `strength` | float | 1.0 | 회전 강도 |
| `axis` | [x,y,z] | [0,1,0] | 회전 축 |
| `vortexFalloff` | float | 0.0 | 거리에 따른 감쇠율 |
| `pull` | [min,max] | [0,0] | 중심으로 끌어당기는/밀어내는 힘 범위 |
| `strengthCurve` | CurveData | — | 시간에 따른 강도 커브 |

```json
"Vortex": {
  "center": [0, 0, 0],
  "strength": 5.0,
  "axis": [0, 1, 0],
  "vortexFalloff": 0.3,
  "pull": [-0.5, 0.2],
  "strengthCurve": {
    "type": "LINEAR",
    "keyframes": [
      { "key": 0.0, "value": 1.0 },
      { "key": 1.0, "value": 0.0 }
    ]
  }
}
```

---

### 5.5 Orbit 모듈

> 소스: `OrbitModule.cpp` — `OrbitModule::LoadFromJson`
> JSON 키: `"Orbit"`

파티클에 궤도 운동을 적용한다.

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `center` | [x,y,z] | [0,0,0] | 궤도 중심점 |
| `axis` | [x,y,z] | [0,1,0] | 궤도 회전 축 |
| `rotationRate` | float | 1.0 | 회전 속도 (rad/s) |
| `offset` | float | 0.0 | 초기 각도 오프셋 |
| `rateCurve` | CurveData | — | 시간에 따른 회전 속도 커브 |

```json
"Orbit": {
  "center": [0, 1, 0],
  "axis": [0, 1, 0],
  "rotationRate": 3.14,
  "offset": 0.5,
  "rateCurve": {
    "type": "SIN",
    "params": { "frequency": 2.0, "amplitude": 0.5, "offset": 1.0 }
  }
}
```

---

### 5.6 Material 모듈

> 소스: `MaterialModule.cpp` — `MaterialModule::LoadFromJson`
> JSON 키: `"Material"`

텍스처와 머티리얼을 지정한다. 두 가지 방식을 지원한다.

#### 방식 1: PBR Material JSON 파일 참조

| 필드 | 타입 | 설명 |
|------|------|------|
| `materials` | string[] | Material JSON 파일 경로 배열 |

#### 방식 2: 인라인 텍스처

| 필드 | 타입 | 설명 |
|------|------|------|
| `texture` | string | 텍스처 파일 경로 (albedo만 사용, 기본 머티리얼 자동 생성) |

#### 스프라이트 애니메이션

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `sprite.frameTiles` | [cols,rows] | — | 스프라이트 시트 분할 (열, 행) |
| `sprite.frameCount` | uint | — | 총 프레임 수 |
| `sprite.animDuration` | float | — | 애니메이션 위치 (0~1로 클램프, 수명 대비 비율) |
| `sprite.frameBlending` | bool | false | 프레임 간 보간 여부 |
| `sprite.animTime` | float | 0.0 | 애니메이션 시작 시간 (≥0) |

```json
"Material": {
  "texture": "Textures/particle_fire.png"
}
```

```json
"Material": {
  "materials": ["Materials/fire_pbr.json"],
  "sprite": {
    "frameTiles": [4, 4],
    "frameCount": 16,
    "animDuration": 1.0,
    "frameBlending": true,
    "animTime": 0.0
  }
}
```

---

### 5.7 BillboardRender 모듈

> 소스: `RenderModule.cpp` — `BillboardRenderModule::LoadFromJson`
> JSON 키: `"BillboardRender"`

카메라를 향하는 2D 빌보드로 렌더링한다.

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `blendMode` | string | "Additive" | 블렌드 모드 (아래 표 참조) |
| `lowResolution` | bool | false | 저해상도 렌더링 (Modulate/Opaque 불가) |
| `softDistance` | float | 0.0 | 소프트 파티클 페이드 시작 거리 |
| `softMaxDist` | float | 0.0 | 소프트 파티클 페이드 최대 거리 |
| `softNearDist` | float | 0.0 | 카메라 근접 페이드 거리 |
| `velocityStretchFactor` | float | 0.0 | 속도 방향 스트레칭 계수 |
| `noiseUVDistort` | object | — | UV 디스토션 설정 (아래 참조) |
| `alphaClipThreshold` | float | 0.0 | 알파 클리핑 임계값 |
| `solidCircle` | bool | false | 원형 파티클 (셰이더에서 원 마스크 적용) |
| `centerWhiteIntensity` | float | 0.0 | 중심부 백색 강도 (발광 효과) |

#### blendMode 종류

| 값 | 설명 | 특이사항 |
|----|------|----------|
| `"Additive"` | 가산 합성 (밝아짐) | 정렬 불필요 |
| `"AlphaBlend"` | 알파 블렌딩 | 깊이 정렬 필요 |
| `"Opaque"` | 불투명 | `lowResolution` 강제 false |
| `"Modulate"` | 곱셈 합성 (어두워짐) | `lowResolution` 강제 false |

#### noiseUVDistort 오브젝트

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `enabled` | bool | false | UV 디스토션 활성화 |
| `frequency` | float | 1.0 | 노이즈 주파수 |
| `strength` | float | 0.015 | 디스토션 강도 |
| `scrollSpeed` | [u,v] | [0, 0.5] | UV 스크롤 속도 |

```json
"BillboardRender": {
  "blendMode": "AlphaBlend",
  "lowResolution": true,
  "softDistance": 0.5,
  "softMaxDist": 2.0,
  "solidCircle": true,
  "centerWhiteIntensity": 0.5
}
```

---

### 5.8 MeshRender 모듈

> 소스: `RenderModule.cpp` — `MeshRenderModule::LoadFromJson`
> JSON 키: `"MeshRender"`

3D 메쉬로 파티클을 렌더링한다.

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `blendMode` | string | "Additive" | 블렌드 모드 (Billboard과 동일) |
| `lowResolution` | bool | false | 저해상도 렌더링 |
| `model` | string | — | 모델 파일 경로 |
| `basePath` | string | "" | 모델 기본 경로 |
| `isGLTF` | bool | false | GLTF 포맷 여부 |
| `defaultMesh` | int | 0 | 기본 메쉬 (0: Box, 1: Sphere) |

`model`을 지정하면 외부 모델을 로드하고, 지정하지 않으면 `defaultMesh`에 따라 Box 또는 Sphere를 사용한다.

```json
"MeshRender": {
  "blendMode": "Opaque",
  "model": "Models/debris.obj",
  "basePath": "Models/"
}
```

```json
"MeshRender": {
  "blendMode": "AlphaBlend",
  "defaultMesh": 1
}
```

---

## 6. 커브 시스템

> 소스: `CurveData.cpp` — `CurveData::FromJson`

커브는 파티클 수명(0~1)에 따른 속성 변화를 정의한다. 내부적으로 LUT(Look-Up Table)로 베이크되어 GPU에서 빠르게 샘플링된다.

### 커브 JSON 스키마

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `type` | string | "LINEAR" | 커브 타입 |
| `resolution` | uint | — | LUT 해상도 |
| `params` | object | — | 커브 파라미터 (SIN, COS, NOISE 등에 사용) |
| `keyframes` | object[] | — | 키프레임 배열 |

### 커브 타입

| 타입 | 설명 |
|------|------|
| `"LINEAR"` | 키프레임 간 선형 보간 |
| `"BEZIER"` | 키프레임 간 베지어 보간 (탄젠트 사용) |
| `"STEP"` | 키프레임 값 그대로 유지 (계단식) |
| `"SIN"` | 사인 함수 |
| `"COS"` | 코사인 함수 |
| `"NOISE"` | 노이즈 함수 |

### LUT 해상도

| resolution 값 | 등급 | 설명 |
|---------------|------|------|
| ≤ 64 | Low | 저해상도, 성능 우선 |
| ≤ 128 | Medium | 기본 |
| ≤ 256 | High | 고품질 |
| > 256 | Ultra | 최고 품질 (512) |

### params 오브젝트

| 필드 | 타입 | 설명 |
|------|------|------|
| `frequency` | float | SIN/COS/NOISE 주파수 |
| `amplitude` | float | SIN/COS/NOISE 진폭 |
| `offset` | float | 기본값 오프셋 |
| `seed` | float | NOISE 시드 |

### keyframes 배열

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `key` | float | 0.0 | 시간 위치 (0~1, 수명 비율) |
| `value` | float | 0.0 | 해당 시점의 값 |
| `inTangent` | float | 0.0 | 입력 탄젠트 (BEZIER) |
| `outTangent` | float | 0.0 | 출력 탄젠트 (BEZIER) |

### 커브 적용 가능한 속성

| 모듈 | 커브 필드 | 대상 속성 |
|------|-----------|-----------|
| Visual | `colorCurve` | 색상 보간 |
| Visual | `alphaCurve` | 알파 보간 |
| Visual | `sizeCurve` | 크기 보간 |
| Force | `velocityCurve` | 속도 배율 |
| Force | `dragCurve` | 드래그 배율 |
| Force | `gravityCurve` | 중력 배율 |
| Force | `noiseStrengthCurve` | Curl Noise 강도 |
| Vortex | `strengthCurve` | 회전 강도 |
| Orbit | `rateCurve` | 궤도 회전 속도 |

### 커브 예제

```json
"alphaCurve": {
  "type": "BEZIER",
  "resolution": 128,
  "keyframes": [
    { "key": 0.0, "value": 0.0, "outTangent": 3.0 },
    { "key": 0.1, "value": 1.0 },
    { "key": 0.8, "value": 1.0 },
    { "key": 1.0, "value": 0.0, "inTangent": -2.0 }
  ]
}
```

```json
"sizeCurve": {
  "type": "SIN",
  "params": { "frequency": 2.0, "amplitude": 0.3, "offset": 1.0 }
}
```

---

## 7. 서브 이미터

> 소스: `ParticleLoader.cpp` — `SubEmitters` 처리

서브 이미터는 특정 이벤트에 반응하여 추가 이미터를 활성화한다.

### SubEmitters 배열 항목

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `path` | string | — | 서브 이미터 JSON 경로 |
| `trigger` | string | — | 트리거 이벤트 |
| `inheritPosition` | bool | false | 부모 위치 상속 여부 |

### 트리거 이벤트

| 값 | 설명 |
|----|------|
| `"OnStart"` | 부모 이미터 시작 시 |
| `"OnDurationEnd"` | 부모 이미터 Duration 종료 시 |
| `"OnComplete"` | 모든 파티클 소멸 완료 시 |

```json
"SubEmitters": [
  {
    "path": "Particles/Emitters/smoke_trail.json",
    "trigger": "OnDurationEnd",
    "inheritPosition": true
  },
  {
    "path": "Particles/Emitters/ember.json",
    "trigger": "OnComplete",
    "inheritPosition": true
  }
]
```

---

## 8. 오버드로우 제어

> 소스: `ParticleEmitter.cpp` — `ParticleEmitter::LoadOverdrawSettings`

카메라 거리 기반으로 파티클 크기와 스폰 수를 조절하여 오버드로우를 줄인다.

### overdrawControl 오브젝트

```json
"overdrawControl": {
  "sizeScaling": { ... },
  "spawnLimiting": { ... }
}
```

#### sizeScaling (거리 기반 크기 축소)

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `enabled` | bool | false | 활성화 |
| `minDistance` | float | 2.0 | 축소 시작 거리 |
| `maxDistance` | float | 10.0 | 축소 종료 거리 (이 이상은 최대 축소) |
| `closeRangeScale` | float | 0.7 | 최소 거리에서의 크기 배율 |

#### spawnLimiting (거리 기반 스폰 제한)

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `enabled` | bool | false | 활성화 |
| `nearDistance` | float | 5.0 | 제한 시작 거리 |
| `farDistance` | float | 15.0 | 최대 제한 거리 |
| `nearSpawnRatio` | float | 0.5 | 가까울 때 스폰 비율 (0~1) |

```json
"overdrawControl": {
  "sizeScaling": {
    "enabled": true,
    "minDistance": 3.0,
    "maxDistance": 15.0,
    "closeRangeScale": 0.6
  },
  "spawnLimiting": {
    "enabled": true,
    "nearDistance": 5.0,
    "farDistance": 20.0,
    "nearSpawnRatio": 0.4
  }
}
```

---

## 9. Material JSON 스키마

> 소스: `MaterialSystem.cpp` — `MaterialSystem::CreateMaterialFromJson`

PBR 머티리얼 JSON 파일의 스키마이다. `Material` 모듈의 `materials` 배열에서 참조된다.

| 필드 | 타입 | 설명 |
|------|------|------|
| `Name` | string | 머티리얼 이름 (중복 시 캐시에서 재사용) |
| `Albedo` | [r,g,b] | 알베도 색상 팩터 (또는 `AlbedoFactor`) |
| `Roughness` | float | 거칠기 팩터 (또는 `RoughnessFactor`) |
| `Metallic` | float | 금속성 팩터 (또는 `MetallicFactor`) |
| `Emission` | [r,g,b] | 발광 팩터 (또는 `EmissionFactor`) |
| `Textures` | object | 텍스처 맵 경로 (아래 참조) |

### Textures 오브젝트

| 필드 | 설명 |
|------|------|
| `albedo` | 알베도 텍스처 |
| `normal` | 노멀 맵 |
| `metallic` | 금속성 맵 |
| `roughness` | 거칠기 맵 |
| `ao` | Ambient Occlusion 맵 |
| `emissive` | 발광 맵 |
| `height` | 하이트 맵 |

```json
{
  "Name": "FirePBR",
  "Albedo": [1.0, 0.5, 0.1],
  "Roughness": 0.8,
  "Metallic": 0.0,
  "Emission": [2.0, 1.0, 0.3],
  "Textures": {
    "albedo": "Textures/fire_albedo.png",
    "emissive": "Textures/fire_emissive.png"
  }
}
```

---

## 10. 이펙트 예제

### 10.1 불꽃 (Fire)

**System JSON** — `Particles/Systems/fire.json`
```json
{
  "Name": "Fire",
  "Looping": true,
  "Duration": 5.0,
  "PlayRate": 1.0,
  "PreWarmTime": 1.0,
  "State": "Play",
  "Emitters": [
    "Particles/Emitters/fire_core.json"
  ]
}
```

**Emitter JSON** — `Particles/Emitters/fire_core.json`
```json
{
  "Name": "FireCore",
  "Duration": 5.0,
  "Spawn": {
    "shape": "Sphere",
    "spawnVolume": [0.5, 0.5, 0.5],
    "spawnRate": 20.0,
    "particlesPerSpawn": 3,
    "maxParticles": 1024,
    "lifeRange": [0.5, 1.5]
  },
  "Visual": {
    "startColor": [1.0, 0.7, 0.2, 1.0],
    "endColor": [1.0, 0.1, 0.0, 0.0],
    "sizeRange": [0.3, 0.8],
    "alphaCurve": {
      "type": "BEZIER",
      "keyframes": [
        { "key": 0.0, "value": 0.0, "outTangent": 5.0 },
        { "key": 0.1, "value": 1.0 },
        { "key": 1.0, "value": 0.0 }
      ]
    }
  },
  "Force": {
    "velocity": [0, 2.5, 0],
    "speedRange": [0.8, 1.2],
    "randomDir": [0.3, 0.1, 0.3]
  },
  "Material": {
    "texture": "Textures/particle_default.png"
  },
  "BillboardRender": {
    "blendMode": "Additive",
    "solidCircle": true,
    "centerWhiteIntensity": 0.6
  }
}
```

---

### 10.2 연기 (Smoke)

**System JSON** — `Particles/Systems/smoke.json`
```json
{
  "Name": "Smoke",
  "Looping": true,
  "Duration": 8.0,
  "State": "Play",
  "Emitters": [
    "Particles/Emitters/smoke.json"
  ]
}
```

**Emitter JSON** — `Particles/Emitters/smoke.json`
```json
{
  "Name": "Smoke",
  "Duration": 8.0,
  "Spawn": {
    "shape": "Sphere",
    "spawnVolume": [0.8, 0.3, 0.8],
    "spawnRate": 5.0,
    "particlesPerSpawn": 2,
    "maxParticles": 1024,
    "lifeRange": [3.0, 5.0]
  },
  "Visual": {
    "startColor": [0.4, 0.4, 0.4, 0.6],
    "endColor": [0.2, 0.2, 0.2, 0.0],
    "sizeRange": [0.5, 3.0],
    "rotation": {
      "minRotation": [0, 0, -3.14],
      "maxRotation": [0, 0, 3.14],
      "minRotSpeed": [0, 0, -0.5],
      "maxRotSpeed": [0, 0, 0.5]
    },
    "sizeCurve": {
      "type": "LINEAR",
      "keyframes": [
        { "key": 0.0, "value": 0.0 },
        { "key": 1.0, "value": 1.0 }
      ]
    }
  },
  "Force": {
    "velocity": [0, 1.5, 0],
    "speedRange": [0.8, 1.0],
    "drag": 0.3,
    "curlNoiseEnabled": true,
    "curlNoiseFrequency": 1.5,
    "curlNoiseStrength": 0.4,
    "curlNoiseScrollSpeed": [0, 0.3, 0]
  },
  "Material": {
    "texture": "Textures/smoke.png"
  },
  "BillboardRender": {
    "blendMode": "AlphaBlend",
    "lowResolution": true,
    "softDistance": 0.5,
    "softMaxDist": 2.0
  }
}
```

---

### 10.3 스파크 버스트 (Spark Burst)

**System JSON** — `Particles/Systems/spark_burst.json`
```json
{
  "Name": "SparkBurst",
  "Looping": false,
  "Duration": 2.0,
  "State": "Play",
  "Emitters": [
    "Particles/Emitters/sparks.json"
  ]
}
```

**Emitter JSON** — `Particles/Emitters/sparks.json`
```json
{
  "Name": "Sparks",
  "Duration": 0.1,
  "CompletionDelay": 2.0,
  "Spawn": {
    "shape": "Sphere",
    "spawnVolume": [0.1, 0.1, 0.1],
    "burst": 64,
    "maxParticles": 1024,
    "lifeRange": [0.5, 1.5]
  },
  "Visual": {
    "startColor": [1.0, 0.9, 0.5, 1.0],
    "endColor": [1.0, 0.3, 0.0, 0.0],
    "sizeRange": [0.05, 0.02]
  },
  "Force": {
    "velocity": [0, 2, 0],
    "speedRange": [3.0, 8.0],
    "randomDir": [1.0, 1.0, 1.0],
    "gravity": [0, -9.8, 0],
    "drag": 0.5
  },
  "Material": {
    "texture": "Textures/particle_default.png"
  },
  "BillboardRender": {
    "blendMode": "Additive",
    "velocityStretchFactor": 0.3,
    "solidCircle": true
  }
}
```

---

### 10.4 토네이도 (Vortex Tornado)

**System JSON** — `Particles/Systems/tornado.json`
```json
{
  "Name": "Tornado",
  "Looping": true,
  "Duration": 10.0,
  "State": "Play",
  "Emitters": [
    "Particles/Emitters/tornado.json"
  ]
}
```

**Emitter JSON** — `Particles/Emitters/tornado.json`
```json
{
  "Name": "TornadoParticles",
  "Duration": 10.0,
  "Spawn": {
    "shape": "Sphere",
    "spawnVolume": [2.0, 0.5, 2.0],
    "spawnRate": 30.0,
    "particlesPerSpawn": 3,
    "maxParticles": 2048,
    "lifeRange": [2.0, 4.0]
  },
  "Visual": {
    "startColor": [0.6, 0.5, 0.4, 0.8],
    "endColor": [0.3, 0.3, 0.3, 0.0],
    "sizeRange": [0.3, 1.0]
  },
  "Force": {
    "velocity": [0, 3, 0],
    "speedRange": [0.5, 1.0]
  },
  "Vortex": {
    "center": [0, 0, 0],
    "strength": 8.0,
    "axis": [0, 1, 0],
    "vortexFalloff": 0.1,
    "pull": [-1.0, 0.5]
  },
  "Material": {
    "texture": "Textures/smoke.png"
  },
  "BillboardRender": {
    "blendMode": "AlphaBlend",
    "lowResolution": true
  }
}
```

---

### 10.5 궤도 (Orbit)

**System JSON** — `Particles/Systems/orbit.json`
```json
{
  "Name": "OrbitDemo",
  "Looping": true,
  "Duration": 10.0,
  "State": "Play",
  "Emitters": [
    "Particles/Emitters/orbit_particles.json"
  ]
}
```

**Emitter JSON** — `Particles/Emitters/orbit_particles.json`
```json
{
  "Name": "OrbitParticles",
  "Duration": 10.0,
  "Spawn": {
    "shape": "Box",
    "spawnVolume": [0.1, 0.1, 0.1],
    "spawnRate": 15.0,
    "particlesPerSpawn": 2,
    "maxParticles": 1024,
    "lifeRange": [3.0, 5.0]
  },
  "Visual": {
    "startColor": [0.3, 0.7, 1.0, 1.0],
    "endColor": [0.1, 0.3, 1.0, 0.0],
    "sizeRange": [0.15, 0.05]
  },
  "Force": {
    "velocity": [0, 0.5, 0],
    "speedRange": [0.8, 1.2]
  },
  "Orbit": {
    "center": [0, 0, 0],
    "axis": [0, 1, 0],
    "rotationRate": 2.0,
    "offset": 1.5
  },
  "Material": {
    "texture": "Textures/particle_default.png"
  },
  "BillboardRender": {
    "blendMode": "Additive",
    "solidCircle": true,
    "centerWhiteIntensity": 0.4
  }
}
```

---

### 10.6 메쉬 파티클 (Mesh)

**System JSON** — `Particles/Systems/mesh_particle.json`
```json
{
  "Name": "MeshParticle",
  "Looping": true,
  "Duration": 5.0,
  "State": "Play",
  "Emitters": [
    "Particles/Emitters/mesh_debris.json"
  ]
}
```

**Emitter JSON** — `Particles/Emitters/mesh_debris.json`
```json
{
  "Name": "MeshDebris",
  "Duration": 0.5,
  "CompletionDelay": 5.0,
  "Spawn": {
    "shape": "Sphere",
    "spawnVolume": [0.5, 0.5, 0.5],
    "burst": 32,
    "maxParticles": 1024,
    "lifeRange": [2.0, 4.0]
  },
  "Visual": {
    "startColor": [1.0, 1.0, 1.0, 1.0],
    "endColor": [0.5, 0.5, 0.5, 0.0],
    "sizeRange": [0.3, 0.1],
    "rotation": {
      "minRotSpeed": [-3.0, -3.0, -3.0],
      "maxRotSpeed": [3.0, 3.0, 3.0]
    }
  },
  "Force": {
    "velocity": [0, 5, 0],
    "speedRange": [2.0, 5.0],
    "randomDir": [1.0, 0.5, 1.0],
    "gravity": [0, -9.8, 0],
    "drag": 0.2
  },
  "Material": {
    "materials": ["Materials/debris_pbr.json"]
  },
  "MeshRender": {
    "blendMode": "Opaque",
    "defaultMesh": 0
  }
}
```

---

### 10.7 스프라이트 폭발 (Sprite Explosion)

**System JSON** — `Particles/Systems/sprite_explosion.json`
```json
{
  "Name": "SpriteExplosion",
  "Looping": false,
  "Duration": 2.0,
  "State": "Play",
  "Emitters": [
    "Particles/Emitters/sprite_explosion.json"
  ]
}
```

**Emitter JSON** — `Particles/Emitters/sprite_explosion.json`
```json
{
  "Name": "ExplosionSprite",
  "Duration": 0.05,
  "CompletionDelay": 2.0,
  "Spawn": {
    "shape": "Sphere",
    "spawnVolume": [1.0, 1.0, 1.0],
    "burst": 16,
    "maxParticles": 1024,
    "lifeRange": [0.8, 1.2]
  },
  "Visual": {
    "startColor": [1.0, 1.0, 1.0, 1.0],
    "endColor": [1.0, 0.8, 0.5, 0.0],
    "sizeRange": [1.5, 3.0]
  },
  "Force": {
    "velocity": [0, 1, 0],
    "speedRange": [1.0, 3.0],
    "randomDir": [1.0, 1.0, 1.0],
    "drag": 1.0
  },
  "Material": {
    "texture": "Textures/explosion_sheet.png",
    "sprite": {
      "frameTiles": [4, 4],
      "frameCount": 16,
      "animDuration": 1.0,
      "frameBlending": true
    }
  },
  "BillboardRender": {
    "blendMode": "Additive"
  }
}
```

---

### 10.8 소프트 파티클 + UV 디스토션

**System JSON** — `Particles/Systems/soft_distort.json`
```json
{
  "Name": "SoftDistort",
  "Looping": true,
  "Duration": 5.0,
  "State": "Play",
  "Emitters": [
    "Particles/Emitters/soft_distort.json"
  ]
}
```

**Emitter JSON** — `Particles/Emitters/soft_distort.json`
```json
{
  "Name": "SoftDistortEmitter",
  "Duration": 5.0,
  "Spawn": {
    "shape": "Box",
    "spawnVolume": [2, 0.5, 2],
    "spawnRate": 8.0,
    "particlesPerSpawn": 2,
    "maxParticles": 1024,
    "lifeRange": [2.0, 4.0]
  },
  "Visual": {
    "startColor": [0.2, 0.5, 1.0, 0.7],
    "endColor": [0.1, 0.2, 0.8, 0.0],
    "sizeRange": [1.0, 2.5]
  },
  "Force": {
    "velocity": [0, 0.5, 0],
    "drag": 0.2
  },
  "Material": {
    "texture": "Textures/soft_cloud.png"
  },
  "BillboardRender": {
    "blendMode": "AlphaBlend",
    "softDistance": 1.0,
    "softMaxDist": 3.0,
    "softNearDist": 0.5,
    "noiseUVDistort": {
      "enabled": true,
      "frequency": 3.0,
      "strength": 0.08,
      "scrollSpeed": [0.1, 0.2]
    },
    "alphaClipThreshold": 0.05
  }
}
```

---

### 10.9 서브 이미터 체인 (Explosion Chain)

**System JSON** — `Particles/Systems/explosion_chain.json`
```json
{
  "Name": "ExplosionChain",
  "Looping": false,
  "Duration": 1.0,
  "State": "Play",
  "Emitters": [
    "Particles/Emitters/chain_explosion.json"
  ]
}
```

**Emitter JSON (폭발)** — `Particles/Emitters/chain_explosion.json`
```json
{
  "Name": "Explosion",
  "Duration": 0.1,
  "CompletionDelay": 1.0,
  "Spawn": {
    "shape": "Sphere",
    "spawnVolume": [0.3, 0.3, 0.3],
    "burst": 32,
    "maxParticles": 1024,
    "lifeRange": [0.3, 0.8]
  },
  "Visual": {
    "startColor": [1.0, 0.8, 0.3, 1.0],
    "endColor": [1.0, 0.2, 0.0, 0.0],
    "sizeRange": [0.5, 2.0]
  },
  "Force": {
    "velocity": [0, 2, 0],
    "speedRange": [3.0, 6.0],
    "randomDir": [1.0, 1.0, 1.0],
    "drag": 2.0
  },
  "Material": {
    "texture": "Textures/particle_default.png"
  },
  "BillboardRender": {
    "blendMode": "Additive",
    "solidCircle": true,
    "centerWhiteIntensity": 0.8
  },
  "SubEmitters": [
    {
      "path": "Particles/Emitters/chain_smoke.json",
      "trigger": "OnDurationEnd",
      "inheritPosition": true
    },
    {
      "path": "Particles/Emitters/chain_ember.json",
      "trigger": "OnComplete",
      "inheritPosition": true
    }
  ]
}
```

**Emitter JSON (연기)** — `Particles/Emitters/chain_smoke.json`
```json
{
  "Name": "ChainSmoke",
  "Duration": 3.0,
  "Spawn": {
    "shape": "Sphere",
    "spawnVolume": [1.0, 1.0, 1.0],
    "spawnRate": 10.0,
    "particlesPerSpawn": 3,
    "maxParticles": 1024,
    "lifeRange": [2.0, 4.0]
  },
  "Visual": {
    "startColor": [0.3, 0.3, 0.3, 0.5],
    "endColor": [0.1, 0.1, 0.1, 0.0],
    "sizeRange": [1.0, 4.0],
    "rotation": {
      "minRotSpeed": [0, 0, -0.5],
      "maxRotSpeed": [0, 0, 0.5]
    }
  },
  "Force": {
    "velocity": [0, 2, 0],
    "drag": 0.5
  },
  "Material": {
    "texture": "Textures/smoke.png"
  },
  "BillboardRender": {
    "blendMode": "AlphaBlend"
  }
}
```

**Emitter JSON (잔불)** — `Particles/Emitters/chain_ember.json`
```json
{
  "Name": "ChainEmber",
  "Duration": 2.0,
  "Spawn": {
    "shape": "Sphere",
    "spawnVolume": [0.5, 0.5, 0.5],
    "spawnRate": 5.0,
    "particlesPerSpawn": 2,
    "maxParticles": 1024,
    "lifeRange": [1.0, 3.0]
  },
  "Visual": {
    "startColor": [1.0, 0.5, 0.1, 1.0],
    "endColor": [0.5, 0.1, 0.0, 0.0],
    "sizeRange": [0.05, 0.02]
  },
  "Force": {
    "velocity": [0, 1.5, 0],
    "speedRange": [0.5, 1.5],
    "randomDir": [0.5, 0.3, 0.5],
    "gravity": [0, -2.0, 0]
  },
  "Material": {
    "texture": "Textures/particle_default.png"
  },
  "BillboardRender": {
    "blendMode": "Additive",
    "solidCircle": true
  }
}
```

---

### 10.10 복합 이펙트 (Fire + Smoke + Sparks)

**System JSON** — `Particles/Systems/campfire.json`
```json
{
  "Name": "CampFire",
  "Looping": true,
  "Duration": 10.0,
  "PlayRate": 1.0,
  "PreWarmTime": 2.0,
  "State": "Play",
  "Emitters": [
    "Particles/Emitters/campfire_flame.json",
    "Particles/Emitters/campfire_smoke.json",
    "Particles/Emitters/campfire_sparks.json"
  ]
}
```

**Emitter JSON (불꽃)** — `Particles/Emitters/campfire_flame.json`
```json
{
  "Name": "CampFireFlame",
  "Duration": 10.0,
  "Spawn": {
    "shape": "Sphere",
    "spawnVolume": [0.3, 0.1, 0.3],
    "spawnRate": 25.0,
    "particlesPerSpawn": 3,
    "maxParticles": 1024,
    "lifeRange": [0.3, 0.8]
  },
  "Visual": {
    "startColor": [1.0, 0.8, 0.3, 1.0],
    "endColor": [1.0, 0.2, 0.0, 0.0],
    "sizeRange": [0.3, 0.6],
    "sizeRandomness": 0.2,
    "alphaCurve": {
      "type": "BEZIER",
      "keyframes": [
        { "key": 0.0, "value": 0.0, "outTangent": 5.0 },
        { "key": 0.15, "value": 1.0 },
        { "key": 0.7, "value": 0.8 },
        { "key": 1.0, "value": 0.0 }
      ]
    }
  },
  "Force": {
    "velocity": [0, 3.0, 0],
    "speedRange": [0.8, 1.3],
    "randomDir": [0.3, 0.1, 0.3],
    "curlNoiseEnabled": true,
    "curlNoiseFrequency": 3.0,
    "curlNoiseStrength": 0.3,
    "curlNoiseScrollSpeed": [0, 1.0, 0]
  },
  "Material": {
    "texture": "Textures/particle_default.png"
  },
  "BillboardRender": {
    "blendMode": "Additive",
    "solidCircle": true,
    "centerWhiteIntensity": 0.7
  }
}
```

**Emitter JSON (연기)** — `Particles/Emitters/campfire_smoke.json`
```json
{
  "Name": "CampFireSmoke",
  "Duration": 10.0,
  "Spawn": {
    "shape": "Sphere",
    "spawnVolume": [0.5, 0.2, 0.5],
    "localPos": [0, 1.5, 0],
    "spawnRate": 3.0,
    "particlesPerSpawn": 1,
    "maxParticles": 1024,
    "lifeRange": [4.0, 6.0]
  },
  "Visual": {
    "startColor": [0.3, 0.3, 0.3, 0.4],
    "endColor": [0.15, 0.15, 0.15, 0.0],
    "sizeRange": [0.8, 4.0],
    "colorRandomness": 0.1,
    "rotation": {
      "minRotation": [0, 0, -3.14],
      "maxRotation": [0, 0, 3.14],
      "minRotSpeed": [0, 0, -0.3],
      "maxRotSpeed": [0, 0, 0.3]
    }
  },
  "Force": {
    "velocity": [0, 1.0, 0],
    "drag": 0.3,
    "curlNoiseEnabled": true,
    "curlNoiseFrequency": 1.0,
    "curlNoiseStrength": 0.5,
    "curlNoiseScrollSpeed": [0, 0.2, 0]
  },
  "Material": {
    "texture": "Textures/smoke.png"
  },
  "BillboardRender": {
    "blendMode": "AlphaBlend",
    "lowResolution": true,
    "softDistance": 0.5,
    "softMaxDist": 2.0
  },
  "overdrawControl": {
    "sizeScaling": {
      "enabled": true,
      "minDistance": 3.0,
      "maxDistance": 12.0,
      "closeRangeScale": 0.6
    }
  }
}
```

**Emitter JSON (스파크)** — `Particles/Emitters/campfire_sparks.json`
```json
{
  "Name": "CampFireSparks",
  "Duration": 10.0,
  "Spawn": {
    "shape": "Sphere",
    "spawnVolume": [0.2, 0.1, 0.2],
    "spawnRate": 3.0,
    "particlesPerSpawn": 2,
    "maxParticles": 1024,
    "lifeRange": [1.0, 2.5]
  },
  "Visual": {
    "startColor": [1.0, 0.8, 0.3, 1.0],
    "endColor": [1.0, 0.3, 0.0, 0.0],
    "sizeRange": [0.04, 0.01]
  },
  "Force": {
    "velocity": [0, 4, 0],
    "speedRange": [1.0, 3.0],
    "randomDir": [0.8, 0.3, 0.8],
    "gravity": [0, -3.0, 0],
    "drag": 0.3
  },
  "Material": {
    "texture": "Textures/particle_default.png"
  },
  "BillboardRender": {
    "blendMode": "Additive",
    "velocityStretchFactor": 0.2,
    "solidCircle": true
  }
}
```

---

## 11. 팁과 모범 사례

### 성능

- **maxParticles는 1024의 배수 권장**: 내부 메모리 풀이 블록 단위(1024)로 할당한다. 비배수 값은 올림 처리되어 메모리가 낭비될 수 있다.
- **Additive 블렌드는 정렬이 필요 없다**: Additive 파티클은 순서에 관계없이 결과가 동일하므로 BitonicSort를 건너뛴다.
- **AlphaBlend는 깊이 정렬이 필요하다**: back-to-front 정렬이 자동 적용된다.
- **Modulate/Opaque는 lowResolution 불가**: Modulate는 클리어 값(0) 곱셈 문제, Opaque는 씬 뎁스 버퍼 직접 기록이 필요하여 저해상도 패스가 강제 비활성화된다.

### 시각 품질

- **PreWarmTime으로 초기 빈 화면 방지**: 루핑 이펙트에서 시작 직후 파티클이 없는 구간을 미리 시뮬레이션하여 채울 수 있다.
- **커브로 자연스러운 변화 연출**: 색상, 크기, 알파를 시간에 따라 커브로 제어하면 선형 보간보다 자연스러운 결과를 얻는다.
- **softDistance로 지오메트리 교차부 처리**: 파티클이 씬 지오메트리와 만나는 경계에서 하드 엣지가 보일 때 소프트 파티클을 사용한다.
- **solidCircle + centerWhiteIntensity**: 텍스처 없이 간단한 발광 파티클을 만들 수 있다.

### 워크플로우

- **Hot Reload 지원**: JSON 파일 수정 시 파일 워처가 변경을 감지하여 실시간 반영한다. 에디터를 재시작하지 않아도 된다.
- **서브 이미터로 이벤트 체인 구성**: 폭발 → 연기 → 잔불 같은 시퀀스를 서브 이미터 트리거로 자연스럽게 연결한다.
- **overdrawControl로 최적화**: 카메라에 가까운 파티클의 크기를 줄이거나 스폰을 제한하여 필레이트 부하를 줄인다.
- **Material 모듈의 두 가지 방식**: 간단한 이펙트는 `texture`로 인라인 지정하고, PBR이 필요한 메쉬 파티클은 `materials`로 별도 JSON을 참조한다.
