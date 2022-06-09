struct VS_OUTPUT {
    float4 outPos : SV_POSITION;
    float4 outColor : COLOR;
};

float4 main(VS_OUTPUT psInput) : SV_TARGET
{
	return psInput.outColor;
}