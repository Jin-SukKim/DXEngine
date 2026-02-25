struct Element {
    uint2 key;
    uint value;
};

// Batch ID 별로 먼저 정렬 후 depth를 비교해서 정렬
bool IsLess(uint2 a, uint2 b) {
    return (a.x < b.x) || (a.x == b.x && a.y < b.y);
}
bool IsGreater(uint2 a, uint2 b) {
    return (a.x > b.x) || (a.x == b.x && a.y > b.y);
}

cbuffer MyBuffer : register(b0)
{
    // https://en.wikipedia.org/wiki/Bitonic_sorter Example Code
    uint k;
    uint j;
}

RWStructuredBuffer<Element> arr : register(u0);

[numthreads(1024, 1, 1)]
void main(int3 gID : SV_GroupID, int3 gtID : SV_GroupThreadID,
          uint3 dtID : SV_DispatchThreadID)
{
    // (value ����) key�� ����
    uint i = dtID.x;
    
    uint l = i ^ j;
    
    if (l > i)
    {
        Element iElem = arr[i];
        Element lElem = arr[l];
        
        bool isLess = IsLess(iElem.key, lElem.key) ||
            (iElem.key.x == lElem.key.x && iElem.key.y == lElem.key.y && iElem.value < lElem.value);
        bool isGreater = IsGreater(iElem.key, lElem.key) ||
            (iElem.key.x == lElem.key.x && iElem.key.y == lElem.key.y && iElem.value > lElem.value);

        if (((i & k) == 0) && isLess ||
            ((i & k) != 0) && isGreater)
        {
            arr[i] = lElem;
            arr[l] = iElem;
        }
    }
}