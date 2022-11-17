#include"BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	//N���@��
	//���C�g�̈ʒu
	float3 light = normalize(float3(1,-1,1));
	//�P�x(Id)�����߂��dot(����)
	float brightness = dot(-light, input.normal);

	return float4(brightness, brightness, brightness, 1) * diffuse;
}