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
    
    // поворот вокруг точки (0; 0; 0.5)
    newPos.x = pos.x * cos(angle) - (pos.z - 0.5) * sin(angle);
    newPos.y = pos.y;
    newPos.z = 0.5 + pos.x * sin(angle) + (pos.z - 0.5) * cos(angle);
    newPos.w = pos.w;

    newPos = mul(view, newPos);
    newPos = mul(newPos, projection);

    VS_OUTPUT output;
    output.outPos = newPos;
    output.outColor = color;
    return output;
}