#pragma once

#include "common.h"

static bool CompareVector3WithEpsilon(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs)
{
	const DirectX::XMFLOAT3 vector3Epsilon = DirectX::XMFLOAT3(0.00001f, 0.00001f, 0.00001f);
	return DirectX::XMVector3NearEqual(DirectX::XMLoadFloat3(&lhs), DirectX::XMLoadFloat3(&rhs), DirectX::XMLoadFloat3(&vector3Epsilon));
}

static bool CompareVector2WithEpsilon(const DirectX::XMFLOAT2& lhs, const DirectX::XMFLOAT2& rhs)
{
	const DirectX::XMFLOAT2 vector3Epsilon = DirectX::XMFLOAT2(0.00001f, 0.00001f);
	return DirectX::XMVector2NearEqual(DirectX::XMLoadFloat2(&lhs), DirectX::XMLoadFloat2(&rhs), DirectX::XMLoadFloat2(&vector3Epsilon));
}

struct ConfigInfo
{
	LPWSTR windowName = L"";
	int width = 640;
	int height = 360;
	bool vsync = false;
	std::string model = "";
	HINSTANCE instance = NULL;
};

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 uv;

	bool operator==(const Vertex& v) const
	{
		return CompareVector3WithEpsilon(position, v.position);
	}

	Vertex& operator=(const Vertex& v)
	{
		position = v.position;
		uv = v.uv;
		return *this;
	}
};

struct Material
{
	std::string name = "defaultMaterial";
	std::string texturePath = "";
	int textureResolution = 512;
};

struct Model
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

struct TextureInfo
{
	std::vector<UINT8> pixels;
	int width = 0;
	int height = 0;
	int stride = 0;
	int offset = 0;
};

struct MaterialCB
{
	DirectX::XMFLOAT4 resolution;
};

struct ViewCB
{
	DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
	DirectX::XMFLOAT4 viewOriginAndTanHalfFovY = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 0.f);
	DirectX::XMFLOAT2 resolution = DirectX::XMFLOAT2(0.f, 0.f);
};

struct D3D12BufferCreateInfo
{
	UINT64 size = 0;
	UINT64 alignment = 0;
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;

	D3D12BufferCreateInfo() {}

	D3D12BufferCreateInfo(UINT64 InSize, D3D12_RESOURCE_FLAGS InFlags) :
		size(InSize),
		flags(InFlags) {}

	D3D12BufferCreateInfo(UINT64 InSize, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InState) :
		size(InSize),
		heapType(InHeapType),
		state(InState) {}

	D3D12BufferCreateInfo(UINT64 InSize, D3D12_RESOURCE_FLAGS InFlags, D3D12_RESOURCE_STATES InState) :
		size(InSize),
		flags(InFlags),
		state(InState) {}

	D3D12BufferCreateInfo(UINT64 InSize, UINT64 InAlignement, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_FLAGS InFlags, D3D12_RESOURCE_STATES InState) :
		size(InSize),
		alignment(InAlignement),
		heapType(InHeapType),
		flags(InFlags),
		state(InState) {}
};

struct D3D12ShaderCompilerInfo
{
	dxc::DxcDllSupport DxcDllHelper;
	IDxcCompiler* compiler = nullptr;
	IDxcLibrary* library = nullptr;
};

struct D3D12ShaderInfo
{
	LPCWSTR filename = nullptr;
	LPCWSTR entryPoint = nullptr;
	LPCWSTR targetProfile = nullptr;
	LPCWSTR* arguments = nullptr;
	DxcDefine* defines = nullptr;
	UINT32 argCount = 0;
	UINT32 defineCount = 0;

	D3D12ShaderInfo() {}

	D3D12ShaderInfo(LPCWSTR inFilename, LPCWSTR inEntryPoint, LPCWSTR inProfile) :
		filename(inFilename),
		entryPoint(inEntryPoint),
		targetProfile(inProfile) {}
};

struct D3D12Resources
{
	ID3D12Resource* DXROutput;

	ID3D12Resource* vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	ID3D12Resource* indexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	ID3D12Resource* viewCB = nullptr;
	ViewCB viewCBData;
	UINT8* viewCBStart = nullptr;

	ID3D12Resource* materialCB = nullptr;
	MaterialCB materialCBData;
	UINT8* materialCBStart = nullptr;

	ID3D12DescriptorHeap* rtvHeap = nullptr;
	ID3D12DescriptorHeap* descriptorHeap = nullptr;

	ID3D12Resource* texture = nullptr;
	ID3D12Resource* textureUploadResource = nullptr;

	UINT rtvDescSize = 0;

	float translationOffset = 0.f;
	float rotationOffset = 0.f;
	DirectX::XMFLOAT3 eyeAngle;
	DirectX::XMFLOAT3 eyePosition;
};

struct D3D12Global
{
	IDXGIFactory4* factory = nullptr;
	IDXGIAdapter1* adapter = nullptr;
	ID3D12Device5* device = nullptr;
	ID3D12GraphicsCommandList4* cmdList = nullptr;
	ID3D12CommandQueue* cmdQueue = nullptr;
	ID3D12CommandAllocator* cmdAlloc[2] = { nullptr, nullptr };

	IDXGISwapChain3* swapChain = nullptr;
	ID3D12Resource* backBuffer[2] = { nullptr, nullptr };

	ID3D12Fence* fence = nullptr;
	UINT64 fenceValues[2] = { 0, 0 };
	HANDLE fenceEvent;
	UINT frameIndex = 0;

	int width = 640;
	int height = 360;
	bool vsync = false;
};

struct AccelerationStructureBuffer
{
	ID3D12Resource* pScratch = nullptr;
	ID3D12Resource* pResult = nullptr;
	ID3D12Resource* pInstanceDesc = nullptr;
};

struct RtProgram
{
	D3D12ShaderInfo info = {};
	IDxcBlob* blob = nullptr;
	ID3D12RootSignature* pRootSignature = nullptr;

	D3D12_DXIL_LIBRARY_DESC dxilLibDesc;
	D3D12_EXPORT_DESC exportDesc;
	D3D12_STATE_SUBOBJECT subobject;
	std::wstring exportName;

	RtProgram()
	{
		exportDesc.ExportToRename = nullptr;
		dxilLibDesc = {};
		subobject = {};
	}

	RtProgram(D3D12ShaderInfo shaderInfo)
	{
		info = shaderInfo;
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		exportName = shaderInfo.entryPoint;
		exportDesc.ExportToRename = nullptr;
		exportDesc.Flags = D3D12_EXPORT_FLAG_NONE;
	}

	void SetBytecode()
	{
		exportDesc.Name = exportName.c_str();

		dxilLibDesc.NumExports = 1;
		dxilLibDesc.pExports = &exportDesc;
		dxilLibDesc.DXILLibrary.BytecodeLength = blob->GetBufferSize();
		dxilLibDesc.DXILLibrary.pShaderBytecode = blob->GetBufferPointer();

		subobject.pDesc = &dxilLibDesc;
	}
};

struct HitProgram
{
	RtProgram ahs;
	RtProgram chs;

	std::wstring exportName;
	D3D12_HIT_GROUP_DESC desc = {};
	D3D12_STATE_SUBOBJECT subobject = {};

	HitProgram() {}

	HitProgram(LPWSTR name) : exportName(name)
	{
		desc = {};
		desc.HitGroupExport = exportName.c_str();
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		subobject.pDesc = &desc;
	}

	void SetExports(bool anyHit)
	{
		desc.HitGroupExport = exportName.c_str();
		if (anyHit) desc.AnyHitShaderImport = ahs.exportDesc.Name;
		desc.ClosestHitShaderImport = chs.exportDesc.Name;
	}
};

struct DXRGlobal
{
	AccelerationStructureBuffer TLAS;
	AccelerationStructureBuffer BLAS;
	uint64_t tlasSize;

	ID3D12Resource* shaderTable = nullptr;
	uint32_t shaderTableRecordSize = 0;

	RtProgram rgs;
	RtProgram miss;
	HitProgram hit;

	ID3D12StateObject* rtpso = nullptr;
	ID3D12StateObjectProperties* rtpsoInfo = nullptr;
};