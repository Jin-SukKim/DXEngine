// BitonicBlockSortCS.hlsl
// Phase 1: LDS-based block sort
// Each threadgroup fully sorts BLOCK_SIZE elements using shared memory

#define BLOCK_SIZE 2048
#define THREADS 1024

struct Element
{
    uint2 key;
    uint value;
};

RWStructuredBuffer<Element> arr : register(u0);

groupshared Element shared_data[BLOCK_SIZE];

[numthreads(THREADS, 1, 1)]
void main(uint3 gID : SV_GroupID, uint3 gtID : SV_GroupThreadID)
{
    uint localIdx = gtID.x;
    uint blockOffset = gID.x * BLOCK_SIZE;

    // Each thread loads 2 elements into shared memory
    uint idx0 = localIdx;
    uint idx1 = localIdx + THREADS;
    shared_data[idx0] = arr[blockOffset + idx0];
    shared_data[idx1] = arr[blockOffset + idx1];

    // LDS(groupshared) ͸ (Write) ,  ͸ ٸ  б(Read)   
    GroupMemoryBarrierWithGroupSync();

    // Full bitonic sort within the block (k=2..BLOCK_SIZE)
    for (uint k = 2; k <= BLOCK_SIZE; k <<= 1)
    {
        for (uint j = k >> 1; j > 0; j >>= 1)
        {
            [unroll]
            for (uint t = 0; t < 2; t++)
            {
                uint i = localIdx + t * THREADS;
                uint l = i ^ j;

                if (l > i)
                {
                    uint globalI = blockOffset + i;

                    Element iElem = shared_data[i];
                    Element lElem = shared_data[l];

                    bool isLess = (iElem.key.x < lElem.key.x) ||
                                  (iElem.key.x == lElem.key.x && iElem.key.y < lElem.key.y) ||
                                  (iElem.key.x == lElem.key.x && iElem.key.y == lElem.key.y && iElem.value < lElem.value);
                    bool isGreater = (iElem.key.x > lElem.key.x) ||
                                     (iElem.key.x == lElem.key.x && iElem.key.y > lElem.key.y) ||
                                     (iElem.key.x == lElem.key.x && iElem.key.y == lElem.key.y && iElem.value > lElem.value);

                    if (((globalI & k) == 0) && isLess ||
                        ((globalI & k) != 0) && isGreater)
                    {
                        shared_data[i] = lElem;
                        shared_data[l] = iElem;
                    }
                }
            }

            GroupMemoryBarrierWithGroupSync();
        }
    }

    // Write sorted block back to global memory
    arr[blockOffset + idx0] = shared_data[idx0];
    arr[blockOffset + idx1] = shared_data[idx1];
}
