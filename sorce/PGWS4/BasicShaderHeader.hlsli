struct Output
{
	float4 svpos:SV_POSITION;
	float2 uv:TEXCOORD;
};

Texture2D<float4>tex : register(t0); //0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
SamplerState smp:register(s0); //0�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[

//cbuffer:�萔�o�b�t�@���܂Ƃ߂�L�[���[�h
//b0:CPU�̃X���b�g�ԍ�=�V�F�[�_�̃��W�X�^�ԍ�(b�͒萔���W�X�^��\��)(main�Œ�߂�SRV�̔ԍ��ɑΉ�)
cbuffer cbuff0 : register(b0)
{
	matrix mat;
};