cbuffer MatrixesConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
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
    float projectionCoeff = 0.25 / pos.z; 
    float4x4 mProj = {
        projectionCoeff, 0, 0, 0,
        0, projectionCoeff, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    float4 newPos;

    newPos.x = pos.x * cos(angle) - (pos.z - 0.5) * sin(angle);
    newPos.y = pos.y;
    newPos.z = 0.5 + pos.x * sin(angle) + (pos.z - 0.5) * cos(angle);
    newPos.w = pos.w;

    newPos = mul(newPos, mProj);
    newPos.x = newPos.x * 0.75;

	return newPos;
}