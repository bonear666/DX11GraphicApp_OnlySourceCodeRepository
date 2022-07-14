cbuffer MatrixesConstantBuffer : register(b0)
{
    float4x4 world;
    float4x4 view;
    float4x4 projection;
};

cbuffer angleConstantBuffer : register(b1)
{
    float angle;
    float angle1;
    float angle2;
    float angle3;
};

struct VS_OUTPUT {
    float4 outPos : SV_POSITION;
    float4 outColor : COLOR;
};

VS_OUTPUT main(float4 pos : POSITION, float4 color : COLOR) 
{
    
    float4 newPos;
    // поворот вокруг точки (0; 0; 5) и вектора (0; 1; 0)
    newPos.x = pos.x * cos(angle) - (pos.z - 5) * sin(angle);
    newPos.y = pos.y;
    newPos.z = 5 + pos.x * sin(angle) + (pos.z - 5) * cos(angle);
    newPos.w = pos.w;

    newPos = mul(newPos, world);
    newPos = mul(newPos, view);

    //float coeff0 = 0.25f * 0.579f;
    //float coeff = newPos.z * coeff0 + newPos.w;

    newPos = mul(newPos, projection);

    //newPos.x = newPos.x * newPos.w / coeff;
    //newPos.y = newPos.y * newPos.w / coeff;

    VS_OUTPUT output;
    output.outPos = newPos;
    output.outColor = color;
    return output;
}