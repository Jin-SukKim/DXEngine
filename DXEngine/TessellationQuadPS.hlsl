struct DomainOut {
    float4 pos : SV_Position;
};

float4 main(DomainOut pin) : SV_TARGET {
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}