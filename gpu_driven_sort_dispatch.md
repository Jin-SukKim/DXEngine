# GPU-Driven Sort Dispatch 최적화

> DirectX 11 파티클 엔진에서 CPU-GPU 동기화를 제거하고, GPU가 자율적으로 정렬을 수행하도록 개선한 최적화 문서

---

# Part 1: 쉬운 설명

## 기존 문제: "매 프레임 GPU한테 물어보고 다시 알려주는" 비효율

기존 정렬 파이프라인은 이런 흐름이었습니다:

```
매 프레임:
  GPU: "파티클 정보 여기 있어요" (batchSortParams)
  CPU: "잠깐, 그거 내려줘" ← batchSortParams.Download()  GPU 멈춤!
  CPU: (파티클 몇 개인지 계산, sortSize 결정)
  CPU: "좋아, 이 설정으로 정렬해" → 매 step마다 CB Map/Unmap + Dispatch
```

**두 가지 병목이 있었습니다:**

1. **GPU→CPU Readback (`Download()`)**: GPU가 작업하다가 CPU가 데이터 달라고 하면, GPU가 현재 작업 다 끝날 때까지 **멈추고 기다려야** 합니다. ~0.3-0.5ms 스톨.

2. **매 Step CB 업데이트**: Bitonic Sort는 수십 번의 step을 실행하는데, 매번 `Map → 데이터 복사 → Unmap → Bind`를 반복. step 수 × ~50μs 오버헤드.

---

## 해결: "GPU가 스스로 다 결정하게 하자"

```
초기화 (1회만):
  CPU: 모든 가능한 sort step의 CB를 미리 만들어서 Upload해둠 ← InitIndirect()

매 프레임:
  GPU: PrepareSortDispatchCS 실행 (1,1,1)
    → batchSortParams 직접 읽기
    → "파티클 총 몇 개? → sortSize 계산"
    → "각 step별로 dispatch 몇 번 해야 해?" → IndirectArgs 버퍼에 기록
  GPU: DispatchIndirect × N (GenKeys, Sort steps, CopyBack)
    → GPU가 자기가 써둔 IndirectArgs를 읽어서 자기가 실행
```

**CPU는 더 이상 개입하지 않습니다!**

---

## 핵심 아이디어 3가지

### 1. Pre-allocated Constant Buffers (초기화 시 1회)

```cpp
// BitonicSort::InitIndirect()
// maxSortSize=131072면 28개의 sort step이 나옴
for (각 step) {
    CB 생성 → {k, j} 값 설정 → Upload()  // 한 번만!
}
```

런타임에는 이미 만들어둔 CB를 **바인딩만** 합니다. Map/Unmap 제로.

### 2. PrepareSortDispatchCS (GPU가 스스로 계획)

```hlsl
// GPU 셰이더가 직접:
// 1) 파티클 수 집계
// 2) sortSize 계산 (2의 거듭제곱으로 올림)
// 3) 각 sort step에 dispatch할 thread group 수를 IndirectArgs 버퍼에 기록
```

이게 핵심입니다. 기존에 CPU가 하던 "몇 개 정렬할지 결정" 작업을 **GPU 셰이더 1개**로 대체.

### 3. minSortSize 기반 자동 스킵

각 sort step에는 "이 step이 필요한 최소 sortSize"가 있습니다:

```
예: 파티클 3000개 → sortSize = 4096
  - Phase 1 (minSortSize=2048): 실행 O
  - k=4096 Phase 2/3 (minSortSize=4096): 실행 O
  - k=8192 Phase 2/3 (minSortSize=8192): groups=0 → 자동 스킵 O
```

`DispatchIndirect(0, 1, 1)`은 GPU에서 **아무것도 안 하는** 호출이므로, 분기문 없이 자연스럽게 불필요한 단계를 건너뜁니다.

---

## 최종 결과

| 항목 | 기존 | 개선 후 |
|------|------|---------|
| GPU→CPU Readback | 매 프레임 Download() | **완전 제거** |
| CB Map/Unmap | step 수 × 매 프레임 | **0** (pre-allocated) |
| CPU 개입 | 파티클 수 계산 + dispatch 제어 | PrepareSortConsts CB 1개만 설정 |
| Sort step 스킵 | CPU가 if문으로 판단 | GPU가 groups=0으로 자동 no-op |

---

간단히 비유하면: 기존에는 **감독(CPU)이 매번 선수(GPU)한테 와서 다음 플레이를 알려주는** 방식이었는데, 이제는 **경기 시작 전에 작전판을 다 짜놓고 선수가 상황 보고 알아서 플레이하는** 방식으로 바뀐 겁니다.

---

# Part 2: 코드 중심 설명

## 코드 흐름: 기존 vs 개선

### Before: `BitonicSort::SortInternal` (기존 방식)

`BitonicSort.cpp:28-68`에서 **매 프레임, 매 step마다** CB를 갱신합니다:

```cpp
void BitonicSort::SortInternal(ID3D11DeviceContext* context,
                                ID3D11UnorderedAccessView* uav, UINT sortSize)
{
    auto& sortPSOs = RenderBase::computeCommon.sort;
    context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

    if (sortSize >= BLOCK_SIZE) {
        // Phase 1: LDS block sort (1 dispatch)
        context->CSSetShader(sortPSOs.bitonicBlockSortCS.computeShader.Get(), 0, 0);
        context->Dispatch(sortSize / BLOCK_SIZE, 1, 1);

        // Phase 2+3: Global merge
        for (uint32_t k = BLOCK_SIZE * 2; k <= sortSize; k *= 2) {
            // Phase 2: outer j passes (global memory)
            context->CSSetShader(sortPSOs.bitonicSortCS.computeShader.Get(), 0, 0);
            for (uint32_t j = k / 2; j >= BLOCK_SIZE; j /= 2) {
                m_constBuffer.SetCpuData({ k, j });   // ← CPU에서 값 설정
                m_constBuffer.Upload();                 // ← Map → memcpy → Unmap (매번!)
                context->CSSetConstantBuffers(0, 1, m_constBuffer.GetAddressOf());
                context->Dispatch((sortSize + 1023) / 1024, 1, 1);
            }
            // Phase 3: inner j passes (LDS, 1 dispatch)
            m_constBuffer.SetCpuData({ k, 0 });
            m_constBuffer.Upload();                     // ← 또 Map/Unmap
            context->CSSetConstantBuffers(0, 1, m_constBuffer.GetAddressOf());
            context->CSSetShader(sortPSOs.bitonicInnerSortCS.computeShader.Get(), 0, 0);
            context->Dispatch(sortSize / BLOCK_SIZE, 1, 1);
        }
    }
}
```

그리고 이걸 호출하기 전에 **CPU에서 sortSize를 알아야** 했으므로:

```cpp
// 기존 ParticleManager (개선 전, 삭제된 코드)
batchSortParams.Download(context);  // GPU→CPU readback stall!
// CPU에서 totalParticleCount 계산...
UINT sortSize = nextPowerOf2(totalParticleCount);
bitonicSort.Sort(context, sortBufferUAV, sortSize);  // → SortInternal 호출
```

---

### After: 3단계로 분리

#### Step 1: 초기화 — `BitonicSort::InitIndirect` (1회만 실행)

`BitonicSort.cpp:103-142`:

```cpp
void BitonicSort::InitIndirect(UINT maxSortSize)  // maxSortSize = 131072
{
    m_sortStepTable.clear();
    m_stepConstBuffers.clear();

    // 1) 모든 가능한 step을 테이블로 생성
    // Step 0: Phase 1 (BlockSort) — minSortSize = BLOCK_SIZE(2048)
    m_sortStepTable.push_back({ 0, 0, 1, BLOCK_SIZE });

    // Phase 2+3 steps for k = BLOCK_SIZE*2 up to maxSortSize
    for (uint32_t k = BLOCK_SIZE * 2; k <= maxSortSize; k *= 2) {
        for (uint32_t j = k / 2; j >= BLOCK_SIZE; j /= 2)
            m_sortStepTable.push_back({ k, j, 2, k });    // Phase 2, minSortSize = k
        m_sortStepTable.push_back({ k, 0, 3, k });        // Phase 3, minSortSize = k
    }
    // → 총 28개 step 생성됨

    // 2) 각 step마다 CB를 하나씩 만들어서 미리 Upload
    m_stepConstBuffers.resize(m_sortStepTable.size());  // 28개
    for (size_t i = 0; i < m_sortStepTable.size(); ++i) {
        m_stepConstBuffers[i].Initialize();
        m_stepConstBuffers[i].SetCpuData({ m_sortStepTable[i].k, m_sortStepTable[i].j });
        m_stepConstBuffers[i].Upload();  // ★ 여기서 1회만 Upload, 이후 절대 안 건드림
    }

    // 3) GPU용 step table 생성 (PrepareSortDispatchCS가 읽을 SRV)
    auto device = GET_SINGLE(RenderBase)->GetDevice().Get();
    std::vector<SortStepGPU> gpuTable(m_sortStepTable.size());
    for (size_t i = 0; i < m_sortStepTable.size(); ++i) {
        gpuTable[i].k = m_sortStepTable[i].k;
        gpuTable[i].j = m_sortStepTable[i].j;
        gpuTable[i].minSortSize = m_sortStepTable[i].minSortSize;
        gpuTable[i].phase = m_sortStepTable[i].phase;
    }
    m_sortStepTableBuffer.SetData(std::move(gpuTable));
    m_sortStepTableBuffer.Initialize(device);
    m_sortStepTableBuffer.Upload(GET_SINGLE(RenderBase)->GetContext().Get());
}
```

**핵심**: `m_stepConstBuffers[28]` — 28개의 CB가 초기화 시 한 번만 Upload됨.

---

#### Step 2: 매 프레임 — `PrepareSortDispatchCS.hlsl` (GPU가 계획)

`PrepareSortDispatchCS.hlsl:36-132`:

```hlsl
// 입력: batchSortParams (t0), sortStepTable (t1)
// 출력: gpuSortParams (u0), dispatchArgs (u1)

[numthreads(1, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    // 1) 파티클 수 집계 (기존엔 CPU가 Download 받아서 했던 일)
    uint totalParticleCount = 0;
    uint baseOffset = 0xFFFFFFFF;
    for (uint b = firstBatchIdx; b <= lastBatchIdx; ++b) {
        BatchSortParam param = batchSortParams[b];  // GPU 메모리에서 직접 읽기!
        if (param.particleCount > 0) {
            if (baseOffset == 0xFFFFFFFF) baseOffset = param.baseOffset;
            totalParticleCount += param.particleCount;
        }
    }

    // 2) sortSize 계산
    uint sortSize = max(NextPowerOf2(totalParticleCount), BLOCK_SIZE);

    // 3) GenKeys dispatch args (offset 0)
    dispatchArgs[0] = (sortSize + 1023) / 1024;  // thread groups
    dispatchArgs[1] = 1;
    dispatchArgs[2] = 1;

    // 4) 각 sort step의 dispatch args (offset (i+1)*3)
    for (uint i = 0; i < numSortSteps; ++i) {
        SortStepGPU step = sortStepTable[i];
        uint offset = (i + 1) * 3;

        if (sortSize >= step.minSortSize) {
            // 이 step 실행 필요 → group 수 계산
            uint groups = (step.phase == 1 || step.phase == 3)
                ? sortSize / BLOCK_SIZE
                : (sortSize + 1023) / 1024;
            dispatchArgs[offset]     = groups;
            dispatchArgs[offset + 1] = 1;
            dispatchArgs[offset + 2] = 1;
        } else {
            dispatchArgs[offset]     = 0;  // ★ no-op! DispatchIndirect가 아무것도 안 함
            dispatchArgs[offset + 1] = 1;
            dispatchArgs[offset + 2] = 1;
        }
    }

    // 5) CopyBack dispatch args (offset (numSortSteps+1)*3)
    uint copyOffset = (numSortSteps + 1) * 3;
    dispatchArgs[copyOffset] = (totalParticleCount + 1023) / 1024;

    // 6) GPUSortParams 기록 (GenKeys/CopyBack 셰이더가 이걸 읽음)
    GPUSortParams sp;
    sp.sortBaseOffset = baseOffset;
    sp.sortParticleCount = totalParticleCount;
    sp.sortSize = sortSize;
    // ...
    gpuSortParams[sortParamsSlot] = sp;
}
```

**dispatchArgs 버퍼 레이아웃:**
```
[0-2]   GenKeys args         ← offset 0 bytes
[3-5]   Sort step 0 args     ← offset 12 bytes
[6-8]   Sort step 1 args     ← offset 24 bytes
...
[84-86] Sort step 27 args    ← offset 252 bytes
[87-89] CopyBack args        ← offset 264 bytes  ((28+1)*3 * 4 bytes = 348 from start)
```

---

#### Step 3: 매 프레임 — `BitonicSort::SortIndirect` (GPU가 실행)

`BitonicSort.cpp:144-180`:

```cpp
void BitonicSort::SortIndirect(ID3D11DeviceContext* context,
                                ID3D11UnorderedAccessView* sortBufferUAV,
                                ID3D11Buffer* indirectArgsBuffer,
                                UINT argsBaseByteOffset)  // = 12 (GenKeys 다음부터)
{
    auto& sortPSOs = RenderBase::computeCommon.sort;
    context->CSSetUnorderedAccessViews(0, 1, &sortBufferUAV, nullptr);

    for (UINT i = 0; i < static_cast<UINT>(m_sortStepTable.size()); ++i) {  // 28 iterations
        const auto& step = m_sortStepTable[i];

        // ★ Pre-allocated CB — Map/Unmap 없이 바인딩만!
        context->CSSetConstantBuffers(0, 1, m_stepConstBuffers[i].GetAddressOf());

        // Phase에 따라 셰이더 선택
        switch (step.phase) {
        case 1: context->CSSetShader(sortPSOs.bitonicBlockSortCS.computeShader.Get(), 0, 0);  break;
        case 2: context->CSSetShader(sortPSOs.bitonicSortCS.computeShader.Get(), 0, 0);       break;
        case 3: context->CSSetShader(sortPSOs.bitonicInnerSortCS.computeShader.Get(), 0, 0);  break;
        }

        // ★ DispatchIndirect — GPU가 써놓은 args를 읽어서 실행
        // groups=0이면 GPU가 자동으로 아무것도 안 함
        UINT byteOffset = argsBaseByteOffset + i * 12;  // 12 = sizeof(uint) * 3
        context->DispatchIndirect(indirectArgsBuffer, byteOffset);
    }
}
```

**기존 `SortInternal`과 비교:**

| | SortInternal (기존) | SortIndirect (개선) |
|---|---|---|
| CB | `SetCpuData → Upload()` 매 step | `CSSetConstantBuffers()` 바인딩만 |
| Dispatch | `Dispatch(groups, 1, 1)` — CPU가 groups 계산 | `DispatchIndirect(buffer, offset)` — GPU가 결정 |
| 분기 | CPU `if/for`로 step 스킵 | `groups=0`이면 자동 no-op |

---

### 전체 호출 흐름: `ParticleManager::SortAlphaBlendEmitters`

`ParticleManager.cpp:387-537`:

```cpp
void ParticleManager::SortAlphaBlendEmitters()
{
    // sortBatchGroupIndirect 람다를 2번 호출:
    //   slot 0: fullResBillboard 배치
    //   slot 1: lowResBillboard 배치

    auto sortBatchGroupIndirect = [&](batches, descStartIdx, sortParamsSlot) {

        // ── 1. CPU 최소 작업: AlphaBlend batch 범위만 찾기 ──
        UINT firstBatchIdx, lastBatchIdx;  // blend mode는 CPU 데이터

        // ── 2. PrepareSortDispatchCS (GPU가 계획) ──
        m_prepareSortCB.SetCpuData({ firstBatchIdx, lastBatchIdx, numSortSteps, sortParamsSlot, ... });
        m_prepareSortCB.Upload();  // 이건 작은 CB 1개만
        context->CSSetShader(prepareSortDispatchCS, 0, 0);
        context->Dispatch(1, 1, 1);  // GPU 스레드 1개가 계획 수립
        // → dispatchArgs 버퍼에 모든 step의 args가 기록됨
        // → gpuSortParams[slot]에 sortSize, baseOffset 등 기록됨

        // ── 3. DispatchIndirect: GenerateSortKeysCS ──
        context->CSSetShader(generateSortKeysCS, 0, 0);
        context->DispatchIndirect(argsBuffer, 0);  // offset 0 = GenKeys args

        // ── 4. DispatchIndirect x 28: BitonicSort steps ──
        bitonicSort.SortIndirect(context, sortUAV, argsBuffer, 12);
        // → 내부에서 28번 DispatchIndirect, pre-allocated CB 바인딩

        // ── 5. DispatchIndirect: CopySortedIndicesCS ──
        context->CSSetShader(copySortedIndicesCS, 0, 0);
        UINT copyBackOffset = (1 + numSortSteps) * 12;
        context->DispatchIndirect(argsBuffer, copyBackOffset);
    };

    sortBatchGroupIndirect(m_fullResBillboardBatches, fullResStart, 0);  // slot 0
    sortBatchGroupIndirect(m_billboardBatches, billboardStart, 1);       // slot 1
}
```

---

### GenKeys/CopyBack이 GPU 파라미터를 읽는 방법

기존에는 CPU가 CB로 `sortSize`, `baseOffset` 등을 전달했지만, 이제 `gpuSortParams` StructuredBuffer에서 직접 읽습니다:

```hlsl
// GenerateSortKeysCS.hlsl — 개선 후
StructuredBuffer<GPUSortParams> gpuSortParams : register(t2);

cbuffer SortGroupConsts : register(b5) {
    uint sortParamsSlot;  // 0 또는 1 (fullRes / lowRes)
};

[numthreads(1024, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID) {
    GPUSortParams sp = gpuSortParams[sortParamsSlot];  // ★ GPU에서 직접 읽기
    // sp.sortBaseOffset, sp.sortParticleCount, sp.sortSize 등 사용
}
```

```hlsl
// CopySortedIndicesCS.hlsl — 동일한 패턴
GPUSortParams sp = gpuSortParams[sortParamsSlot];
if (dtID.x < sp.sortParticleCount) {
    batchAliveIndices[sp.sortBaseOffset + dtID.x] = sortedElements[dtID.x].value;
}
```

---

### 관련 구조체 (`Particle.h`)

```cpp
// GPU-side sort parameters (replaces CPU cbuffer b5 for sort)
struct GPUSortParams {
    UINT sortBaseOffset;
    UINT sortParticleCount;
    UINT firstBatchIdx;
    UINT lastBatchIdx;
    Vector3 cameraForward;
    UINT sortSize;
};

// PrepareSortDispatchCS constant buffer
struct PrepareSortConsts {
    UINT firstBatchIdx;
    UINT lastBatchIdx;
    UINT numSortSteps;
    UINT sortParamsSlot;
    Vector3 cameraForward;
    float padding;
};

// GenKeys/CopyBack constant buffer (indexes into gpuSortParams[slot])
struct SortGroupConsts {
    UINT sortParamsSlot;
    Vector3 padding;
};
```

```cpp
// BitonicSort.h
struct SortStepGPU {
    uint32_t k;
    uint32_t j;
    uint32_t minSortSize;
    uint32_t phase; // 1=BlockSort, 2=GlobalSort, 3=InnerSort
};

struct SortStepInfo {
    uint32_t k;
    uint32_t j;
    uint32_t phase; // 1, 2, or 3
    uint32_t minSortSize;
};
```

---

### 데이터 흐름 요약

```
CPU (1회 초기화)
  InitIndirect(131072)
    → m_stepConstBuffers[28] Upload
    → m_sortStepTableBuffer Upload (GPU SRV)

CPU (매 프레임, 최소 작업)
  m_prepareSortCB Upload (firstBatchIdx, lastBatchIdx, numSortSteps, slot)

GPU (매 프레임, 자율 실행)
  PrepareSortDispatchCS
    reads:  batchSortParams[t0], sortStepTable[t1]
    writes: gpuSortParams[u0], dispatchArgs[u1]
       ↓ UAV barrier
  DispatchIndirect(GenKeys)       ← reads dispatchArgs[0]
       ↓ UAV barrier
  DispatchIndirect(Step 0~27)     ← reads dispatchArgs[3~86], uses pre-allocated CBs
       ↓ UAV barrier
  DispatchIndirect(CopyBack)      ← reads dispatchArgs[87]
```

---

## 참조 소스 파일

| 파일 | 역할 |
|------|------|
| `DXEngine/BitonicSort.h` | SortStepGPU, SortStepInfo 구조체, InitIndirect/SortIndirect 선언 |
| `DXEngine/BitonicSort.cpp` | InitIndirect(), SortIndirect(), SortInternal() 구현 |
| `DXEngine/PrepareSortDispatchCS.hlsl` | GPU-side sort parameter + dispatch args 생성 |
| `DXEngine/ParticleManager.cpp` | SortAlphaBlendEmitters() — 전체 정렬 파이프라인 호출 |
| `DXEngine/Particle.h` | GPUSortParams, PrepareSortConsts, SortGroupConsts 구조체 |
| `DXEngine/ParticleCommon.hlsli` | HLSL 공통 구조체 (GPUSortParams, SortStepGPU) |
| `DXEngine/GenerateSortKeysCS.hlsl` | 정렬 키 생성 (StructuredBuffer\<GPUSortParams\> 사용) |
| `DXEngine/CopySortedIndicesCS.hlsl` | 정렬 결과 복사 (StructuredBuffer\<GPUSortParams\> 사용) |
