cbuffer MatrixesConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
};

cbuffer angleBuf : register(b1)
{
    float angle;
    float angle1;
    float angle2;
    float angle3;
};

float4 main(float4 pos : POSITION) : SV_POSITION
{
    float4 newPos;

newPos.x = pos.x * cos(angle) - pos.y * sin(angle);
newPos.y = pos.x * sin(angle) + pos.y * cos(angle);
newPos.z = pos.z;
newPos.w = pos.w;

newPos = mul(newPos, World); 
newPos = mul(newPos, View); 
newPos = mul(newPos, Projection);
	return newPos;
}