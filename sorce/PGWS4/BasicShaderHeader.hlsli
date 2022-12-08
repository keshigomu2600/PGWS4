struct Output
{
	float4 svpos:SV_POSITION;
	float4 normal:NORMAL0; //�@���x�N�g��
	float4 vnormal : NORMAL1;//�r���[�ϊ���̖@���x�N�g��
	float3 ray : VECTOR;//�x�N�g��
	float2 uv:TEXCOORD;
};

Texture2D<float4>tex : register(t0); //0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4>sph : register(t1); //1�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4>spa : register(t2); //1�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��

SamplerState smp:register(s0); //0�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[

//cbuffer:�萔�o�b�t�@���܂Ƃ߂�L�[���[�h
//b0:CPU�̃X���b�g�ԍ�=�V�F�[�_�̃��W�X�^�ԍ�(b�͒萔���W�X�^��\��)(main�Œ�߂�SRV�̔ԍ��ɑΉ�)
cbuffer cbuff0 : register(b0)
{
	matrix world;
	matrix view;
	matrix proj;
	float3 eye;
};

//�萔�o�b�t�@�[1
//�}�e���A���p
cbuffer Material : register(b1)
{
	float4 diffuse;
	float4 specular;
	float3 ambient;
}