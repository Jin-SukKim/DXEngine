// BitonicInnerSortCS.hlsl
// Phase 3: LDS-based inner merge for j < BLOCK_SIZE
// Each threadgroup loads BLOCK_SIZE elements and performs all inner merge passes

#define BLOCK_SIZE 2048
#define THREADS 1024

struct Element
{
    uint2 key;
    uint value;
};

cbuffer MyBuffer : register(b0)
{
    uint k;
    uint j; // unused (inner sort iterates j internally)
};

RWStructuredBuffer<Element> arr : register(u0);

groupshared Element shared_data[BLOCK_SIZE];

[numthreads(THREADS, 1, 1)]
void main(uint3 gID : SV_GroupID, uint3 gtID : SV_GroupThreadID)
{
    uint localIdx = gtID.x;
    uint blockOffset = gID.x * BLOCK_SIZE;

    // Load into shared memory
    uint idx0 = localIdx;
    uint idx1 = localIdx + THREADS;
    shared_data[idx0] = arr[blockOffset + idx0];
    shared_data[idx1] = arr[blockOffset + idx1];

    GroupMemoryBarrierWithGroupSync();

    // Inner merge passes: j = BLOCK_SIZE/2 down to 1
    for (uint jj = BLOCK_SIZE >> 1; jj > 0; jj >>= 1)
    {
        [unroll]
        for (uint t = 0; t < 2; t++)
        {
            uint i = localIdx + t * THREADS;
            uint l = i ^ jj;

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

    // Write back to global memory
    arr[blockOffset + idx0] = shared_data[idx0];
    arr[blockOffset + idx1] = shared_data[idx1];
}
