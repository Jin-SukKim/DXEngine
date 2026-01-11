struct Element
{
    float key;
    uint value;
};

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
    // (value 말고) key로 정렬
    uint i = dtID.x;
    
    uint l = i ^ j;
    
    if (l > i)
    {
        Element iElem = arr[i];
        Element lElem = arr[l];
        
        if (((i & k) == 0) && (iElem.key < lElem.key) ||
            ((i & k) != 0) && (iElem.key > lElem.key))
        {
            arr[i] = lElem;
            arr[l] = iElem;
        }
    }
}