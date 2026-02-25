# GPU 파티클 시스템 포트폴리오

> **프로젝트**: DXEngine - DirectX 11 기반 GPU 파티클 시스템
> **기간**: 2026년 1월 6일 ~ 2월 25일 (8주)
> **핵심 기술**: Compute Shader, Indirect Drawing, LDS Bitonic Sort, Memory Pool, Batch Rendering

---

# Part 1: 주차별 개발 타임라인

---

## 1~2주차 (Jan 6 ~ Jan 18): 기초 GPU 파티클 시스템

### 핵심 구현
- **CPU → GPU 전환**: CPU 파티클 애니메이션에서 Compute Shader 기반 GPU 파티클로 전환
- **ConsumeAppend Buffer**: 파티클 생존/사망 관리를 GPU에서 원자적으로 처리
- **DrawInstancedIndirect**: GPU가 직접 드로우 인자를 결정하여 CPU-GPU 라운드트립 제거
- **모듈 시스템 설계**: ParticleModuleFactory 패턴으로 Spawn, Render 등 모듈 분리
- **JSON 기반 Data-Driven**: ParticleLoader를 통한 JSON 로딩 + FileWatcher Hot-Reload

### 기술 결정 (WHY)
- ConsumeAppend Buffer를 채택한 이유: GPU에서 lock-free로 파티클 인덱스를 관리하면 CPU 개입 없이 생사 판정 가능
- Indirect Drawing: DrawInstancedIndirect로 GPU가 자체적으로 인스턴스 수를 결정 → CPU readback 제거
- Module Factory 패턴: 이펙트마다 다른 조합의 모듈을 JSON에서 선언적으로 구성 가능

### 커밋 흐름
```
Jan 06  Scene 분리 → ParticleEditor Scene 생성 → Particle Temp Class
Jan 07  CPU Particle Animation Basic/Animation
Jan 08  Basic Particle Compute Shader → ConsumeAppend Buffer → Indirect → Spawn CS → GPU Spawn → spawnRate
Jan 09  Basic GPU Particle Animation → ViewProj Transform
Jan 12  Bitonic Sort → Module 시도 → Constant Data Refactoring → Module Factory → JSON Loader
Jan 13  Hot-Reload → Vortex Effect → Billboard Bug Fix
Jan 14  ParticleSystem 구축 → Scene 통합 → Depth Testing
Jan 15  Texture2DArray → Texture Support → Sprite Animation
Jan 16  Particle Sprite Animation by life
```

---

## 3주차 (Jan 19 ~ Jan 25): 3D Mesh 파티클 & 재질 시스템

### 핵심 구현
- **3D Mesh Particle**: Billboard 외에 3D 모델을 파티클로 사용 (Vertex Shader에서 인스턴싱)
- **PBR Material Module**: Metallic/Roughness 분리, 재질 데이터를 JSON으로 저장/로드
- **6가지 Spawn 타입**: Box, Sphere, Vertex, Surface, Texture, Custom
- **TextureSpawnBake**: 텍스처 색상 기반으로 메시 표면의 스폰 위치를 GPU에서 베이킹
- **ParticleManager 도입**: 전체 파티클 시스템의 중앙 관리자

### 기술 결정 (WHY)
- 3D Mesh 파티클: 파편, 잔해 등 비정형 파티클 표현을 위해 메시 렌더링 파이프라인 추가
- Texture Spawn Bake: 텍스처의 특정 색상/채널 위치에서만 파티클을 방출하기 위해 사전 베이킹 방식 채택 → 런타임 부하 제로

### 커밋 흐름
```
Jan 19  Refactoring Constant & Module → 3D Mesh Particle → Vertex Shader Fix
Jan 20  3D Model Particle → PBR Material → Material Module → Save Material to JSON
Jan 21  Material System → Vertex Spawn → Surface Spawn → Texture Spawn 시도
Jan 22  TextureSpawnBake Class → Spawn on Texture Position → Simulation Space
Jan 23  ParticleManager 작업 시작 → Refactoring with ModelManager
Jan 24  ParticleManager 추가 → ComputePSO 사용
Jan 25  BitonicSort using ComputePSO → Buffer Refactoring → AlphaBlend 전용 정렬 → 최적화
```

---

## 4주차 (Jan 26 ~ Feb 1): EffectActor & SubEmitter

### 핵심 구현
- **EffectActor**: 다중 이펙트를 지원하는 이펙트 액터 시스템
- **SubEmitter System**: 이벤트 기반 연쇄 파티클 (OnStart, OnDurationEnd, OnComplete)
- **Double Buffering**: Ping-Pong Alive Indices로 시뮬레이션/렌더링 분리
- **StructuredBuffer 전환**: ConstantBuffer에서 StructuredBuffer로 마이그레이션

### 기술 결정 (WHY)
- SubEmitter: 폭발 후 연기, 충돌 시 스파크 등 연쇄적 이펙트 표현을 위한 이벤트 시스템
- Double Buffering: 시뮬레이션 중 렌더링이 읽는 인덱스가 변경되는 문제를 Ping-Pong 패턴으로 해결
- StructuredBuffer: Constant Buffer의 64KB 제한 없이 대규모 파티클 데이터 전달

### 커밋 흐름
```
Jan 26  EffectActor Multi-Effects → Burst → OrbitModule → SubEmitter System
Jan 27  SubEmitter System 완성 → ParticleManager 중앙 관리로 이전
Jan 28  Effect Classes → GPU Buffer 초기화 최적화 → ObjectPool 패턴 → StructuredBuffer 전환
Jan 29  Double Buffering 시도/구현 → GPU Memory Pool 시작 → Buffer 이전
Jan 30  Bitonic Sort 제거 (임시) → SubEmitter 수정
Feb 01  Bug Fixes → SubEmitter Spawn Bug → CPU/GPU Refactoring
```

---

## 5~6주차 (Feb 2 ~ Feb 13): Memory Pool & GPU Compacting & 배치 렌더링

### 핵심 구현
- **ParticleMemoryPool**: map 기반 블록 할당/해제 (O(log m))
- **GPU Compacting**: ParticleCS에서 시뮬레이션과 동시에 컴팩팅 (단일 패스)
- **Fragmentation Tracking**: dirty flag 패턴으로 O(1) 단편화율 조회
- **Batch Rendering**: Material 기반 BatchGroup → 2-Pass Compute → DrawIndexedInstancedIndirect
- **GS 제거 → VS Billboard**: Geometry Shader 병목 제거
- **CPU Frustum Culling**: BoundingSphere-Frustum 테스트
- **LOD & Priority-Based Eviction**: 거리 기반 SpawnRate 조절 + 우선순위 기반 시스템 퇴거
- **Global CS Dispatch**: 개별 디스패치를 통합하여 오버헤드 감소

### 기술 결정 (WHY)
- Memory Pool 진화: Defrag 시도 → Paging → GPU Compacting 최종 채택. GPU Compacting이 가장 단순하고 효율적
- Batch Rendering: 시스템별 개별 드로우콜 → Material 기준 배치로 드로우콜 대폭 감소
- GS 제거: Geometry Shader의 파이프라인 병목이 심각 → VS에서 직접 쿼드 확장
- Priority Eviction: 메모리 부족 시 가장 중요하지 않은 시스템부터 제거하여 graceful degradation

### 커밋 흐름
```
Feb 02  Memory Pool 시작 → Multi Effect → Dispatch/Mesh Args to Pool → Constant Buffer to Pool
Feb 03  ProcessWaitingQueue → Defragmentation 시도 → Page Memory Pool → Discontinuous Block
Feb 04  Memory Pool Debug → Paging → GPU Compacting 전환 → Offset Bug Fix
Feb 05  Defrag Timing → Stress Test → Profiling → CreateSystem 최적화 → Global CS Dispatch 시도
Feb 06  Remove unused vectors → Combine Orbit&Vortex → Dynamic StructuredBuffer
Feb 07  Global CS Dispatch 완성 → Fix Vortex & Orbit
Feb 08  GS 제거 → VS Billboard → CPU Frustum Culling → GPU Culling → LOD → Priority Base
Feb 09  LOD SpawnLimiting Fix → Priority Eviction 개선 → Low Resolution Rendering
Feb 11  Off-Screen Particles → Batch Rendering 계획 → Phase 1 구현 → Billboard Batch by Material
Feb 12  Mesh Batch → Memory Pool Restructuring → Single Buffer + Index Tracking → GPU Compacting Single Pass
Feb 13  StressTest with EffectActor → Batch Deletion 최적화
```

---

## 7주차 (Feb 14 ~ Feb 22): 렌더링 품질 & LDS Bitonic Sort

### 핵심 구현
- **AlphaBlend 파이프라인**: Opaque/AlphaBlend 분리 렌더링
- **Soft Particles**: 씬 깊이 비교로 교차 부분 알파 페이드 (Full/Low Res 이중 깊이)
- **Velocity Stretch Billboard**: 속도 방향 View Space 투영으로 쿼드 늘림
- **Curl Noise**: 유체 느낌 파티클 움직임
- **2D Noise & UV Distortion**: 텍스처 왜곡 효과
- **Curve Data → LUT**: CPU에서 커브 샘플링 → Texture2DArray로 변환 → GPU 고속 조회
- **LDS Bitonic Sort 3-Phase**: groupshared memory 활용 최적화 정렬
- **Bloom Effect**: 포스트 프로세싱 발광 효과

### 기술 결정 (WHY)
- Soft Particles: 파티클이 지형과 교차할 때 하드엣지 대신 부드러운 페이드 → 시각 품질 대폭 향상
- LDS Bitonic Sort: AlphaBlend 파티클의 back-to-front 정렬 필수. LDS 사용으로 global memory 접근 최소화
- Curve → LUT: 런타임에 커브 계산 대신 미리 샘플링한 LUT 텍스처 사용 → GPU에서 O(1) 조회

### 커밋 흐름
```
Feb 15  Billboard Render Bug Fix → TargetMesh/BakedSpawnPos/CustomPos Reuse
Feb 17  AlphaBlend → Sorting → Sprite Animation ratio base → Soft Particles → Velocity Stretch Billboard
Feb 18  Curl Noise → Down/Upsampling Fix
Feb 19  Scene-Only Depth for Soft Particles → 2D Noise → UV Distortion → Curve Data Class → LUT
Feb 20  Curve Test → Curve Data 완성
Feb 21  UV Distortion JSON Bug Fix → Sort Bug Fix
Feb 22  Low/High Resolution 분리 → BlendMode → Solid Circle → Bloom Effect → LDS Bitonic Merge Sort
```

---

## 8주차 (Feb 23 ~ Feb 25): 이펙트 제작 & 최종 마무리

### 핵심 구현
- **다양한 이펙트 제작**: Rain, ArcaneCircle, Flash, Explosion 등
- **씬 배치 및 통합 테스트**: 여러 이펙트를 씬에 배치하여 실제 환경 테스트
- **AlphaBlend Overdraw 최적화**: 파티클 겹침 제어
- **최종 문서 정리**

### 커밋 흐름
```
Feb 23  New Effect → Final Presentation TOC → Billboard Normal → Effect Test
Feb 24  Rain → ArcaneCircle → Effect 배치 → AlphaBlend Overdraw → Boundary Bug Fix
Feb 25  배치 업데이트 → 최종 업데이트
```

---
---

# Part 2: 핵심 기술 심화 설명

---

## 1. GPU 파티클 아키텍처

### WHAT
GPU Compute Shader 기반 파티클 시스템. 파티클의 생성, 시뮬레이션, 정렬, 렌더링이 모두 GPU에서 수행된다.

### WHY
- CPU 파티클의 한계: 수만 개 파티클에서 CPU 병목 발생
- GPU 병렬 처리: 수십만 파티클을 동시에 시뮬레이션 가능
- CPU-GPU 데이터 전송 최소화: Indirect Drawing으로 readback 불필요

### HOW
1. **Dead/Alive Index 관리**: ConsumeAppend Buffer로 사용 가능한 파티클 인덱스를 GPU에서 원자적으로 관리
2. **Spawn**: DeadIndices에서 Consume하여 새 파티클 할당, WriteAliveIndices에 Append
3. **Simulate**: ReadAliveIndices에서 읽어 시뮬레이션 후, 생존 파티클을 WriteAliveIndices에 기록
4. **Render**: DrawInstancedIndirect로 GPU가 결정한 인스턴스 수만큼 렌더링

### 코드: SpawnCS.hlsl - Consume/Append 패턴

```hlsl
ConsumeStructuredBuffer<uint> deadIndices : register(u4);

[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint spawnIdx = dtID.x;
    if (spawnIdx >= spawnCount) return;

    // Dead 리스트에서 인덱스를 가져옴 (원자적)
    uint particleIdx = deadIndices.Consume();

    Particle p;
    p.life = lerp(lifeRange.x, lifeRange.y, rand_float(rngState));
    p.position = SpawnPosition(rngState);  // Spawn 타입에 따른 위치 결정
    // ... 초기화 ...

    particles[particleIdx] = p;

    // Alive 리스트에 기록 (원자적 카운터)
    uint aliveSlot;
    InterlockedAdd(writeAliveCount[emitterID], 1, aliveSlot);
    writeAliveIndices[readParticleOffset + aliveSlot] = particleIdx;
}
```

### 코드: Indirect Buffer 구조

```hlsl
// BatchRenderArgsCS.hlsl - GPU가 직접 Draw 인자를 결정
uint argsIdx = batchID * 5;
batchBillboardArgs[argsIdx + 0] = batch.indexCount;       // IndexCountPerInstance
batchBillboardArgs[argsIdx + 1] = totalInstances;         // InstanceCount (GPU가 결정)
batchBillboardArgs[argsIdx + 2] = batch.startIndexLocation;
batchBillboardArgs[argsIdx + 3] = batch.baseVertexLocation;
batchBillboardArgs[argsIdx + 4] = 0;                      // StartInstanceLocation
```

---

## 2. 모듈 시스템 & Data-Driven

### WHAT
파티클의 각 기능(Spawn, Visual, Force, Material, Render)을 독립된 모듈로 분리하고, JSON 파일에서 선언적으로 조합하는 시스템.

### WHY
- 이펙트마다 다른 기능 조합이 필요 (예: 불꽃은 Gravity + Noise, 비는 Gravity만)
- 코드 수정 없이 JSON만으로 새 이펙트 생성/수정 가능
- Hot-Reload로 런타임 중 즉시 반영 → 이터레이션 속도 향상

### HOW

#### Priority 기반 실행 순서

```cpp
// ParticleModule.h
enum class ModulePriority : uint8_t {
    Spawn    = 1,  // 파티클 생성
    Visual   = 2,  // 시각적 속성 (크기, 색상)
    Force    = 3,  // 외력 (중력, 노이즈)
    UpdateForce = 4, // 2차 외력 (와류, 궤도)
    Material = 5,  // 재질 (PBR 텍스처)
    Render   = 6   // 렌더링 방식 (Billboard, Mesh)
};
```

#### Factory 패턴

```cpp
// ParticleModuleFactory.h
class ParticleModuleFactory {
public:
    using Creator = std::function<std::unique_ptr<ParticleModule>()>;

    template<typename T>
    static void Register(const std::string& type) {
        Register(type, []() -> std::unique_ptr<ParticleModule> {
            return std::make_unique<T>();
        });
    }

    static std::unique_ptr<ParticleModule> Create(const std::string& type);

private:
    static std::map<std::string, Creator>& GetRegistry();
};
```

#### JSON → Module 로딩

```cpp
// ParticleLoader.cpp
template<>
void ParticleLoader::ApplyJsonTo<ParticleEmitter>(ParticleEmitter* target, const json& jsonData) {
    target->ClearModules();

    for (auto& [key, value] : jsonData.items()) {
        if (key == "Name" || key == "Duration" || key == "SubEmitters")
            continue;

        // Factory에서 모듈 이름으로 생성
        auto module = ParticleModuleFactory::Create(key);
        if (module) {
            module->LoadFromJson(value);
            target->AddModule(std::move(module));
        }
    }
}
```

#### Hot-Reload

```cpp
// ParticleLoader.cpp - FileWatcher 등록
T* rawPtr = instance.get();
auto callback = [rawPtr, fullPath]() {
    std::ifstream reloadFile(fullPath);
    if (reloadFile.is_open()) {
        json newJson;
        reloadFile >> newJson;
        ApplyJsonTo(rawPtr, newJson);
    }
};

auto id = FileWatcher::Get().Register(fullPath, callback);
instance->SetHotReloadInfo(fullPath, id);
```

---

## 3. Spawn 시스템 확장

### WHAT
6가지 Spawn 타입(Box, Sphere, Vertex, Surface, Texture, Custom)을 지원하는 확장 가능한 스폰 시스템.

### WHY
- 다양한 이펙트 표현: 박스(폭발 파편), 구(에너지 구체), 메시 표면(캐릭터에서 발산), 텍스처(문양 따라 발생)
- Texture Spawn: 텍스처의 특정 색상 영역에서만 파티클 발생 → 복잡한 패턴 지원

### HOW

#### SpawnCS.hlsl - Spawn 타입별 함수

```hlsl
// Box Spawn: 볼륨 내 랜덤 위치, 내부 비율로 속이 빈 박스 지원
float3 BoxSpawn(inout uint rngState, float3 volume, float innerRatio)
{
    float3 pos = float3(rand_signed(rngState), rand_signed(rngState), rand_signed(rngState));
    float3 hollowScale = lerp(innerRatio, 1.0f, abs(pos));
    return sign(pos) * hollowScale * volume;
}

// Sphere Spawn: theta-z 파라미터화로 균일한 구면 분포
float3 SphereSpawn(inout uint rngState, float3 volume, float innerRatio)
{
    float theta = rand_float(rngState) * 6.28318530718f;
    float z = rand_float(rngState) * 2.0f - 1.0f;
    float r = sqrt(max(0.0f, 1.0f - z * z));
    float3 dir = float3(r * cos(theta), r * sin(theta), z);
    float dist = lerp(innerRatio, 1.0f, rand_float(rngState));
    return dir * dist * volume;
}

// Surface Spawn: 삼각형 위 무게중심 좌표 보간 → 법선 상속
float3 SurfaceSpawn(inout uint rngState, ...) {
    uint triIdx = rand_uint(rngState) % (indexCount / 3);
    float u = rand_float(rngState);
    float v = rand_float(rngState);
    if (u + v > 1.0f) { u = 1.0f - u; v = 1.0f - v; }
    float w = 1.0f - u - v;
    // 무게중심 좌표로 위치 보간
    return w * v0 + u * v1 + v * v2;
}
```

#### TextureSpawnBakeCS.hlsl - 텍스처 기반 위치 베이킹

```hlsl
// 텍스처의 색상 조건을 만족하는 메시 표면 위치를 사전에 GPU 베이킹
[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint triIdx = dtID.x;
    if (triIdx >= indexCount / 3) return;

    // 삼각형의 UV 공간에서 텍스처 픽셀을 순회
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            float2 pixelCenter = float2(x, y) + 0.5f;
            float3 bc = CalculateBarycentric(uv0, uv1, uv2, pixelCenter);

            if (bc.x >= -1e-5f && bc.y >= -1e-5f && bc.z >= -1e-5f) {
                // 텍스처 색상 조건 검사 (채널 마스크 & 임계값)
                if (CheckTextureCondition(int2(x, y), w, h)) {
                    float3 pos = bc.x * v0.position + bc.y * v1.position + bc.z * v2.position;
                    outputPos.Append(pos);  // AppendStructuredBuffer에 추가
                }
            }
        }
    }
}

// UV Seam 감지 - 텍스처 래핑 아티팩트 방지
bool isSeam = (length(uv0_raw - uv1_raw) > 0.8f) ||
              (length(uv1_raw - uv2_raw) > 0.8f) ||
              (length(uv2_raw - uv0_raw) > 0.8f);
if (isSeam) return;
```

---

## 4. SubEmitter System

### WHAT
파티클 이미터의 생명주기 이벤트에 반응하여 새로운 이미터를 생성하는 계층적 파티클 시스템.

### WHY
- 폭발 → 연기, 불꽃 → 잔불 등 자연스러운 연쇄 이펙트 표현
- 이벤트 기반이므로 타이밍을 선언적으로 제어 가능

### HOW

```cpp
// ParticleEmitter.h - 이벤트 타입 정의
enum class EmitterEvent : uint8_t {
    OnStart,        // 이미터가 시작될 때
    OnDurationEnd,  // Duration이 끝날 때
    OnComplete      // 모든 파티클이 사라진 후
};

struct SubEmitter {
    std::wstring emitterPath;                    // JSON 프리셋 경로
    EmitterEvent trigger = EmitterEvent::OnComplete;
    bool inheritPosition = true;                 // 부모 파티클 위치 상속
};
```

```cpp
// ParticleLoader.cpp - JSON에서 SubEmitter 로딩
if (jsonData.contains("SubEmitters") && jsonData["SubEmitters"].is_array()) {
    for (const auto& subJson : jsonData["SubEmitters"]) {
        SubEmitter sub;

        if (subJson.contains("path"))
            sub.emitterPath = std::wstring(path.begin(), path.end());

        if (subJson.contains("trigger")) {
            std::string trigger = subJson["trigger"];
            if (trigger == "OnStart")       sub.trigger = EmitterEvent::OnStart;
            else if (trigger == "OnDurationEnd") sub.trigger = EmitterEvent::OnDurationEnd;
            else if (trigger == "OnComplete")    sub.trigger = EmitterEvent::OnComplete;
        }

        if (subJson.contains("inheritPosition"))
            sub.inheritPosition = subJson["inheritPosition"];

        target->AddSubEmitter(sub);
    }
}
```

---

## 5. ParticleMemoryPool & GPU Compacting

### WHAT
모든 파티클 시스템이 하나의 공유 GPU 버퍼를 사용하되, 블록 단위로 할당/해제하는 메모리 풀. 파티클 시뮬레이션과 동시에 alive index 컴팩팅을 단일 패스로 수행.

### WHY
- 시스템마다 별도 GPU 버퍼 → 메모리 낭비 + 배치 불가
- 통합 버퍼 + 블록 할당 → 배치 렌더링 가능 + 메모리 효율
- GPU Compacting 진화 과정:
  1. **Defrag 시도**: CPU에서 블록 재배치 → GPU 메모리 복사 비용 과다
  2. **Paging**: 페이지 단위 관리 → 복잡도 증가, 이점 미미
  3. **GPU Compacting 채택**: 시뮬레이션 패스에서 자연스럽게 alive index 재기록 → 추가 비용 제로

### HOW

#### 블록 할당: map 기반 Gap Finding O(m log m)

```cpp
// ParticleMemoryPool.cpp
PoolHandle ParticleMemoryPool::Allocate(UINT reqParticleCount, UINT reqEmitterCount,
                                         UINT reqSpawnPosCount) {
    PoolHandle handle;
    UINT neededBlocks = (reqParticleCount + m_blockSize - 1) / m_blockSize;
    UINT foundBlock = UINT_MAX;

    // map의 할당된 블록들 사이 빈 공간을 탐색
    UINT searchStart = 0;
    for (const auto& [allocatedStart, allocatedCount] : m_particleBlockMap) {
        if (allocatedStart >= searchStart + neededBlocks) {
            foundBlock = searchStart;  // 빈 공간 발견
            break;
        }
        searchStart = allocatedStart + allocatedCount;
    }

    // 마지막 블록 뒤에 공간이 있는지 확인
    if (foundBlock == UINT_MAX && searchStart + neededBlocks <= m_blockCount) {
        foundBlock = searchStart;
    }

    handle.particleOffset = foundBlock * m_blockSize;
    handle.blockCount = neededBlocks;
    m_particleBlockMap[foundBlock] = neededBlocks;
    m_fragmentationDirty = true;
    return handle;
}
```

#### Free: Reference Counting

```cpp
void ParticleMemoryPool::Free(const PoolHandle& handle) {
    if (!handle.IsActive()) return;

    // 파티클 블록 해제
    UINT startBlock = handle.particleOffset / m_blockSize;
    auto it = m_particleBlockMap.find(startBlock);
    if (it != m_particleBlockMap.end()) {
        m_cachedUsedBlocks -= it->second;
        m_particleBlockMap.erase(it);
    }

    // SpawnPos 캐시 - 참조 카운트 감소
    if (!handle.bakedPosKey.empty()) {
        auto cacheIt = m_spawnPosCache.find(handle.bakedPosKey);
        if (cacheIt != m_spawnPosCache.end()) {
            cacheIt->second.refCount--;
            if (cacheIt->second.refCount == 0) {
                m_spawnPosBlockMap.erase(cacheIt->second.offset / m_blockSize);
                m_spawnPosCache.erase(cacheIt);
            }
        }
    }

    m_fragmentationDirty = true;
}
```

#### Fragmentation Tracking: dirty flag O(1)

```cpp
float ParticleMemoryPool::GetFragmentationRatio() const {
    // dirty flag가 설정된 경우에만 재계산
    if (m_fragmentationDirty) {
        m_cachedLastUsedBlock = 0;
        m_cachedTotalUsedBlocks = m_cachedUsedBlocks;

        for (const auto& [blockStart, blockCount] : m_particleBlockMap) {
            UINT blockEnd = blockStart + blockCount;
            if (blockEnd > m_cachedLastUsedBlock)
                m_cachedLastUsedBlock = blockEnd;
        }
        m_fragmentationDirty = false;
    }

    if (m_cachedLastUsedBlock == 0 || m_cachedTotalUsedBlocks == 0) return 0.0f;

    UINT gapBlocks = m_cachedLastUsedBlock - m_cachedTotalUsedBlocks;
    return static_cast<float>(gapBlocks) / m_cachedLastUsedBlock;
    // 20% 임계값 초과 시 Defrag 트리거
}
```

#### Defragment: 블록 재배치

```cpp
std::vector<UINT> ParticleMemoryPool::Defragment(const std::vector<PoolHandle>& activeHandles) {
    std::vector<UINT> newOffsets;
    newOffsets.reserve(activeHandles.size());

    m_particleBlockMap.clear();

    // 모든 활성 블록을 연속으로 재배치
    UINT currentBlock = 0;
    for (const auto& handle : activeHandles) {
        newOffsets.push_back(currentBlock * m_blockSize);
        m_particleBlockMap[currentBlock] = handle.blockCount;
        currentBlock += handle.blockCount;
    }

    m_cachedUsedBlocks = currentBlock;
    m_fragmentationDirty = true;
    return newOffsets;  // 각 핸들의 새 오프셋 반환 → GPU 복사에 사용
}
```

#### GPU Compacting: ParticleCS.hlsl 단일 패스

```hlsl
// ParticleCS.hlsl - 시뮬레이션 + 컴팩팅을 동시에 수행
[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint aliveIdx = dtID.x;
    uint particleIdx = readAliveIndices[aliveIdx];
    Particle p = particles[particleIdx];

    // 수명 감소
    p.life -= dt;
    if (p.life <= 0.f) {
        deadIndices.Append(particleIdx);  // 사망 → Dead 리스트로
        return;
    }

    // ... 물리 시뮬레이션 (중력, 드래그, 노이즈 등) ...

    particles[particleIdx] = p;

    // 단일 패스 컴팩팅: InterlockedAdd로 연속된 alive index 기록
    uint writeSlot;
    InterlockedAdd(writeAliveCount[p.ownerID], 1, writeSlot);
    writeAliveIndices[eID.readParticleOffset + writeSlot] = particleIdx;
}
```

---

## 6. 배치 렌더링 파이프라인

### WHAT
같은 Material을 사용하는 파티클 이미터들을 하나의 BatchGroup으로 묶어 단일 DrawIndexedInstancedIndirect 호출로 렌더링.

### WHY
- 이미터별 개별 드로우콜 → 수십~수백 회 드로우콜 발생
- Material 기준 배치 → 드로우콜 대폭 감소 (동일 셰이더/텍스처 묶음)
- Billboard/Mesh 분리: 렌더링 파이프라인이 다르므로 별도 배치

### HOW

#### BatchGroup 구조

```cpp
// ParticleManager.h
struct BatchGroup {
    int materialKey;         // 재질 기준 그룹핑
    int modelIndex;          // 메시 모델 인덱스 (-1: Billboard)
    BlendMode blendMode;     // Additive / AlphaBlend
    std::vector<UINT> emitterIDs;  // 이 배치에 속한 이미터들
    UINT instanceOffset;     // 글로벌 인스턴스 오프셋
};
```

#### 2-Pass Compute

**Pass 1: BatchRenderArgsCS** - 배치별 드로우 인자 계산

```hlsl
// BatchRenderArgsCS.hlsl
[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint globalOffset = 0;

    for (uint batchID = 0; batchID < numBatches; batchID++) {
        BatchDescriptor batch = batchDescriptors[batchID];
        uint totalInstances = 0;

        // 배치 내 각 이미터의 alive 파티클 수 합산
        for (uint i = 0; i < batch.emitterCount; i++) {
            uint eid = batchEmitterList[batch.emitterListOffset + i];
            uint count = uint(float(simulationAliveCount[eid]) * frameConsts[eid].spawnRatio);
            emitterWriteOffsets[batch.emitterListOffset + i] = globalOffset + totalInstances;
            totalInstances += count;
        }

        // Indirect Draw Arguments 설정
        uint argsIdx = batchID * 5;
        batchBillboardArgs[argsIdx + 0] = batch.indexCount;
        batchBillboardArgs[argsIdx + 1] = totalInstances;
        batchBillboardArgs[argsIdx + 2] = batch.startIndexLocation;
        batchBillboardArgs[argsIdx + 3] = batch.baseVertexLocation;
        batchBillboardArgs[argsIdx + 4] = 0;

        globalOffset += totalInstances;
    }
}
```

**Pass 2: BuildAliveIndicesCS** - 이미터별 인덱스를 배치용 평탄 배열로 변환

```hlsl
// BuildAliveIndicesCS.hlsl
[numthreads(256, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint3 threadID : SV_GroupThreadID)
{
    uint flatEmitterIdx = groupID.x;
    if (flatEmitterIdx >= numFlatEmitters) return;

    uint eid = batchEmitterList[flatEmitterIdx];
    uint count = uint(float(simulationAliveCount[eid]) * frameConsts[eid].spawnRatio);
    uint writeBase = emitterWriteOffsets[flatEmitterIdx];
    uint readBase = emitterIDs[eid].readParticleOffset;

    // 각 스레드가 여러 인덱스를 병렬 복사
    for (uint i = threadID.x; i < count; i += 256) {
        uint particleIdx = simulationAliveIndices[readBase + i];
        batchAliveIndices[writeBase + i] = particleIdx;
    }
}
```

---

## 7. 최적화 기법들

### 7-1. GS 제거 → VS Billboard

**WHAT**: Geometry Shader에서 쿼드를 생성하던 방식을 Vertex Shader에서 직접 쿼드를 확장하는 방식으로 변경.

**WHY**: GS는 GPU 파이프라인에서 심각한 병목. 쓰레드 확장이 비효율적이며, output stream 비용이 큼.

```hlsl
// ParticleBillboardVS.hlsl - VS에서 쿼드 확장
// SV_InstanceID로 파티클 식별, 정점 위치로 쿼드 코너 결정
uint batchStartOffset = emitterWriteOffsets[batchEmitterListOffset];
uint globalIdx = aliveIndices[batchStartOffset + input.instanceID];
Particle p = readParticles[globalIdx];

// View Space에서 카메라를 향한 쿼드 생성
float2 quadOffset = input.position.xy;  // [-1,1] 범위의 쿼드 정점
float halfSize = p.size * sizeCurve * 0.5;

float4 viewPos = mul(float4(particleCenter, 1.0), view);
viewPos.xy += quadOffset * halfSize;  // 쿼드 확장
output.posProj = mul(viewPos, proj);
```

### 7-2. CPU Frustum Culling

```cpp
// ParticleManager.cpp
void ParticleManager::GatherVisibleEmitters(
    const DirectX::BoundingFrustum& frustum,
    const Vector3& cameraPos,
    UINT& totalCount, UINT& visibleCount)
{
    for (auto* system : m_activeSystems) {
        totalCount++;
        Vector3 posView = Vector3::Transform(system->GetWorldPosition(), m_view);
        float radius = system->GetBoundingRadius();
        DirectX::BoundingSphere sphere(posView, radius);

        if (frustum.Intersects(sphere)) {
            visibleCount++;
            system->GatherActiveEmitters(m_meshJobs, m_billboardJobs);
        }
    }
}
```

### 7-3. LOD: 거리 기반 SpawnRate 조절

```cpp
// spawnRatio → GPU에서 적용
// 가까울수록 spawnRatio = 1.0, 멀수록 감소
// GPU에서: count = uint(float(simulationAliveCount[eid]) * frameConsts[eid].spawnRatio);
```

### 7-4. Priority-Based Eviction

```cpp
// ParticleManager.cpp - 다중 요소 가중합으로 우선순위 계산
float ParticleManager::CalculatePriority(
    ParticleSystem* system, const Vector3& cameraPos,
    const DirectX::BoundingFrustum& frustum) const
{
    // 1. 거리 (가까울수록 높음) - 제곱 감쇠
    float distance = Vector3::Distance(system->GetWorldPosition(), cameraPos);
    float distanceFactor = 1.0f - std::min(distance / MAX_DISTANCE, 1.0f);
    distanceFactor = distanceFactor * distanceFactor;

    // 2. 가시성 (절두체 안이면 1, 밖이면 0)
    float visibilityFactor = frustum.Intersects(sphere) ? 1.0f : 0.0f;

    // 3. 나이 (새로울수록 높음)
    float ageFactor = std::max(0.0f, 1.0f - (age / AGE_THRESHOLD));

    // 4. 기본 우선순위 (사용자 지정)
    float basePriorityFactor = system->GetBasePriority();

    // 가중합: 거리 40% + 가시성 30% + 나이 20% + 기본 10%
    return (distanceFactor * 0.4f) + (visibilityFactor * 0.3f)
         + (ageFactor * 0.2f) + (basePriorityFactor * 0.1f);
}
```

### 7-5. Global CS Dispatch

**WHAT**: 이전에는 파티클 시스템마다 개별 Compute Shader 디스패치를 수행. 통합 디스패치로 변경하여 오버헤드 감소.

**WHY**: 디스패치 호출 자체의 CPU/GPU 오버헤드 + 셰이더 전환 비용 절감.

---

## 8. LDS Bitonic Sort 3-Phase — 64-bit 계층적 정렬

### WHAT
AlphaBlend 파티클의 올바른 렌더링을 위한 **64-bit 계층적 키(Batch ID + Depth) 기반 back-to-front 정렬**.
groupshared memory(LDS)를 활용한 3단계 최적화 Bitonic Sort로, 다중 Material 배치를 단일 Sort 호출로 처리.

### WHY

**단순 depth 정렬의 한계**
- AlphaBlend 파티클은 뒤에서 앞으로(back-to-front) 순서로 렌더링해야 올바른 블렌딩
- 하지만 서로 다른 Material(Batch)의 파티클이 depth만으로 섞이면, **렌더링 시 Material 전환(셰이더·텍스처 바인딩)이 파티클마다 발생** → Draw Call 폭증
- 해결: **Batch ID를 Major Key, Depth를 Minor Key**로 하여 같은 Material끼리 연속 배치 + 각 Material 내에서 back-to-front 유지

**일반 Bitonic Sort의 대역폭 병목**
- 일반 Bitonic Sort: 매 (k, j) 단계마다 global memory 접근 → Dispatch 횟수 폭증
- LDS 활용: 블록 내부 정렬은 빠른 shared memory에서 수행 → global memory 접근 대폭 감소

### HOW

#### 64-bit Key 구조

```cpp
// BitonicSort.h
struct Element {
    uint32_t key[2]; // x: Batch ID (Major), y: Depth (Minor)
    uint32_t value;  // 파티클 인덱스
};
```

- `key[0]` (= `key.x`): Batch ID — `0xFFFFFFFF - batchIdx`로 반전하여 **작은 batchIdx가 내림차순에서 앞으로**
- `key[1]` (= `key.y`): Depth — `FloatToSortableUint(viewZ)`로 float를 **크기 비교 가능한 uint**로 변환

```hlsl
// GenerateSortKeysCS.hlsl — IEEE 754 float → sortable uint 변환
uint FloatToSortableUint(float f) {
    uint u = asuint(f);
    // 양수: MSB를 1로 세워 음수보다 크게, 음수: 비트 전체 반전으로 크기 순서 보존
    return (u & 0x80000000) ? ~u : (u | 0x80000000);
}
```

#### Key 생성: GenerateSortKeysCS

```hlsl
// GenerateSortKeysCS.hlsl
[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint id = dtID.x;
    SortElement elem;
    if (id < particleCount)
    {
        uint gIdx = baseOffset + id;
        uint particleIdx = batchAliveIndices[gIdx];
        Particle p = readParticles[particleIdx];

        float3 toParticle = p.position - eyeWorld;
        float viewZ = dot(toParticle, cameraForward);

        // 1. 현재 파티클이 속한 Batch Index 찾기
        uint batchIdx = firstBatchIdx;
        for (uint b = firstBatchIdx; b <= lastBatchIdx; ++b) {
            if (gIdx >= batchParams[b].baseOffset &&
                gIdx < batchParams[b].baseOffset + batchParams[b].particleCount) {
                batchIdx = b;
                break;
            }
        }

        // 2. Key 할당 (내림차순 정렬 기준)
        elem.key.x = 0xFFFFFFFF - batchIdx; // BatchID 작을수록 앞으로
        elem.key.y = FloatToSortableUint(viewZ); // viewZ 클수록(멀수록) 앞으로
        elem.value = particleIdx;
    }
    else
    {
        // 패딩 원소: 정렬 시 맨 뒤로 밀리도록 최소값
        elem.key.x = 0;
        elem.key.y = 0;
        elem.value = 0xFFFFFFFF;
    }
    sortBuffer[id] = elem;
}
```

#### 3-Phase 아키텍처

```cpp
// BitonicSort.cpp - SortInternal
void BitonicSort::SortInternal(ID3D11DeviceContext* context,
                                ID3D11UnorderedAccessView* uav, UINT sortSize)
{
    if (sortSize >= BLOCK_SIZE) {
        // Phase 1: LDS 블록 정렬 (1회 디스패치)
        // 각 2048 원소 블록을 groupshared memory에서 완전 정렬
        context->CSSetShader(sortPSOs.bitonicBlockSortCS.computeShader.Get(), 0, 0);
        context->Dispatch(sortSize / BLOCK_SIZE, 1, 1);

        for (uint32_t k = BLOCK_SIZE * 2; k <= sortSize; k *= 2) {
            // Phase 2: Global Memory merge (j >= BLOCK_SIZE인 단계)
            context->CSSetShader(sortPSOs.bitonicSortCS.computeShader.Get(), 0, 0);
            for (uint32_t j = k / 2; j >= BLOCK_SIZE; j /= 2) {
                m_constBuffer.SetCpuData({ k, j });
                m_constBuffer.Upload();
                context->CSSetConstantBuffers(0, 1, m_constBuffer.GetAddressOf());
                context->Dispatch((sortSize + 1023) / 1024, 1, 1);
            }
            // Phase 3: LDS Inner Merge (1회 디스패치)
            m_constBuffer.SetCpuData({ k, 0 });
            m_constBuffer.Upload();
            context->CSSetConstantBuffers(0, 1, m_constBuffer.GetAddressOf());
            context->CSSetShader(sortPSOs.bitonicInnerSortCS.computeShader.Get(), 0, 0);
            context->Dispatch(sortSize / BLOCK_SIZE, 1, 1);
        }
    } else {
        // Fallback: sortSize < 2048이면 LDS 블록을 채울 수 없으므로
        // 기존 BitonicSortCS로 모든 (k, j) 패스를 Global Memory에서 수행
        context->CSSetShader(sortPSOs.bitonicSortCS.computeShader.Get(), 0, 0);
        for (uint32_t k = 2; k <= sortSize; k *= 2) {
            for (uint32_t j = k / 2; j > 0; j /= 2) {
                m_constBuffer.SetCpuData({ k, j });
                m_constBuffer.Upload();
                context->CSSetConstantBuffers(0, 1, m_constBuffer.GetAddressOf());
                context->Dispatch((sortSize + 1023) / 1024, 1, 1);
            }
        }
    }
}
```

#### Phase 1: LDS 블록 정렬 — uint2 Lexicographic 비교

```hlsl
// BitonicBlockSortCS.hlsl (핵심 비교 로직)
// 각 스레드가 2개 원소를 LDS에 로드 후, k=2~BLOCK_SIZE까지 완전 정렬
Element iElem = shared_data[i];
Element lElem = shared_data[l];

// uint2 lexicographic 3단 비교: key.x → key.y → value (tie-break)
bool isLess = (iElem.key.x < lElem.key.x) ||
              (iElem.key.x == lElem.key.x && iElem.key.y < lElem.key.y) ||
              (iElem.key.x == lElem.key.x && iElem.key.y == lElem.key.y
               && iElem.value < lElem.value);
bool isGreater = (iElem.key.x > lElem.key.x) ||
                 (iElem.key.x == lElem.key.x && iElem.key.y > lElem.key.y) ||
                 (iElem.key.x == lElem.key.x && iElem.key.y == lElem.key.y
                  && iElem.value > lElem.value);

if (((globalI & k) == 0) && isLess ||
    ((globalI & k) != 0) && isGreater) {
    shared_data[i] = lElem;   // swap
    shared_data[l] = iElem;
}
```

Phase 2(BitonicSortCS)와 Phase 3(BitonicInnerSortCS)도 동일한 lexicographic 비교를 사용.
Phase 2는 헬퍼 함수로 간결하게 표현:

```hlsl
// BitonicSortCS.hlsl
bool IsLess(uint2 a, uint2 b) {
    return (a.x < b.x) || (a.x == b.x && a.y < b.y);
}
bool IsGreater(uint2 a, uint2 b) {
    return (a.x > b.x) || (a.x == b.x && a.y > b.y);
}
```

#### 전체 Sort Pipeline 흐름

`ParticleManager::SortAlphaBlendEmitters`에서 3단계 파이프라인으로 통합:

```
1. GenerateSortKeysCS  — 파티클 → {BatchID, Depth, ParticleIdx} 키 생성
       ↓ (UAV barrier)
2. BitonicSort.Sort()  — 3-Phase LDS Bitonic Sort 실행
       ↓ (UAV barrier)
3. CopySortedIndicesCS — 정렬된 인덱스를 batchAliveIndices에 기록
```

```hlsl
// CopySortedIndicesCS.hlsl — 정렬 결과 복사
[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint id = dtID.x;
    if (id < particleCount)
    {
        uint sortedParticleIdx = sortedElements[id].value;
        batchAliveIndices[baseOffset + id] = sortedParticleIdx;
    }
}
```

여러 Batch 그룹(FullResBillboard, Billboard)에 대해 각각 독립적으로 파이프라인을 실행하며,
각 단계 사이에 UAV barrier(unbind)를 삽입하여 SRV/UAV 충돌을 방지.

#### 디스패치 횟수 비교 (LDS 최적화 효과)

| 파티클 수 | 기존 (매 패스 Dispatch) | 3-Phase LDS | 개선 |
|----------|----------------------|-------------|------|
| 2,048 | 66회 | 1회 | 66배 |
| 4,096 | 78회 | 3회 | 26배 |
| 8,192 | 91회 | 6회 | 15배 |
| 65,536 | 136회 | 21회 | 6.5배 |

---

## 9. 렌더링 품질

### 9-1. Soft Particles

**WHAT**: 파티클이 불투명 오브젝트와 교차하는 부분에서 하드 엣지 대신 부드러운 알파 페이드.

**WHY**: 파티클이 지형을 뚫고 나가는 시각적 결함 제거. Full/Low Resolution 이중 깊이로 해상도별 최적화.

```hlsl
// ParticlePS.hlsl - 씬 깊이와 파티클 깊이 비교
float softDist = consts[input.emitterSlotID].render.softDistance;
if (softDist > 0) {
    float2 screenUV = input.pos.xy / texSize;
    float sceneDepthNDC = sceneDepthTex.SampleLevel(pointClampSampler, screenUV, 0).r;

    // 씬 깊이와 파티클 깊이의 차이로 알파 페이드
    float softFactor = saturate(
        (LinearizeDepth(sceneDepthNDC) - LinearizeDepth(input.pos.z)) / softDist
    );

    // 거리 기반 페이드아웃 (먼 파티클은 소프트 효과 감소)
    if (softMaxDist > 0.0f) {
        float particleDist = length(input.posWorld.xyz - eyeWorld);
        float distBlend = 1.0f - smoothstep(fadeStart, softMaxDist, particleDist);
        softFactor = lerp(1.0f, softFactor, distBlend);
    }

    finalColor.a *= softFactor;
}
```

### 9-2. Velocity Stretch Billboard

**WHAT**: 파티클의 속도 방향으로 빌보드를 늘려 속도감 표현.

```hlsl
// ParticleBillboardVS.hlsl - View Space에서 속도 방향 투영
float3 viewVel = mul(float4(worldVel, 0.0), view).xyz;
float speed2D = length(viewVel.xy);

if (stretchFactor > 0.0 && speed2D > 0.001)
{
    float2 velDir = viewVel.xy / speed2D;        // 속도 방향
    float2 velPerp = float2(-velDir.y, velDir.x); // 수직 방향

    float stretchAmount = 1.0 + speed2D * stretchFactor;
    viewPos.xy += (quadOffset.x * velDir * stretchAmount   // 속도 방향으로 늘림
                 + quadOffset.y * velPerp) * halfSize;      // 수직은 유지
}
```

### 9-3. Curl Noise

**WHAT**: 비압축성 유체 시뮬레이션의 Curl Noise를 파티클에 적용하여 유기적 움직임 생성.

```hlsl
// ParticleCS.hlsl - 시간에 따라 변화하는 Curl Noise Force
float noiseCurve = SampleCurve(CURVE_NOISE_STRENGTH, ageRatio, curveSlice);
if (noiseCurve > 0.0f) {
    float3 curlForce = CurlNoise3D(p.position * spawn.noiseFrequency + time * spawn.noiseSpeed);
    p.velocity += curlForce * spawn.noiseAmplitude * noiseCurve * dt;
}
```

### 9-4. Curve Data → LUT

**WHAT**: CPU에서 커브를 샘플링하여 1D 텍스처(LUT)로 변환, GPU에서 O(1) 조회.

```cpp
// CurveData.cpp - CPU에서 LUT 베이킹
const std::vector<float>& CurveData::CreateCurveData() {
    UINT res = static_cast<UINT>(m_res);  // 64 ~ 512
    m_bakedData.resize(res);

    for (UINT i = 0; i < res; ++i) {
        float t = (float)i / (res - 1);   // [0.0, 1.0] 범위 샘플링
        float val = Evaluate(t);            // Bezier, Linear, Noise 등
        m_bakedData[i] = std::clamp(val, m_minVal, m_maxVal);
    }
    return m_bakedData;
}
```

```hlsl
// ParticleCS.hlsl - GPU에서 LUT 샘플링
float sizeCurve = SampleCurve(CURVE_SIZE, ageRatio, curveSlice);
float colorCurve = SampleCurve(CURVE_COLOR, ageRatio, curveSlice);
float alphaCurve = SampleCurve(CURVE_ALPHA, ageRatio, curveSlice);
```

### 9-5. Sprite Animation

**WHAT**: 수명/시간 기반으로 스프라이트 시트의 프레임을 전환. 프레임 블렌딩으로 부드러운 전환.

```hlsl
// ParticlePS.hlsl - 프레임 블렌딩
float frameFloat = animLifeRatio * render.frameCount;
uint frame0 = min((uint)floor(frameFloat), render.frameCount - 1);
uint frame1 = min(frame0 + 1, render.frameCount - 1);
float blend = frac(frameFloat);

float2 uvSize = 1.f / render.frameTiles;
float2 uv0 = (uv + float2(frame0 % width, frame0 / width)) * uvSize;
float2 uv1 = (uv + float2(frame1 % width, frame1 / width)) * uvSize;

return lerp(
    albedoTex.Sample(linearClampSampler, uv0),
    albedoTex.Sample(linearClampSampler, uv1),
    blend  // 두 프레임 간 선형 보간
);
```

---

## 10. 전체 렌더링 파이프라인 플로우

```
[CPU] UpdateSpawnRatios
  ↓  거리 기반 LOD SpawnRate 계산
[CPU] GatherVisibleEmitters
  ↓  Frustum Culling → EmitterJob 수집 → Billboard/Mesh 분류 → BatchGroup 생성
[CPU] BuildBatchDescriptors
  ↓  BatchGroup → BatchDescriptor GPU 업로드
[GPU] BatchRenderArgsCS
  ↓  배치별 Indirect Draw Args 계산 + 이미터별 쓰기 오프셋 계산
[GPU] BuildAliveIndicesCS
  ↓  이미터별 alive indices → 배치용 평탄 배열로 변환
[GPU] SortAlphaBlend
  ↓  AlphaBlend 배치만 선택적 정렬 (GenerateKeys → BitonicSort → CopySorted)
[GPU] DrawMeshBatches
  ↓  3D 메시 파티클 렌더링 (DrawIndexedInstancedIndirect)
[GPU] DrawFullResBillboardBatches
  ↓  전체 해상도 빌보드 렌더링
[GPU] DrawLowResBillboardBatches
  ↓  저해상도 빌보드 렌더링 → Bilateral Upsampling으로 합성
```

### 렌더링 순서의 의도
1. **Mesh 먼저**: 불투명 메시 파티클은 깊이 정렬 불필요, 먼저 렌더링하여 깊이 버퍼 확보
2. **Full Res Billboard**: 선명도가 중요한 빌보드 (근거리, 주요 이펙트)
3. **Low Res Billboard**: 원거리/보조 이펙트는 저해상도로 렌더링 → Overdraw 비용 절감
4. **AlphaBlend 정렬**: 반투명 파티클만 선택적으로 back-to-front 정렬하여 올바른 블렌딩

---

## 참조 파일 요약

| 영역 | 파일 | 핵심 역할 |
|------|------|-----------|
| 메모리 풀 | `ParticleMemoryPool.h/cpp` | 블록 할당, 단편화 추적, 디프래그 |
| 매니저 | `ParticleManager.h/cpp` | 전체 파이프라인 오케스트레이션 |
| 모듈 시스템 | `ParticleModule.h`, `ParticleModuleFactory.h/cpp` | Priority 기반 모듈, Factory 패턴 |
| Spawn | `SpawnModule.h/cpp`, `SpawnCS.hlsl` | 6가지 스폰 타입, Rate/Burst 관리 |
| 텍스처 스폰 | `TextureSpawnBake.h/cpp`, `TextureSpawnBakeCS.hlsl` | GPU 베이킹, Seam 감지 |
| 시뮬레이션 | `ParticleCS.hlsl` | 물리 + 컴팩팅 단일 패스 |
| 정렬 | `BitonicSort.h/cpp`, `BitonicBlockSortCS.hlsl`, `BitonicSortCS.hlsl`, `BitonicInnerSortCS.hlsl` | 3-Phase LDS Bitonic Sort |
| 키 생성 | `GenerateSortKeysCS.hlsl` | Planar depth 키 생성 |
| 배치 렌더링 | `BatchRenderArgsCS.hlsl`, `BuildAliveIndicesCS.hlsl` | Indirect Args + 인덱스 평탄화 |
| VS 렌더링 | `ParticleBillboardVS.hlsl`, `ParticleVS.hlsl` | 쿼드 확장, Velocity Stretch |
| PS 렌더링 | `ParticlePS.hlsl` | Soft Particles, Sprite Animation |
| 커브 | `CurveData.h/cpp` | Bezier/Noise 커브, LUT 생성 |
| JSON 로딩 | `ParticleLoader.h/cpp` | 선언적 이펙트 로딩, Hot-Reload |
