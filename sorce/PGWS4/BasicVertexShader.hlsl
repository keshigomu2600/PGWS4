#include"BasicShaderHeader.hlsli"

Output BasicVS(float4 pos : POSITION,float2 uv:TEXCOORD)
{
	Output output; //�s�N�Z���V�F�[�_�[�ɓn���l
	output.svpos = mul(mat,pos); //mul�֐����g����matrix�ŕϊ����ꂽ���̂��\�������
	output.uv = uv;

	return output;
}