#include "Application.h"
#include <tchar.h>
#include <assert.h>
#include <vector>
#include <exception>
#include <d3dcompiler.h>

#pragma comment (lib, "DirectXTex.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

using namespace DirectX;
using namespace Microsoft::WRL;

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

//@brief �R���\�[����ʂɃt�H�[�}�b�g�t���������\��
//@param format �t�H�[�}�b�g(%d�A%f��)
//@param �ϒ�����
//@remarks �f�o�b�O�p�֐��i�f�o�b�O���ɂ����\������Ȃ��j
void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);
	va_end(valist);
#endif
}

//OS�̃C�x���g�ɉ������鏈��(�����Ȃ��Ƃ����Ȃ�)
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//�E�B���h�E���j�����ꂽ��Ă΂��
	if (msg == WM_DESTROY)
	{
		//OS�ɑ΂��ďI�������鎖��`����
		PostQuitMessage(0);
		return 0;
	}
	//����̏������g��
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

//���f���ƃp�X�ƃe�N�X�`���̃p�X���獇���p�X�𓾂�
//@param modelPath �A�v���P�[�V�������猩��pmd���f���̃p�X
//@param texPath PMD���f�����猩���e�N�X�`���̃p�X
//@return �A�v���P�[�V�������猩���e�N�X�`���̃p�X
std::string GetTexturePathFromModelAndTexPath(
	const std::string& modelPath,
	const char* texPath)
{
	int pathIndex1 = static_cast<int>(modelPath.rfind('/'));
	int pathIndex2 = static_cast<int>(modelPath.rfind('\\'));
	int pathIndex = max(pathIndex1, pathIndex2);

	std::string folderPath = modelPath.substr(0, pathIndex + 1);
	return folderPath + texPath;
}

//�t�@�C��������g���q�擾
std::string GetExtension(const std::string& path)
{
	int idx = static_cast<int>(path.rfind('.'));
	return path.substr(idx + 1, path.length() - idx - 1);
}

//�e�N�X�`���̃p�X���Z�p���[�^�[�����ŕ�������
std::pair<std::string, std::string>SplitFileName(
	const std::string& path, const char splitter = '*')
{
	//[*]���̈ʒu�𒲂ׂ�
	int idx = static_cast<int>(path.find(splitter));
	std::pair<std::string, std::string> ret;
	//�u*�v���̑O�̕�����
	ret.first = path.substr(0, idx);
	//�u*�v���̌�̕�����
	ret.second = path.substr(idx + 1, path.length() - idx - 1);

	return ret;
}

//Char�����񂩂�wchar_t������ւ̕ϊ�
std::wstring GetWideStringFromString(const std::string& str)
{
	//�Ăяo��1���
	auto num1 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		nullptr,
		0);

	std::wstring wstr;
	wstr.resize(num1);

	//�Ăяo��2���
	auto num2 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		&wstr[0],
		num1);

	//�ꉞ�`�F�b�N
	assert(num1 == num2);
	return wstr;
}

//�e�N�X�`�����[�h
ComPtr<ID3D12Resource> Application::LoadTextureFromFile(std::string& texPath, ID3D12Device* dev)
{
	//�t�@�C�����ƃp�X�ƃ��\�[�X�̃}�b�v�e�[�u��

	auto it = _resourceTable.find(texPath);
	if (it != _resourceTable.end())
	{
		//�e�[�u�����ɂ������烍�[�h����̂ł͂Ȃ��}�b�v���̃��\�[�X��Ԃ�
		return it->second;
	}

	//WIC�e�N�X�`���̃��[�h

	if (_loadLambdaTable.empty())
	{
		_loadLambdaTable["sph"]
			= _loadLambdaTable["spa"]
			= _loadLambdaTable["bmp"]
			= _loadLambdaTable["png"]
			= _loadLambdaTable["jpg"]
			= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
		{
			return LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, meta, img);
		};
		_loadLambdaTable["tga"]
			= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
		{
			return LoadFromTGAFile(path.c_str(), meta, img);
		};
		_loadLambdaTable["dds"]
			= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
		{
			return LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, meta, img);
		};
	}

	//�ȉ��G���[�R�[�h
	//�e�N�X�`�����[�h
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	//�e�N�X�`���̃t�@�C���p�X
	std::wstring wtexpath = GetWideStringFromString(texPath);
	//�g���q���擾
	std::string ext = GetExtension(texPath);
	if (_loadLambdaTable.find(ext) == _loadLambdaTable.end()) {
		return nullptr;
	}
	auto result = _loadLambdaTable[ext](
		wtexpath,
		&metadata,
		scratchImg);
	if (FAILED(result)) { return nullptr; }

	//WriteToSubresource1�œ]������p�̃q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	const D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		static_cast<UINT>(metadata.width),
		static_cast<UINT>(metadata.height),
		static_cast<UINT16>(metadata.arraySize),
		static_cast<UINT16>(metadata.mipLevels));

	//�o�b�t�@�[�쐬
	ComPtr<ID3D12Resource> texbuff = nullptr;
	result = dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(texbuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) { return nullptr; }

	//���f�[�^�N�o
	const Image* img = scratchImg.GetImage(0, 0, 0);

	result = texbuff->WriteToSubresource(
		0,
		nullptr,
		img->pixels,
		static_cast<UINT>(img->rowPitch), //1���C���T�C�Y
		static_cast<UINT>(img->slicePitch)//�S�T�C�Y
	);

	if (FAILED(result)) { return nullptr; }

	_resourceTable[texPath] = texbuff.Get();

	return texbuff.Get();
}

ComPtr<ID3D12Resource> CreateMonoTexture(ID3D12Device* dev, unsigned int val)
{
	D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	ComPtr<ID3D12Resource> whiteBuff = nullptr;
	auto result = dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // ���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(whiteBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) { return nullptr; }

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), val); // �S��val�Ŗ��߂�

	// �f�[�^�]��
	result = whiteBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		static_cast<UINT>(data.size()));

	return whiteBuff;
}

ComPtr<ID3D12Resource> CreateWhiteTexture(ID3D12Device* dev)
{
	return CreateMonoTexture(dev, 0xff);
}

ComPtr<ID3D12Resource> CreateBlackTexture(ID3D12Device* dev)
{
	return CreateMonoTexture(dev, 0x00);
}

//�f�t�H���g�O���f�[�V�����e�N�X�`��
ComPtr<ID3D12Resource> CreateGrayGradationTexture(ID3D12Device* dev)
{
	D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);

	ComPtr<ID3D12Resource> gradBuff = nullptr;
	HRESULT result = dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(gradBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) { return nullptr; }

	//���������낫���������e�N�X�`���f�[�^���쐬
	std::vector<unsigned int>data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4)
	{
		//RGBA���t���т̈�RGB�}�N����0xff<<24��p���ĕ\��
		unsigned int col = (0xff) << 24 | RGB(c, c, c);
		fill(it, it + 4, col);
		--c;
	}
	result = gradBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * static_cast<UINT>(sizeof(unsigned int)),
		static_cast<UINT>(sizeof(unsigned int) * data.size()));

	return gradBuff;
}

//�A���C�����g�ɂ��낦���T�C�Y��Ԃ�
size_t AlignmentedSize(size_t size, size_t alignment)
{
	return size + alignment - size % alignment;
}

#ifdef _DEBUG
void EnableDebugLayer()
{
	ComPtr<ID3D12Debug> debugLayer = nullptr;
	HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(debugLayer.ReleaseAndGetAddressOf()));
	if (!SUCCEEDED(result)) return;

	//�f�o�b�O���C���[��L���ɂ���
	debugLayer->EnableDebugLayer();
	//�L����������C���^�[�t�F�C�X���������
	debugLayer->Release();
}
#endif// _DEBUG

//�E�B���h�E����
HWND Application::CreateGameWindow(WNDCLASSEX _windowClass,int _window_width,int _window_height)
{
	//�N���X����
	_windowClass.cbSize = sizeof(WNDCLASSEX);
	//�R�[���o�b�N�֐��̎w��
	_windowClass.lpfnWndProc = (WNDPROC)WindowProcedure;
	//�A�v���P�[�V�����N���X��
	_windowClass.lpszClassName = _T("DX12Sample");
	//�n���h���̎擾
	_windowClass.hInstance = GetModuleHandle(nullptr);

	//�A�v���P�[�V�����N���X(�E�B���h�E�N���X�̎w���OS�ɓ`����)
	RegisterClassEx(&_windowClass);

	//�E�B���h�E�T�C�Y�����߂�
	RECT wrc = { 0,0,(LONG)_window_width,(LONG)_window_height };

	//�֐����g���ăE�B���h�E�̃T�C�Y��ݒ肷��
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	return CreateWindow(
		//�N���X���w��
		_windowClass.lpszClassName,
		//�^�C�g���o�[�̕���
		_T("DX12 �e�X�g"),
		//�^�C�g���o�[�Ƌ��E���̂���E�B���h�E
		WS_OVERLAPPEDWINDOW,
		//�\��x(OS����)
		CW_USEDEFAULT,
		//�\��y(���l)
		CW_USEDEFAULT,
		//�E�B���h�E��
		wrc.right - wrc.left,
		//�E�B���h�E��
		wrc.bottom - wrc.top,
		//�e�E�B���h�E�n���h��
		nullptr,
		//���j���[�n���h��
		nullptr,
		//�Ăяo���A�v���P�[�V�����n���h��
		_windowClass.hInstance,
		//�ǉ��p�����[�^-
		nullptr);
}

//�R�}���h���X�g�쐬
void Application::InitializeCommand()
{
	ThrowIfFailed(_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(_cmdAllocator.ReleaseAndGetAddressOf())));
	ThrowIfFailed(_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		_cmdAllocator.Get(), nullptr,
		IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf())));

	//�R�}���h�L���[�쐬
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	//�^�C���A�E�g�Ȃ�
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	//�A�_�v�^�[��1�����g��Ȃ��ꍇ��0�ŉ�
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	//�R�}���h���X�g�ƍ��킹��
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	//�L���[�쐬
	ThrowIfFailed(_dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(_cmdQueue.ReleaseAndGetAddressOf())));
}

//�X���b�v�`�F�[���쐬
void Application::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = _window_width;
	swapchainDesc.Height = _window_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	//�o�b�t�@�[�͐L�яk�݉\
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	//�t���b�v��͑��₩�ɔj��
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//�w��Ȃ�
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	//�E�B���h�E�t���X�N���[���ؑ։\
	ThrowIfFailed(_dxgiFactory->CreateSwapChainForHwnd(
		_cmdQueue.Get(), _hwnd,
		&swapchainDesc, nullptr, nullptr,
		(IDXGISwapChain1**)_swapchain.ReleaseAndGetAddressOf()));
}

//�o�b�N�o�b�t�@�̍쐬
void Application::InitializeBackBuffers()
{
	//�X���b�v�`�F�[���̃o�b�t�@���擾
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	//�����_�[�^�[�Q�b�g�r���[�Ȃ̂�RTV
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NodeMask = 0;
	//���\��2��
	heapDesc.NumDescriptors = 3;
	//�w��Ȃ�
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ThrowIfFailed(_dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_rtvHeaps.ReleaseAndGetAddressOf())));
	//�X���b�v�`�F�[���̃��\�[�X��RTV���֘A�t����
	//�X���b�v�`�F�[���̏��̎擾
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	ThrowIfFailed(_swapchain->GetDesc(&swcDesc));

	//�����_�[�^�[�Q�b�g�r���[�ݒ�
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//�K���}�␳�Ȃ�
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	//�f�B�X�N���v�^���o�b�t�@�̐���������
	_backBuffers = std::vector<ComPtr<ID3D12Resource>>(swcDesc.BufferCount);
	for (UINT idx = 0; idx < swcDesc.BufferCount; ++idx)
	{
		//�o�b�t�@�̃|�C���^���擾
		ThrowIfFailed(_swapchain->GetBuffer(idx, IID_PPV_ARGS(_backBuffers[idx].ReleaseAndGetAddressOf())));
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(_rtvHeaps->GetCPUDescriptorHandleForHeapStart());

		handle.Offset(idx, _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		_dev->CreateRenderTargetView(_backBuffers[idx].Get(), &rtvDesc, handle);
	}
}

//�[�x�o�b�t�@�̍쐬
void Application::InitializeDepthBuffer()
{
	//�[�x�o�b�t�@�[�̍쐬(�w���p�[�\���͉̂������Ă���̂��킩��Ȃ��Ȃ�ׁA�g��Ȃ���������)
	D3D12_RESOURCE_DESC depthResDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT,
		_window_width, _window_height,
		1U, 1U, 1U, 0U, //arraySize,mipLevels,sampleCount,SampleQuality
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	//�[�x�l�p�q�[�v�v���p�e�B
	const D3D12_HEAP_PROPERTIES depthHeapProp =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	//�d�v�@�N���A����l
	CD3DX12_CLEAR_VALUE depthClearValue(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

	ThrowIfFailed(_dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(_depthBuffer.ReleaseAndGetAddressOf())));

	// �[�x�ׂ̈̃f�B�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};//�[�x�Ɏg��
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	ThrowIfFailed(_dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(_dsvHeap.ReleaseAndGetAddressOf())));

	//�[�x�r���[�쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	_dev->CreateDepthStencilView(
		_depthBuffer.Get(),
		&dsvDesc,
		_dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

//�e�N�X�`���n������
void Application::InitialixeTextureLoaderTable(FILE* fp, std::string strModelPath)
{
	//���_��S�ēǂݍ���
	constexpr size_t pmdvertex_size = 38; //���_��ӂ�̂��T�C�Y

	unsigned int vertNum; //���_�����擾
	fread(&vertNum, sizeof(vertNum), 1, fp);

#pragma pack(1)
	//PMD�}�e���A���\��
	struct PMDMaterial
	{
		XMFLOAT3 diffuse;
		float alpha;
		float specularity;
		XMFLOAT3 specular;
		XMFLOAT3 ambient;
		unsigned char toonIdx;
		unsigned char edgeFlg;

		//2�o�C�g�̃p�f�B���O

		unsigned int indicesNum;

		char texFilePath[20];
	};//72�o�C�g�ɂȂ�//�p�b�L���O�w�������
#pragma pack()

#pragma pack(push,1)
	struct PMD_VERTEX
	{
		XMFLOAT3 pos;
		XMFLOAT3 normal;
		XMFLOAT2 uv;
		uint16_t bone_no[2];
		uint8_t weight;
		uint8_t EdgeFlag;
		uint16_t dummy;
	};
#pragma pack(pop)

	//�o�b�t�@�[�̊m��
	std::vector<PMD_VERTEX> vertices(vertNum);
	for (unsigned int i = 0; i < vertNum; i++)
	{
		//�ǂݍ���
		fread(&vertices[i], pmdvertex_size, 1, fp);
	}

	auto heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resdesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(PMD_VERTEX));

	ThrowIfFailed(_dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc, //�T�C�Y�ύX
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_vertBuff.ReleaseAndGetAddressOf())));

	//���_�f�[�^���R�s-
	PMD_VERTEX* vertMap = nullptr; //�^��Vertex�ɕύX
	ThrowIfFailed(_vertBuff->Map(0, nullptr, (void**)&vertMap));
	//�m�ۂ������C���������ɒ��_�f�[�^���R�s�[
	copy(begin(vertices), end(vertices), vertMap);
	//CPU�Őݒ�͊��������̂Ńr�f�I�������ɓ]��
	_vertBuff->Unmap(0, nullptr);

	//���_�o�b�t�@�r���[(���_�o�b�t�@�̐���)
	//�o�b�t�@�[�̉��z�A�h���X
	_vbView.BufferLocation = _vertBuff->GetGPUVirtualAddress();
	//�S�o�C�g��
	_vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(PMD_VERTEX));
	//1���_������̃o�C�g��
	_vbView.StrideInBytes = sizeof(vertices[0]);

	//�C���f�b�N�X�o�b�t�@�̍쐬
	unsigned int indicesNum; //�C���f�b�N�X��
	fread(&indicesNum, sizeof(indicesNum), 1, fp);

	std::vector<unsigned short> indices;
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	//�ݒ�̓o�b�t�@�[�T�C�Y�ȊO���_�o�b�t�@�[�̐ݒ���g���܂킵�ėǂ�
	resdesc.Width = indices.size() * sizeof(indices[0]);
	ThrowIfFailed(_dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_idxBuff.ReleaseAndGetAddressOf())));

	//������o�b�t�@�[�ɃC���f�b�N�X�f�[�^���R�s�[
	unsigned short* mappedIdx = nullptr;
	_idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	_idxBuff->Unmap(0, nullptr);

	//�C���f�b�N�X�o�b�t�@�[�r���[���쐬
	_ibView.BufferLocation = _idxBuff->GetGPUVirtualAddress();
	_ibView.Format = DXGI_FORMAT_R16_UINT;
	_ibView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(indices[0]));

	//�}�e���A����
	fread(&_materialNum, sizeof(_materialNum), 1, fp);

	std::vector<PMDMaterial>pmdMaterials(_materialNum);

	fread(
		pmdMaterials.data(),
		pmdMaterials.size() * sizeof(PMDMaterial),
		1,
		fp); //��C�ɓǂݍ���

	materials.resize(pmdMaterials.size());
	//�R�s�[
	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		materials[i].indicesNum = pmdMaterials[i].indicesNum;
		materials[i].material.diffuse = pmdMaterials[i].diffuse;
		materials[i].material.alpha = pmdMaterials[i].alpha;
		materials[i].material.specular = pmdMaterials[i].specular;
		materials[i].material.specularity = pmdMaterials[i].specularity;
		materials[i].material.ambient = pmdMaterials[i].ambient;
		materials[i].additional.toonIdx = pmdMaterials[i].toonIdx;
	}

	//�e�N�X�`���Ăяo������
	_textureResources.resize(_materialNum);
	//�X�t�B�A�}�b�v�̓ǂݍ���
	_sphResources.resize(_materialNum);
	//���Z�X�t�B�A�}�b�v�̓ǂݍ���
	_spaResources.resize(_materialNum);
	//�g�D�[���V�F�[�_�[
	_toonResources.resize(_materialNum);

	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		//�g�D�[�����\�[�X�ǂݍ���
		std::string toonFilePath = "toon/";
		char toonFileName[16];
		sprintf_s(toonFileName, "toon%02d.bmp", pmdMaterials[i].toonIdx + 1);
		toonFilePath += toonFileName;
		_toonResources[i] = LoadTextureFromFile(toonFilePath, _dev.Get());

		if (strlen(pmdMaterials[i].texFilePath) == 0) { continue; }

		//[*]�����鎞������������
		std::string texFileName = pmdMaterials[i].texFilePath;
		std::string sphFileName = "";
		std::string spaFileName = "";

		if (count(texFileName.begin(), texFileName.end(), '*') > 0)
		{
			//�X�v���b�^������
			auto namepair = SplitFileName(texFileName);
			//spa���X�t�B�A�}�b�v(���Z�X�t�B�A�}�b�v)
			if (GetExtension(namepair.first) == "sph")
			{
				texFileName = namepair.second;
				sphFileName = namepair.first;
			}
			else if (GetExtension(namepair.first) == "spa")
			{
				texFileName = namepair.second;
				spaFileName = namepair.first;
			}
			else
			{
				//�܂��̓e�N�X�`���t�@�[�X�g
				texFileName = namepair.first;
				if (GetExtension(namepair.second) == "sph")
				{
					sphFileName = namepair.second;
				}
				else if (GetExtension(namepair.second) == "spa")
				{
					spaFileName = namepair.second;
				}
			}
		}
		else
		{
			std::string ext = GetExtension(pmdMaterials[i].texFilePath);
			//�X�t�B�A�}�b�v�����ŃA�x���x���Ȃ��ꍇ�ɂ��Ή�
			if (ext == "sph")
			{
				sphFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
			else if (ext == "spa")
			{
				spaFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
		}

		//���f���ƃe�N�X�`���p�X����A�v���P�[�V������̃e�N�X�`���p�X�𓾂�
		auto texFilePath = GetTexturePathFromModelAndTexPath(
			strModelPath,
			texFileName.c_str());
		_textureResources[i] = LoadTextureFromFile(texFilePath, _dev.Get());

		if (sphFileName != "")
		{
			auto sphFilePath = GetTexturePathFromModelAndTexPath(strModelPath, sphFileName.c_str());
			_sphResources[i] = LoadTextureFromFile(sphFilePath, _dev.Get());
		}

		if (spaFileName != "")
		{
			auto spaFilePath = GetTexturePathFromModelAndTexPath(strModelPath, spaFileName.c_str());
			_spaResources[i] = LoadTextureFromFile(spaFilePath, _dev.Get());
		}
	}

	//�}�e���A���o�b�t�@�[���쐬
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;

	const D3D12_HEAP_PROPERTIES heapPropMat =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const D3D12_RESOURCE_DESC resDescMat = CD3DX12_RESOURCE_DESC::Buffer(
		materialBuffSize * _materialNum);
	ThrowIfFailed(_dev->CreateCommittedResource(
		&heapPropMat,
		D3D12_HEAP_FLAG_NONE,
		&resDescMat,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_materialBuff.ReleaseAndGetAddressOf()))
	);

	//�}�b�v�}�e���A���ɃR�s�[
	char* mapMaterial = nullptr;
	ThrowIfFailed(_materialBuff->Map(0, nullptr, (void**)&mapMaterial));
	for (auto& m : materials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material; //�f�[�^���R�s�[
		mapMaterial += materialBuffSize; //���̃A���C�����g�ʒu�܂Ői�߂�i256�̔{���j
	}
	_materialBuff->Unmap(0, nullptr);

	//�}�e���A���p�o���X�N���v�^�q�[�v�ƃr��-�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC materialDescHeapDesc = {};
	materialDescHeapDesc.Flags =
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	materialDescHeapDesc.NodeMask = 0;
	materialDescHeapDesc.NumDescriptors = _materialNum * 5;
	materialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	ThrowIfFailed(_dev->CreateDescriptorHeap(
		&materialDescHeapDesc, IID_PPV_ARGS(_materialDescHeap.ReleaseAndGetAddressOf())));

	//�f�B�X�N���v�^�q�[�v��Ƀr���[���쐬
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};

	matCBVDesc.BufferLocation =
		_materialBuff->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes =
		static_cast<UINT>(materialBuffSize);

	//�ʏ�e�N�X�`���r���[�쐬
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.Shader4ComponentMapping =
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension =
		D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	//�擪���L�^
	CD3DX12_CPU_DESCRIPTOR_HANDLE matDescHeapH(
		_materialDescHeap->GetCPUDescriptorHandleForHeapStart());
	UINT incSize = _dev->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//�r���[���쐬
	for (UINT i = 0; i < _materialNum; ++i)
	{
		//�}�e���A���p�萔�o�b�t�@�[�r���[
		_dev->CreateConstantBufferView(&matCBVDesc, matDescHeapH);

		matDescHeapH.ptr += incSize;
		matCBVDesc.BufferLocation += materialBuffSize;

		//�V�F�[�_�[���\�[�X�r���[
		if (_textureResources[i] == nullptr)
		{
			srvDesc.Format = _whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(_whiteTex.Get(), &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = _textureResources[i]->GetDesc().Format;

			_dev->CreateShaderResourceView(
				_textureResources[i].Get(),
				&srvDesc,
				matDescHeapH);
		}

		matDescHeapH.ptr += incSize;

		if (_sphResources[i] == nullptr)
		{
			srvDesc.Format = _whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(_whiteTex.Get(), &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = _sphResources[i]->GetDesc().Format;

			_dev->CreateShaderResourceView(
				_sphResources[i].Get(),
				&srvDesc,
				matDescHeapH);
		}

		matDescHeapH.ptr += incSize;

		if (_spaResources[i] == nullptr)
		{
			srvDesc.Format = _blackTex->GetDesc().Format;
			_dev->CreateShaderResourceView(_blackTex.Get(), &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = _spaResources[i]->GetDesc().Format;

			_dev->CreateShaderResourceView(
				_spaResources[i].Get(),
				&srvDesc,
				matDescHeapH);
		}

		matDescHeapH.ptr += incSize;

		//�g�D�[���e�N�X�`���p�̃r���[�쐬
		if (_toonResources[i] == nullptr)
		{
			srvDesc.Format = _gradTex->GetDesc().Format;
			_dev->CreateShaderResourceView(_gradTex.Get(), &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = _toonResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(_toonResources[i].Get(), &srvDesc, matDescHeapH);
		}

		matDescHeapH.ptr += incSize;
	}

}

//���[�g�V�O�l�`���̒ǉ�
void Application::CreateRootSignature(ComPtr<ID3DBlob> errorBlob)
{
	HRESULT result;

	CD3DX12_DESCRIPTOR_RANGE descTblRange[3] = {};
	//���W�ϊ��p
	descTblRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	//�}�e���A���p
	descTblRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	//�e�N�X�`��4��
	descTblRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);

	CD3DX12_ROOT_PARAMETER rootparam[2] = {};
	rootparam[0].InitAsDescriptorTable(1, &descTblRange[0]);
	rootparam[1].InitAsDescriptorTable(2, &descTblRange[1]);

	//�T���v���[�̐ݒ�
	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
	samplerDesc[0].Init(0);
	samplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	//��̃��[�g�V�O�l�`��(���_���̂�)
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init(
		2, rootparam,
		2, samplerDesc,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	//���[�g�V�O�l�`���I�u�W�F�N�g�̍쐬�E�ݒ�
	ComPtr<ID3DBlob> rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(
		&rootSignatureDesc, //���[�g�V�O�l�`���ݒ�
		D3D_ROOT_SIGNATURE_VERSION_1_0, //���[�g�V�O�l�`���o�[�W����
		&rootSigBlob, //�V�F�[�_�[��������Ƃ��Ɠ���
		&errorBlob);//�G���[���������l

	ThrowIfFailed(_dev->CreateRootSignature(
		0, //nodemask
		rootSigBlob->GetBufferPointer(),//�V�F�[�_�[�̎��Ɠ��l
		rootSigBlob->GetBufferSize(), //�V�F�[�_�[�̎��Ɠ��l
		IID_PPV_ARGS(_rootsigunature.ReleaseAndGetAddressOf())));
}

ComPtr<ID3DBlob> LoadShader(LPCWSTR hlsl, LPCSTR basic, LPCSTR ver)
{
	ComPtr<ID3DBlob> blob;
	HRESULT result;

	ComPtr<ID3DBlob> errorBlob = nullptr;

	result = D3DCompileFromFile(
		//�V�F�[�_�[��
		hlsl,
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		basic, ver,
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&blob, &errorBlob);

	//�s�N�Z���V�F�[�_�[�̃R���p�C�����[�h
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("�t�@�C����������܂���");
		}
		else
		{
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);
	}

	return blob;
}

bool Application::Init()
{
	ThrowIfFailed(CoInitializeEx(0, COINIT_MULTITHREADED));

	//�E�B���h�E�I�u�W�F�N�g�̐���
	_hwnd = CreateGameWindow(_windowClass,_window_width,_window_height);

#ifdef _DEBUG
	//�f�o�b�O���C���[�I��
	EnableDebugLayer();
#endif // DEBUG
	//D3D12Device�̐���
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

#ifdef _DEBUG
	ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf())));
#else
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf())));
#endif // _DEBUG

	//�A�_�v�^�[�̗񋓂ƌ���
	//�A�_�v�^�[�񋓗p
	std::vector<IDXGIAdapter*> adapters;
	//�����ɓ���̖��O�����A�_�v�^�[�I�u�W�F�N�g������
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}
	//�񋓂����܂�
	//����
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);
		std::wstring strDesc = adesc.Description;
		//�T�������A�_�v�^�[�̖��O���m�F(null�łȂ����m�F����)
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	//Direct3D�f�o�C�X�̏�����
	D3D_FEATURE_LEVEL featureLevel;
	for (auto lv : levels)
	{
		if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(_dev.ReleaseAndGetAddressOf())) == S_OK)
		{
			featureLevel = lv;
			break;
		}
	}

	//�R�}���h���X�g�쐬
	InitializeCommand();

	//�X���b�v�`�F�[���̍쐬
	CreateSwapChain();
	//�o�b�N�o�b�t�@
	InitializeBackBuffers();
	//�[�x�o�b�t�@�[
	InitializeDepthBuffer();

	//�t�F���X�I�u�W�F�N�g�̐錾�Ɛ����i�҂��ŕK�v�j
	ThrowIfFailed(_dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf())));

	//�f�t�H���g�O���f�[�V�����e�N�X�`��
	_gradTex = CreateGrayGradationTexture(_dev.Get());
	//�z���C�g�e�N�X�`���Ăяo��
	_whiteTex = CreateWhiteTexture(_dev.Get());
	//���e�N�X�`���쐬
	_blackTex = CreateBlackTexture(_dev.Get());

	//���f���n
	//�\���̂̓ǂݍ���
	struct PMDHeader
	{
		float version; //�� 00 00 80 3F == 1.00
		char model_name[20]; //���f����
		char comment[256]; //���f���R�����g
	};

	char signature[3] = {}; //�V�O�l�`��
	PMDHeader pmdheader = {};

	std::string strModelPath = "Model/�����~�N.pmd";
	FILE* fp;
	fopen_s(&fp, strModelPath.c_str(), "rb");

	//fp��nullptr�̑Ή������ĂȂ��x��
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	//�e�N�X�`���n
	InitialixeTextureLoaderTable(fp, strModelPath);

	//����
	fclose(fp);

	ComPtr<ID3DBlob> errorBlob = nullptr;

	ComPtr<ID3DBlob> _vsBlob = LoadShader(L"BasicVertexShader.hlsl", "BasicVS", "vs_5_0");
	ComPtr<ID3DBlob> _psBlob = LoadShader(L"BasicPixelShader.hlsl", "BasicPS", "ps_5_0");

	//���_�f�[�^�ɍ��킹�����_���C�A�E�g�̕ύX
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{
		"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{
		"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{
		"BONE_NO",0,DXGI_FORMAT_R16G16_UINT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{
		"WEIGHT",0,DXGI_FORMAT_R8_UINT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{
		"EDGE_FLG",0,DXGI_FORMAT_R8_UINT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

	};

	//���_�V�F�[�_�[�E�s�N�Z���V�F�[�_�[���쐬
	//�V�F�[�_�[�̐ݒ�
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};

	gpipeline.pRootSignature = nullptr; //��Őݒ�

	gpipeline.VS = CD3DX12_SHADER_BYTECODE(_vsBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(_psBlob.Get());

	//�f�t�H���g�̃T���v���}�X�N��\���萔(0xffffffff)
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//�܂��A���`�G�C���A�X�͎g��Ȃ��ׂ�false
	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//�[�x�X�e���V��
	gpipeline.DepthStencilState.DepthEnable = true;
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//��������

	gpipeline.DepthStencilState.DepthFunc =
		D3D12_COMPARISON_FUNC_LESS;//�������ق����̗p
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//�u�����h�X�e�[�g�̐ݒ�
	gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	//���̓��C�A�E�g�̐ݒ�
	gpipeline.InputLayout.pInputElementDescs = inputLayout; //���C�A�E�g�擪�A�h���X
	gpipeline.InputLayout.NumElements = _countof(inputLayout); //���C�A�E�g�z��̗v�f��
	
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; //�X�g���b�v���̃J�b�g����
	//�|���S���͎O�p�`�@�ו����p�A���p�`�̃f�[�^�p��_PATCH
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; //�O�p�`�ō\��

	//���̑��ݒ�
	//�����_�[�^�[�Q�b�g�̐ݒ�
	gpipeline.NumRenderTargets = 1;
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; //0�`1�ɐ��K�����ꂽRGBA

	//�A���`�G�C���A�X�̃T���v�����ݒ�
	gpipeline.SampleDesc.Count = 1; //�T���v�����O��1�s�N�Z���ɕt��1
	gpipeline.SampleDesc.Quality = 0;//�N�I���e�B�͍Œ�

	//���[�g�V�O�l�`���ł̃f�B�X�N���v�^�̎w��
	//�f�B�X�N���v�^�����W
	//���[�g�V�O�l�`���ɃX���b�g�ƃe�N�X�`���̊֘A���L�q����
	//�f�B�X�N���v�^�e�[�u���F�萔�o�b�t�@�[��e�N�X�`���Ȃǂ̃��W�X�^�ԍ��̎w����܂Ƃ߂Ă������
	//���[�g�V�O�l�`���̒ǉ�
	CreateRootSignature(errorBlob.Get());

	gpipeline.pRootSignature = _rootsigunature.Get();

	ThrowIfFailed(_dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(_pipelinestate.ReleaseAndGetAddressOf())));

	//�r���[�|�[�g
	//�o�͂����RT�̃X�N���[�����W�n�ł̈ʒu�Ƒ傫��
	//Depth�͐[�x�o�b�t�@
	_viewport = CD3DX12_VIEWPORT(_backBuffers[0].Get());
	//�V�U�[��`�F�r���[�|�[�g�̒��Ŏ��ۂɕ`�悳���͈�
	_scissorrect = CD3DX12_RECT(0, 0, _window_width, _window_height);

	//WIC�e�N�X�`�����[�h
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	ThrowIfFailed(LoadFromWICFile(
		L"img/textest.png", WIC_FLAGS_NONE,
		&metadata, scratchImg));

	//���f�[�^���o
	auto img = scratchImg.GetImage(0, 0, 0);

	//���ԃo�b�t�@�[�Ƃ��Ẵ��[�h�q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES uploadHeapProp =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	//���^�f�[�^�̗��p
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;

	resDesc.Width =
		AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)
		* img->height;

	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;

	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;

	//���ԃo�b�t�@�[�쐬
	ComPtr<ID3D12Resource> uploadbuff = nullptr;

	ThrowIfFailed(_dev->CreateCommittedResource(
		&uploadHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadbuff.ReleaseAndGetAddressOf())));

	//�e�N�X�`���o�b�t�@
	//�q�[�v�̐ݒ�
	const D3D12_HEAP_PROPERTIES texHeapProp =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	//���^�f�[�^�̍ė��p
	resDesc.Format = metadata.format;
	resDesc.Width = static_cast<UINT>(metadata.width);
	resDesc.Height = static_cast<UINT>(metadata.height);
	resDesc.DepthOrArraySize = static_cast<uint16_t>(metadata.arraySize);
	resDesc.MipLevels = static_cast<uint16_t>(metadata.mipLevels);
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//���C�A�E�g�͌��肵�Ȃ�

	HRESULT result;
	//���\�[�X�̐���
	ComPtr<ID3D12Resource> texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(texbuff.ReleaseAndGetAddressOf()));

	//�A�b�v���[�h���\�[�X�ւ̃}�b�v
	uint8_t* mapforImg = nullptr;
	result = uploadbuff->Map(0, nullptr, (void**)&mapforImg);//�}�b�v

	//�Y���̔��ǂ�}����
	auto srcAddress = img->pixels;

	auto rowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	for (int y = 0; y < img->height; y++)
	{
		std::copy_n(srcAddress,
			rowPitch,
			mapforImg);//�R�s�[

		//1�s���Ƃ̂��܂��킹
		srcAddress += img->rowPitch;
		mapforImg += rowPitch;
	}

	//copy_n(img->pixels, img->slicePitch, mapforImg);//�R�s�[
	uploadbuff->Unmap(0, nullptr);//�A���}�b�v

	//CopyTextureReGion()���g���R�[�h
	D3D12_TEXTURE_COPY_LOCATION src = {};
	//�R�s�[���ݒ�
	src.pResource = uploadbuff.Get();
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src.PlacedFootprint.Offset = 0;
	src.PlacedFootprint.Footprint.Width = static_cast<UINT>(metadata.width);
	src.PlacedFootprint.Footprint.Height = static_cast<UINT>(metadata.height);
	src.PlacedFootprint.Footprint.Depth = static_cast<UINT>(metadata.depth);
	src.PlacedFootprint.Footprint.RowPitch =
		static_cast<UINT>(AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
	src.PlacedFootprint.Footprint.Format = img->format;

	D3D12_TEXTURE_COPY_LOCATION dst = {};

	//�R�s�[��ݒ�
	dst.pResource = texbuff.Get();
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.SubresourceIndex = 0;

	_cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	
	//�o���A�t�F���X�ݒ�
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = texbuff.Get();
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore =
		D3D12_RESOURCE_STATE_COPY_DEST;// �������d�v
	BarrierDesc.Transition.StateAfter =
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;// �������d�v

	_cmdList->ResourceBarrier(1, &BarrierDesc);
	_cmdList->Close();

	//�R�}���h���X�g�̎��s
	ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(1, cmdlists);

	//�t�F���X�҂�
	_cmdQueue->Signal(_fence.Get(), ++_fenceVal);
	if (_fence->GetCompletedValue() != _fenceVal)
	{
		//�i�r�W�[���[�v���g����CPU���������̂ŗǂ��Ȃ�)
		//�X���b�h��Q�����ăC�x���g���N������OS�ɋN�����Ă��炤
		auto event = CreateEvent(nullptr, false, false, nullptr);
		//�C�x���g�n���h���̎擾
		_fence->SetEventOnCompletion(_fenceVal, event);
		//�C�x���g����������܂Ŗ����ɑ҂�
		WaitForSingleObject(event, INFINITE);
		//�C�x���g�n���h�������
		CloseHandle(event);
	}
	//�L���[���N���A
	_cmdAllocator->Reset();
	//�ĂуR�}���h���X�g�����߂鏀��
	_cmdList->Reset(_cmdAllocator.Get(), nullptr);

	//��]����
	_worldMat = XMMatrixRotationY(XM_PIDIV4);

	//�r���[�s��
	XMFLOAT3 eye(0, 10, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);

	_viewMat = XMMatrixLookAtLH(
		XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	//�v���W�F�N�V�����s��
	_projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,//��p��90��
		static_cast<float>(_window_width)
		/ static_cast<float>(_window_height), //�A�X�y�N�g��
		1.0f,//�߂��ق�
		100.0f//�����ق�
	);

	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//256�o�C�g�A���C�������g�A����8�r�b�g��0�ł���΂��̂܂�
	//�����łȂ����256�P�ʂŐ؂�グ
	resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff);
	_dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_constBuff.ReleaseAndGetAddressOf())
	);

	//�}�b�v�ɂ��萔�̃R�s�[
	ThrowIfFailed(_constBuff->Map(0, nullptr, (void**)&_mapScene)); //�}�b�v
	_mapScene->world = _worldMat;
	_mapScene->view = _viewMat;
	_mapScene->proj = _projMat;
	_mapScene->eye = eye;

	//�萔�o�b�t�@�[�r���[���f�B�X�N���v�^�q�[�v�ɒǉ�
	//�V�F�[�_�[���\�[�X�r��
	//�f�B�X�N���v�^�q�[�v���쐬
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	//�V�F�[�_�[���猩����悤��
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//�}�X�N��0
	descHeapDesc.NodeMask = 0;
	//�r���[SRV1�Ƃ�CBV1��(SRV���摜�Ƃ��̃e�N�X�`�� CBV���ʒu�Ȃ�)
	descHeapDesc.NumDescriptors = 1;
	//�V�F�[�_�[���\�[�X�r���[�p
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//����
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(_basicDescHeap.ReleaseAndGetAddressOf()));

	//�f�B�X�N���v�^�̐擪�n���h�����擾���Ă���
	CD3DX12_CPU_DESCRIPTOR_HANDLE basicHeapHandle(_basicDescHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	//GPU�̈ʒu���w��
	cbvDesc.BufferLocation = _constBuff->GetGPUVirtualAddress();
	//�R���X�^���g�o�b�t�@�[�r���[���V�F�[�_�[���\�[�X�r���[�̌�ɓ����悤�ɂ���
	cbvDesc.SizeInBytes = static_cast<UINT>(_constBuff->GetDesc().Width);

	//�萔�o�b�t�@�r���[�̍쐬
	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);

	return true;
}

void Application::Run()
{
	//�E�B���h�E�\��
	ShowWindow(_hwnd, SW_SHOW);

	//��]�p�p�x
	float angle = 0.0f;
	float index = 0.0f;

	while (true)
	{
		MSG msg;
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			//�A�v�����I��鎞��message��WM_QUIT�ɂȂ�
			if (msg.message == WM_QUIT)
			{
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		//��]
		angle += 0.01f;
		_worldMat = XMMatrixRotationY(angle);
		_mapScene->world = _worldMat;
		_mapScene->view = _viewMat;
		_mapScene->proj = _projMat;

		//�X���b�v�`�F�[���̓���
		//DirectX����
		//�o�b�N�o�b�t�@�̃C���f�b�N�X���擾
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		auto BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[bbIdx].Get(), D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		_cmdList->ResourceBarrier(1, &BarrierDesc);
		//�p�C�v���C���X�e�[�g�̃Z�b�g
		_cmdList->SetPipelineState(_pipelinestate.Get());

		//�����_�[�^�[�Q�b�g���w��
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvH(_rtvHeaps->GetCPUDescriptorHandleForHeapStart());
		rtvH.Offset(bbIdx * _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvH(_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		_cmdList->OMSetRenderTargets(1, &rtvH, true, &dsvH);

		//��ʃN���A
		float clearColor[] = { 1.0f,1.0f,1.0f, 1.0f };
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		//���[�g�V�O�l�`���̃Z�b�g
		_cmdList->SetGraphicsRootSignature(_rootsigunature.Get());
		//�f�B�X�N���v�^�q�[�v�̎w��
		_cmdList->SetDescriptorHeaps(1, _basicDescHeap.GetAddressOf());
		//���[�g�p�����[�^�[�ƃf�B�X�N���v�^�q�[�v�̊֘A�t��
		_cmdList->SetGraphicsRootDescriptorTable(0,
			_basicDescHeap->GetGPUDescriptorHandleForHeapStart());

		//�I�u�W�F�N�g����
		//�r���[�|�[�g�ƃV�U�[��`�̃Z�b�g
		_cmdList->RSSetViewports(1, &_viewport);
		_cmdList->RSSetScissorRects(1, &_scissorrect);
		//�v���~�e�B�u�g�|���W�̃Z�b�g
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//���_���̃Z�b�g
		_cmdList->IASetVertexBuffers(0, 1, &_vbView);
		_cmdList->IASetIndexBuffer(&_ibView);

		//�}�e���A���̐ݒ�
		_cmdList->SetDescriptorHeaps(1, _materialDescHeap.GetAddressOf());

		auto materialH = _materialDescHeap->GetGPUDescriptorHandleForHeapStart();//�q�[�v�擪

		unsigned int idx0ffset = 0; //�ŏ��̓I�t�Z�b�g�Ȃ�

		UINT cbvsrvIncSize = _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;

		for (Material& m : materials)
		{
			_cmdList->SetGraphicsRootDescriptorTable(1, materialH);

			_cmdList->DrawIndexedInstanced(m.indicesNum, 1, idx0ffset, 0, 0);

			//�q�[�v�|�C���^�[�ƃC���f�b�N�X�����ɐi�߂�
			materialH.ptr += cbvsrvIncSize;
			idx0ffset += m.indicesNum;
		}

		//�O�ゾ������ւ���
		BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[bbIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		//���߂̃N���[�Y
		_cmdList->Close();

		//�R�}���h���X�g�̎��s
		ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
		_cmdQueue->ExecuteCommandLists(1, cmdlists);

		//�҂�
		_cmdQueue->Signal(_fence.Get(), ++_fenceVal);
		if (_fence->GetCompletedValue() != _fenceVal)
		{
			//�i�r�W�[���[�v���g����CPU���������̂ŗǂ��Ȃ�)
			//�X���b�h��Q�����ăC�x���g���N������OS�ɋN�����Ă��炤
			auto event = CreateEvent(nullptr, false, false, nullptr);
			//�C�x���g�n���h���̎擾
			_fence->SetEventOnCompletion(_fenceVal, event);
			//�C�x���g����������܂Ŗ����ɑ҂�
			WaitForSingleObject(event, INFINITE);
			//�C�x���g�n���h�������
			CloseHandle(event);
		}

		//�L���[���N���A
		_cmdAllocator->Reset();
		//�ĂуR�}���h���X�g�����߂鏀��
		_cmdList->Reset(_cmdAllocator.Get(), _pipelinestate.Get());

		//�t���b�v
		_swapchain->Present(1, 0);
	}
}

void Application::Terminate()
{
#ifdef _DEBUG
	// ���������J���̏ڍו\��
	ComPtr<ID3D12DebugDevice> debugInterface;
	if (SUCCEEDED(_dev->QueryInterface(debugInterface.GetAddressOf())))
	{
		debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		debugInterface->Release();
	}
#endif// _DEBUG

	// �����N���X�͎g��Ȃ��̂œo�^��������
	UnregisterClass(_windowClass.lpszClassName, _windowClass.hInstance);
}

Application::Application()
{
}


Application::~Application()
{
}

Application& Application::Instance()
{
	static Application instance;
	return instance;
}
