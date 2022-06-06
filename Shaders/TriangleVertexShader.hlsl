cbuffer MatrixesConstantBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
};

cbuffer angleConstantBuffer : register(b1)
{
    float angle;
    float angle1;
    float angle2;
    float angle3;
};

float4 main(float4 pos : POSITION) : SV_POSITION
{
    float4 newPos;

    // поворот вокруг точки 
    newPos.x = pos.x * cos(angle) - (pos.z - 0.5) * sin(angle);
    newPos.y = pos.y;
    newPos.z = 0.5 + pos.x * sin(angle) + (pos.z - 0.5) * cos(angle);
    newPos.w = pos.w;

    newPos = mul(newPos, projection);

	return newPos;
}