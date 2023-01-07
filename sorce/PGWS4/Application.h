#pragma once
#include<Windows.h>
#include <string>
//�ǂݍ��݊֐�
#include<map>
#include <functional>
//�\�L�ɕK�v
#include<dxgi1_6.h>
//DirectX12�̃I�u�W�F�N�g�Ƃ��Ċe���߂��o���̂ɕK�v
#include<d3d12.h>
//�R�[�h���炵�p
#include<d3dx12.h>
//���C�u�����ǂݍ���
#include<DirectXTex.h>
#include<wrl.h>

class Application
{
private:
	Application();
	~Application();
	Application(const Application&) = delete;
	void operator=(const Application&) = delete;

	Microsoft::WRL::ComPtr<ID3D12Resource> LoadTextureFromFile(std::string& texPath, ID3D12Device* dev);

public:
	//�E�B���h�E����
	WNDCLASSEX _windowClass;
	HWND _hwnd;
	HWND CreateGameWindow(WNDCLASSEX _windowClass, int _window_width, int _window_height);

	//DirectX12�V�X�e��
	Microsoft::WRL::ComPtr<ID3D12Device> _dev = nullptr;
	Microsoft::WRL::ComPtr<IDXGIFactory6> _dxgiFactory = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _cmdAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _cmdList = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> _cmdQueue = nullptr;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> _swapchain = nullptr;
	void InitializeCommand();
	void CreateSwapChain();
	void InitializeBackBuffers();
	void InitializeDepthBuffer();

	//����
	Microsoft::WRL::ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal = 0;

	//��ʃX�N���[��
	const unsigned int _window_width = 1280;
	const unsigned int _window_height = 720;
	Microsoft::WRL::ComPtr<ID3D12Resource> _depthBuffer = nullptr;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _backBuffers; //�o�b�N�o�b�t�@
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _rtvHeaps = nullptr;//�����_�[�^�[�Q�b�g�p�f�X�N���v�^�q�[�v
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _dsvHeap = nullptr;//�[�x�o�b�t�@�r���[�p�f�X�N���v�^�q�[�v
	CD3DX12_VIEWPORT _viewport;//�r���[�|�[�g
	CD3DX12_RECT _scissorrect;//�V�U�[��`

	//�f�t�H���g�e�N�X�`��(���A���A�O���C�X�P�[���O���f�[�V����)
	Microsoft::WRL::ComPtr<ID3D12Resource> _whiteTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _blackTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _gradTex = nullptr;
	void InitialixeTextureLoaderTable(FILE* fp, std::string strModelPath);

	//���\�[�X�Ǘ�
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D12Resource>> _resourceTable;
	using LoadLambda_t = std::function<HRESULT(const std::wstring& path,
		DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map<std::string, LoadLambda_t> _loadLambdaTable;

	//�`���Ԃ̐ݒ�
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelinestate = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> _rootsigunature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _constBuff = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _basicDescHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _materialDescHeap = nullptr;
	void CreateRootSignature(Microsoft::WRL::ComPtr<ID3DBlob> errorBlob);

	//�V�F�[�_�[���ɓn�����߂̊�{�I�Ȋ��f�[�^
	struct SceneData
	{
		DirectX::XMMATRIX world; //���[���h�s��
		DirectX::XMMATRIX view; //�r���[�s��
		DirectX::XMMATRIX proj; //�v���W�F�N�V�����s��
		DirectX::XMFLOAT3 eye; //���_���W
	};
	SceneData* _mapScene; //�}�b�v��������|�C���^�[

	//�V�[�����
	DirectX::XMMATRIX _worldMat;
	DirectX::XMMATRIX _viewMat;
	DirectX::XMMATRIX _projMat;

	//�}�e���A���f�[�^
	struct MaterialForHlsl //�V�F�[�_�[���ɓ�������}�e���A���f�[�^
	{
		DirectX::XMFLOAT3 diffuse; //�f�B�t���[�Y�F
		float alpha; //�f�B�t���[�Ya
		DirectX::XMFLOAT3 specular; //�X�y�L�����F
		float specularity; //�X�y�L�����̋����i��Z�n�j
		DirectX::XMFLOAT3 ambient; //�A���r�G���g�F
	};

	struct AdditionalMaterial //����ȊO�̃}�e���A���f�[�^
	{
		std::string texPath; //�e�N�X�`���t�@�C���p�X
		int toonIdx; //�g�D�[���ԍ�
		bool edgeFlg; //�}�e���A�����Ƃ̗֊s���t���O
	};

	struct Material //�S�̂��܂Ƃ߂�f�[�^
	{
		unsigned int indicesNum; //�C���f�b�N�X��
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};
	std::vector<Material> materials;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _textureResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _sphResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _spaResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _toonResources;

	unsigned int _materialNum; //�}�e���A����
	Microsoft::WRL::ComPtr<ID3D12Resource> _materialBuff = nullptr;

	//���f�����
	Microsoft::WRL::ComPtr<ID3D12Resource> _vertBuff = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _idxBuff = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};
	D3D12_INDEX_BUFFER_VIEW _ibView = {};

	//�V���O���g���C���X�^���X
	static Application& Instance();

	//������
	bool Init();

	//���[�v�N��
	void Run();

	//�㏈��
	void Terminate();
};
