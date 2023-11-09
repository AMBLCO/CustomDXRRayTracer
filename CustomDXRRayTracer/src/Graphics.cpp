#include <wrl.h>
#include <atlcomcli.h>

#include "Graphics.h"
#include "Utils.h"

namespace D3DResources
{
	void Create_Buffer(D3D12Global& d3d, D3D12BufferCreateInfo& info, ID3D12Resource** ppResource)
	{
		D3D12_HEAP_PROPERTIES heapDesc = {};
		heapDesc.Type = info.heapType;
		heapDesc.CreationNodeMask = 1;
		heapDesc.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = info.alignment;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Width = info.size;
		resourceDesc.Flags = info.flags;

		HRESULT hr = d3d.device->CreateCommittedResource(&heapDesc, D3D12_HEAP_FLAG_NONE, &resourceDesc, info.state, nullptr, IID_PPV_ARGS(ppResource));
		Utils::Validate(hr, L"Error: failed to create buffer resource");
	}

	void Create_Texture(D3D12Global& d3d, D3D12Resources& resources, Material& material)
	{
		TextureInfo texture = Utils::LoadTexture(material.texturePath);
		material.textureResolution = texture.width;

		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.Width = texture.width;
		textureDesc.Height = texture.height;
		textureDesc.MipLevels = 1;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		
		HRESULT hr = d3d.device->CreateCommittedResource(&DefaultHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resources.texture));
		Utils::Validate(hr, L"Error: failed to create texture");

#if NAME_D3D_RESOURCES
		resources.texture->SetName(L"Texture");
#endif

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Width = texture.width * texture.height * texture.stride;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;

		hr = d3d.device->CreateCommittedResource(&UploadHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resources.textureUploadResource));
		Utils::Validate(hr, L"Error: failed to create texture upload heap");

#if NAME_D3D_RESOURCES
		resources.texture->SetName(L"Texture Upload Buffer");
#endif

		Upload_Texture(d3d, resources.texture, resources.textureUploadResource, texture);
	}

	void Create_Vertex_Buffer(D3D12Global& d3d, D3D12Resources& resources, Model& model)
	{
		D3D12BufferCreateInfo info(((UINT)model.vertices.size() * sizeof(Vertex)), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
		Create_Buffer(d3d, info, &resources.vertexBuffer);

#if NAME_D3D_RESOURCES
		resources.texture->SetName(L"Vertex Buffer");
#endif

		UINT8* pVertexDataBegin;
		D3D12_RANGE readRange = {};
		HRESULT hr = resources.vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
		Utils::Validate(hr, L"Error: failed to map vertex buffer");

		memcpy(pVertexDataBegin, model.vertices.data(), info.size);
		resources.vertexBuffer->Unmap(0, nullptr);

		resources.vertexBufferView.BufferLocation = resources.vertexBuffer->GetGPUVirtualAddress();
		resources.vertexBufferView.StrideInBytes = sizeof(Vertex);
		resources.vertexBufferView.SizeInBytes = static_cast<UINT>(info.size);
	}

	void Create_Index_Buffer(D3D12Global& d3d, D3D12Resources& resources, Model& model)
	{
		D3D12BufferCreateInfo info(((UINT)model.indices.size() * sizeof(UINT)), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
		Create_Buffer(d3d, info, &resources.indexBuffer);

#if NAME_D3D_RESOURCES
		resources.texture->SetName(L"Index Buffer");
#endif

		UINT8* pIndexDataBegin;
		D3D12_RANGE readRange = {};
		HRESULT hr = resources.indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
		Utils::Validate(hr, L"Error: failed to map index buffer");

		memcpy(pIndexDataBegin, model.indices.data(), info.size);
		resources.indexBuffer->Unmap(0, nullptr);

		resources.indexBufferView.BufferLocation = resources.indexBuffer->GetGPUVirtualAddress();
		resources.indexBufferView.SizeInBytes = static_cast<UINT>(info.size);
		resources.indexBufferView.SizeInBytes = DXGI_FORMAT_R32_UINT;
	}

	void Create_Constant_Buffer(D3D12Global& d3d, ID3D12Resource** buffer, UINT64 size)
	{
		D3D12BufferCreateInfo bufferInfo((size + 255) & ~255, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
		Create_Buffer(d3d, bufferInfo, buffer);
	}

	void Create_BackBuffer_RTV(D3D12Global& d3d, D3D12Resources& resources)
	{
		HRESULT hr;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;

		rtvHandle = resources.rtvHeap->GetCPUDescriptorHandleForHeapStart();

		for (UINT n = 0; n < 2; n++)
		{
			hr = d3d.swapChain->GetBuffer(n, IID_PPV_ARGS(&d3d.backBuffer[n]));
			Utils::Validate(hr, L"Error: failed to get swap chain buffer");

			d3d.device->CreateRenderTargetView(d3d.backBuffer[n], nullptr, rtvHandle);

#if NAME_D3D_RESOURCES
			d3d.backBuffer[n]->SetName(L"Back Buffer " + n);
#endif

			rtvHandle.ptr += resources.rtvDescSize;
		}
	}

	void Create_View_CB(D3D12Global& d3d, D3D12Resources& resources)
	{
		Create_Constant_Buffer(d3d, &resources.viewCB, sizeof(MaterialCB));

#if NAME_D3D_RESOURCES
		resources.viewCB->SetName(L"View Constant Buffer");
#endif

		HRESULT hr = resources.viewCB->Map(0, nullptr, reinterpret_cast<void**>(&resources.viewCBStart));
		Utils::Validate(hr, L"Error: failed to map view constant buffer");

		memcpy(resources.viewCBStart, &resources.viewCBData, sizeof(resources.viewCBData));
	}

	void Create_Material_CB(D3D12Global& d3d, D3D12Resources& resources, const Material& material)
	{
		Create_Constant_Buffer(d3d, &resources.materialCB, sizeof(MaterialCB));

#if NAME_D3D_RESOURCES
		resources.materialCB->SetName(L"Material Constant Buffer");
#endif

		resources.materialCBData.resolution = DirectX::XMFLOAT4(material.textureResolution, 0.f, 0.f, 0.f);

		HRESULT hr = resources.materialCB->Map(0, nullptr, reinterpret_cast<void**>(&resources.materialCBStart));
		Utils::Validate(hr, L"Error: failed to map material constant buffer");

		memcpy(resources.materialCBStart, &resources.materialCBData, sizeof(resources.materialCBData));
	}

	void Create_Descriptor_Heaps(D3D12Global& d3d, D3D12Resources& resources)
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
		rtvDesc.NumDescriptors = 2;
		rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		HRESULT hr = d3d.device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&resources.rtvHeap));
		Utils::Validate(hr, L"Error: failed to create RTV descriptor heap");

#if NAME_D3D_RESOURCES
		resources.rtvHeap->SetName(L"RTV Descriptor Heap");
#endif

		resources.rtvDescSize = d3d.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	void Update_View_CB(D3D12Global& d3d, D3D12Resources& resources)
	{

	}

	void Upload_Texture(D3D12Global& d3d, ID3D12Resource* destResource, ID3D12Resource* srcResource, const TextureInfo& texture)
	{
		UINT8* pData;

		HRESULT hr = srcResource->Map(0, nullptr, reinterpret_cast<void**>(&pData));

		memcpy(pData, texture.pixels.data(), texture.width * texture.height * texture.stride);

		srcResource->Unmap(0, nullptr);

		D3D12_SUBRESOURCE_FOOTPRINT subresource = {};
		subresource.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		subresource.Width = texture.width;
		subresource.Height = texture.height;
		subresource.RowPitch = texture.width * texture.stride;
		subresource.Depth = 1;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
		footprint.Offset = texture.offset;
		footprint.Footprint = subresource;

		D3D12_TEXTURE_COPY_LOCATION source = {};
		source.pResource = srcResource;
		source.PlacedFootprint = footprint;
		source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

		D3D12_TEXTURE_COPY_LOCATION destination = {};
		destination.pResource = destResource;
		destination.SubresourceIndex = 0;
		destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = destResource;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		d3d.cmdList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
		d3d.cmdList->ResourceBarrier(1, &barrier);
	}

	void Destroy(D3D12Resources& resources)
	{
		if (resources.viewCB) resources.viewCB->Unmap(0, nullptr);
		if (resources.viewCBStart) resources.viewCBStart = nullptr;
		if (resources.materialCB) resources.materialCB->Unmap(0, nullptr);
		if (resources.materialCBStart) resources.materialCBStart = nullptr;

		SAFE_RELEASE(resources.DXROutput);
		SAFE_RELEASE(resources.vertexBuffer);
		SAFE_RELEASE(resources.indexBuffer);
		SAFE_RELEASE(resources.viewCB);
		SAFE_RELEASE(resources.materialCB);
		SAFE_RELEASE(resources.rtvHeap);
		SAFE_RELEASE(resources.descriptorHeap);
		SAFE_RELEASE(resources.texture);
		SAFE_RELEASE(resources.textureUploadResource);
	}
}

namespace D3DShaders
{
	void Init_Shader_Compiler(D3D12ShaderCompilerInfo& shaderCompiler)
	{

	}

	void Compile_Shader(D3D12ShaderCompilerInfo& compilerInfo, RtProgram& program)
	{

	}

	void Compile_Shader(D3D12ShaderCompilerInfo& compilerInfo, D3D12ShaderInfo& info, IDxcBlob** blob)
	{

	}

	void Destroy(D3D12ShaderCompilerInfo& shaderCompiler)
	{

	}
}

namespace D3D12
{
	void Create_Device(D3D12Global& d3d)
	{

	}

	void Create_CommandList(D3D12Global& d3d)
	{

	}

	void Create_Command_Queue(D3D12Global& d3d)
	{

	}

	void Create_Command_Allocator(D3D12Global& d3d)
	{

	}

	void Create_CommandList(D3D12Global& d3d)
	{

	}

	void Create_Fence(D3D12Global& d3d)
	{

	}

	void Create_SwapChain(D3D12Global& d3d, HWND& window)
	{

	}

	ID3D12RootSignature* Create_Root_Signature(D3D12Global& d3d, const D3D12_ROOT_SIGNATURE_DESC& desc)
	{

	}

	void Reset_CommandList(D3D12Global& d3d)
	{

	}

	void Submit_CmdList(D3D12Global& d3d)
	{

	}

	void Present(D3D12Global& d3d)
	{

	}

	void WaitForGPU(D3D12Global& d3d)
	{

	}

	void MoveToNextFrame(D3D12Global& d3d)
	{

	}

	void Destroy(D3D12Global& d3d)
	{

	}
}

namespace DXR
{
	void Create_Bottom_Level_AS(D3D12Global& d3d, DXRGlobal& dxr, D3D12Resources& resources, Model& model)
	{

	}

	void Create_Top_Level_AS(D3D12Global& d3d, DXRGlobal& dxr, D3D12Resources& resources)
	{

	}

	void Create_RayGen_Program(D3D12Global& d3d, DXRGlobal& dxr, D3D12ShaderCompilerInfo& shaderCompiler)
	{

	}

	void Create_Miss_Program(D3D12Global& d3d, DXRGlobal& dxr, D3D12ShaderCompilerInfo& shaderCompiler)
	{

	}

	void Create_Closest_Hit_Program(D3D12Global& d3d, DXRGlobal& dxr, D3D12ShaderCompilerInfo& shaderCompiler)
	{

	}

	void Create_Pipeline_State_Object(D3D12Global& d3d, DXRGlobal& dxr)
	{

	}

	void Create_Shader_Table(D3D12Global& d3d, DXRGlobal& dxr, D3D12Resources& resources)
	{

	}

	void Create_Descriptor_Heaps(D3D12Global& d3d, DXRGlobal& dxr, D3D12Resources& resources, const Model& model)
	{

	}

	void Create_DXR_Output(D3D12Global& d3d, D3D12Resources& resources)
	{

	}

	void Build_Command_List(D3D12Global& d3d, DXRGlobal& dxr, D3D12Resources& resources)
	{

	}

	void Destroy(DXRGlobal& dxr)
	{

	}
}