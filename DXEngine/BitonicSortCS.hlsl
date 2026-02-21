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
    // (value ����) key�� ����
    uint i = dtID.x;
    
    uint l = i ^ j;
    
    if (l > i)
    {
        Element iElem = arr[i];
        Element lElem = arr[l];
        
        bool isLess = (iElem.key < lElem.key) ||
                      ((iElem.key == lElem.key) && (iElem.value < lElem.value));
        bool isGreater = (iElem.key > lElem.key) ||
                         ((iElem.key == lElem.key) && (iElem.value > lElem.value));

        if (((i & k) == 0) && isLess ||
            ((i & k) != 0) && isGreater)
        {
            arr[i] = lElem;
            arr[l] = iElem;
        }
    }
}