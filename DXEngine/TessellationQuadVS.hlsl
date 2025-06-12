struct VertexIn {
    float3 pos : POSITION;
};

struct VertexOut {
    float4 pos : POSITION;
};

VertexOut main(VertexIn vin) {
    VertexOut vout;
    
    // pos를 받아서 pos를 반환하는 가장 단순한 형태
    vout.pos = float4(vin.pos, 1.0);
    
    return vout;
}