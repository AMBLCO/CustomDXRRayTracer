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
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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
		resources.vertexBuffer->SetName(L"Vertex Buffer");
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
		resources.indexBuffer->SetName(L"Index Buffer");
#endif

		UINT8* pIndexDataBegin;
		D3D12_RANGE readRange = {};
		HRESULT hr = resources.indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
		Utils::Validate(hr, L"Error: failed to map index buffer");

		memcpy(pIndexDataBegin, model.indices.data(), info.size);
		resources.indexBuffer->Unmap(0, nullptr);

		resources.indexBufferView.BufferLocation = resources.indexBuffer->GetGPUVirtualAddress();
		resources.indexBufferView.SizeInBytes = static_cast<UINT>(info.size);
		resources.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
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

		resources.materialCBData.resolution = DirectX::XMFLOAT4((float)material.textureResolution, 0.f, 0.f, 0.f);

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
		const float rotationSpeed = 0.005f;
		DirectX::XMMATRIX view, invView;
		DirectX::XMFLOAT3 eye, focus, up;
		float aspect, fov;

		resources.eyeAngle.x += rotationSpeed;

#if _DEBUG
		float x = 2.f * cosf(resources.eyeAngle.x);
		float y = 0.f;
		float z = 2.25f + 2.f * sinf(resources.eyeAngle.x);

		focus = DirectX::XMFLOAT3(0.f, 0.f, 0.f);
#else
		float x = 8.f * cosf(resources.eyeAngle.x);
		float y = 1.5f + 1.5f * cosf(resources.eyeAngle.x);
		float z = 8.f + 2.25f * sinf(resources.eyeAngle.x);

		focus = DirectX::XMFLOAT3(0.f, 1.75f, 0.f);
#endif

		eye = DirectX::XMFLOAT3(x, y, z);
		up = DirectX::XMFLOAT3(0.f, 1.f, 0.f);

		aspect = (float)d3d.width / (float)d3d.height;
		fov = 65.f * (DirectX::XM_PI / 180.f);

		resources.rotationOffset += rotationSpeed;

		view = DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&eye), DirectX::XMLoadFloat3(&focus), DirectX::XMLoadFloat3(&up));
		invView = DirectX::XMMatrixInverse(NULL, view);

		resources.viewCBData.view = DirectX::XMMatrixTranspose(invView);
		resources.viewCBData.viewOriginAndTanHalfFovY = DirectX::XMFLOAT4(eye.x, eye.y, eye.z, tanf(fov * 0.5f));
		resources.viewCBData.resolution = DirectX::XMFLOAT2((float)d3d.width, (float)d3d.height);

		memcpy(resources.viewCBStart, &resources.viewCBData, sizeof(resources.viewCBData));
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
		HRESULT hr = shaderCompiler.DxcDllHelper.Initialize();
		Utils::Validate(hr, L"Failed to initialize DxCDLLSupport");

		hr = shaderCompiler.DxcDllHelper.CreateInstance(CLSID_DxcCompiler, &shaderCompiler.compiler);
		Utils::Validate(hr, L"Failed to create DxcCompiler");

		hr = shaderCompiler.DxcDllHelper.CreateInstance(CLSID_DxcLibrary, &shaderCompiler.library);
		Utils::Validate(hr, L"Failed to create DxcLibrary");
	}

	void Compile_Shader(D3D12ShaderCompilerInfo& compilerInfo, RtProgram& program)
	{
		Compile_Shader(compilerInfo, program.info, &program.blob);
		program.SetBytecode();
	}

	void Compile_Shader(D3D12ShaderCompilerInfo& compilerInfo, D3D12ShaderInfo& info, IDxcBlob** blob)
	{
		HRESULT hr;
		UINT32 code(0);
		IDxcBlobEncoding* pShaderText = nullptr;

		hr = compilerInfo.library->CreateBlobFromFile(info.filename, &code, &pShaderText);
		Utils::Validate(hr, L"Failed to create blob from shader file");

		CComPtr<IDxcIncludeHandler> dxcIncludeHandler;
		hr = compilerInfo.library->CreateIncludeHandler(&dxcIncludeHandler);
		Utils::Validate(hr, L"Failed to create include handler");

		IDxcOperationResult* result;
		hr = compilerInfo.compiler->Compile(
			pShaderText,
			info.filename,
			info.entryPoint,
			info.targetProfile,
			info.arguments,
			info.argCount,
			info.defines,
			info.defineCount,
			dxcIncludeHandler,
			&result);
		Utils::Validate(hr, L"Error: failed to compile shader");

		result->GetStatus(&hr);
		if (FAILED(hr))
		{
			IDxcBlobEncoding* error;
			hr = result->GetErrorBuffer(&error);
			Utils::Validate(hr, L"Error: failed to get shader compiler error buffer");

			std::vector<char> infoLog(error->GetBufferSize() + 1);
			memcpy(infoLog.data(), error->GetBufferPointer(), error->GetBufferSize());
			infoLog[error->GetBufferSize()] = 0;

			std::string errorMsg = "Shader Compiler Error:\n";
			errorMsg.append(infoLog.data());

			MessageBoxA(nullptr, errorMsg.c_str(), "Error", MB_OK);
			return;
		}

		hr = result->GetResult(blob);
		Utils::Validate(hr, L"Error: failed to get shader blob result");
	}

	void Destroy(D3D12ShaderCompilerInfo& shaderCompiler)
	{
		SAFE_RELEASE(shaderCompiler.compiler);
		SAFE_RELEASE(shaderCompiler.library);
		shaderCompiler.DxcDllHelper.Cleanup();
	}
}

namespace D3D12
{
	void Create_Device(D3D12Global& d3d)
	{
#if defined(_DEBUG)
		{
			ID3D12Debug* debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
			}
		}
#endif

		HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&d3d.factory));
		Utils::Validate(hr, L"Error: failed to create DXGI factory");

		d3d.adapter = nullptr;
		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != d3d.factory->EnumAdapters1(adapterIndex, &d3d.adapter); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 adapterDesc;
			d3d.adapter->GetDesc1(&adapterDesc);

			if (adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)
			{
				continue;
			}

			if (SUCCEEDED(D3D12CreateDevice(d3d.adapter, D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device5), (void**)&d3d.device)))
			{
				D3D12_FEATURE_DATA_D3D12_OPTIONS5 features = {};
				HRESULT hr = d3d.device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features, sizeof(features));
				if (FAILED(hr) || features.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
				{
					SAFE_RELEASE(d3d.device);
					d3d.device = nullptr;
					continue;
				}

#if NAME_D3D_RESOURCES
				d3d.device->SetName(L"DXR Enanbled Device");
				printf("Running on DXGI Adapter %S\n", adapterDesc.Description);
#endif
				break;
			}

			if (d3d.device == nullptr)
			{
				Utils::Validate(E_FAIL, L"Error: failed to create ray tracing device");
			}
		}
	}

	void Create_Command_Queue(D3D12Global& d3d)
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		HRESULT hr = d3d.device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d.cmdQueue));
		Utils::Validate(hr, L"Error: failed to create command queue");

#if NAME_D3D_RESOURCES
		d3d.cmdQueue->SetName(L"D3D12 Command Queue");
#endif
	}

	void Create_Command_Allocator(D3D12Global& d3d)
	{
		for (UINT n = 0; n < 2; n++)
		{
			HRESULT hr = d3d.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&d3d.cmdAlloc[n]));
			Utils::Validate(hr, L"Error: failed to create the command alllocator");

#if NAME_D3D_RESOURCES
			d3d.cmdQueue->SetName(L"D3D12 Command Allocator" + n);
#endif
		}
	}

	void Create_CommandList(D3D12Global& d3d)
	{
		HRESULT hr = d3d.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3d.cmdAlloc[d3d.frameIndex], nullptr, IID_PPV_ARGS(&d3d.cmdList));
		hr = d3d.cmdList->Close();
		Utils::Validate(hr, L"Error: failed ot create the command list");

#if NAME_D3D_RESOURCES
		d3d.cmdList->SetName(L"D3D12 Command List");
#endif
	}

	void Create_Fence(D3D12Global& d3d)
	{
		HRESULT hr = d3d.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3d.fence));
		Utils::Validate(hr, L"Error: failed to create fence");

#if NAME_D3D_RESOURCES
		d3d.fence->SetName(L"D3D12 Fence");
#endif

		d3d.fenceValues[d3d.frameIndex]++;

		d3d.fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (d3d.fenceEvent == nullptr)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			Utils::Validate(hr, L"Error: failed to create fence event");
		}
	}

	void Create_SwapChain(D3D12Global& d3d, HWND& window)
	{
		DXGI_SWAP_CHAIN_DESC1 desc = {};
		desc.BufferCount = 2;
		desc.Width = d3d.width;
		desc.Height = d3d.height;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.SampleDesc.Count = 1;

		IDXGISwapChain1* swapChain;
		HRESULT hr = d3d.factory->CreateSwapChainForHwnd(d3d.cmdQueue, window, &desc, nullptr, nullptr, &swapChain);
		Utils::Validate(hr, L"Error: failed to create swap chain");

		hr = d3d.factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
		Utils::Validate(hr, L"Error: failed to make window association");

		hr = swapChain->QueryInterface(__uuidof(IDXGISwapChain3), reinterpret_cast<void**>(&d3d.swapChain));
		Utils::Validate(hr, L"Error: failed to cast swap chain");

		SAFE_RELEASE(swapChain);
		d3d.frameIndex = d3d.swapChain->GetCurrentBackBufferIndex();
	}

	ID3D12RootSignature* Create_Root_Signature(D3D12Global& d3d, const D3D12_ROOT_SIGNATURE_DESC& desc)
	{
		ID3DBlob* sig;
		ID3DBlob* error;
		HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error);
		Utils::Validate(hr, L"Error: failed to serialize root signature");

		ID3D12RootSignature* pRootSig;
		hr = d3d.device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&pRootSig));
		Utils::Validate(hr, L"Error: failed to create root signature");

		SAFE_RELEASE(sig);
		SAFE_RELEASE(error);
		
		return pRootSig;
	}

	void Reset_CommandList(D3D12Global& d3d)
	{
		HRESULT hr = d3d.cmdAlloc[d3d.frameIndex]->Reset();
		Utils::Validate(hr, L"Error: failed to reset command allocator");

		hr = d3d.cmdList->Reset(d3d.cmdAlloc[d3d.frameIndex], nullptr);
		Utils::Validate(hr, L"Error: failed to reset command list");
	}

	void Submit_CmdList(D3D12Global& d3d)
	{
		d3d.cmdList->Close();

		ID3D12CommandList* pGraphicsList = { d3d.cmdList };
		d3d.cmdQueue->ExecuteCommandLists(1, &pGraphicsList);
		d3d.fenceValues[d3d.frameIndex]++;
		d3d.cmdQueue->Signal(d3d.fence, d3d.fenceValues[d3d.frameIndex]);
	}

	void Present(D3D12Global& d3d)
	{
		HRESULT hr = d3d.swapChain->Present(d3d.vsync, 0);
		if (FAILED(hr))
		{
			hr = d3d.device->GetDeviceRemovedReason();
			Utils::Validate(hr, L"Error: failed to present");
		}
	}

	void WaitForGPU(D3D12Global& d3d)
	{
		HRESULT hr = d3d.cmdQueue->Signal(d3d.fence, d3d.fenceValues[d3d.frameIndex]);
		Utils::Validate(hr, L"Error: failed to signal fence");

		hr = d3d.fence->SetEventOnCompletion(d3d.fenceValues[d3d.frameIndex], d3d.fenceEvent);
		Utils::Validate(hr, L"Error: failed to set fence event");

		WaitForSingleObjectEx(d3d.fenceEvent, INFINITE, FALSE);

		d3d.fenceValues[d3d.frameIndex]++;
	}

	void MoveToNextFrame(D3D12Global& d3d)
	{
		const UINT64 currentFenceValue = d3d.fenceValues[d3d.frameIndex];
		HRESULT hr = d3d.cmdQueue->Signal(d3d.fence, currentFenceValue);
		Utils::Validate(hr, L"Error: failed to signal command queue");

		d3d.frameIndex = d3d.swapChain->GetCurrentBackBufferIndex();

		if (d3d.fence->GetCompletedValue() < d3d.fenceValues[d3d.frameIndex])
		{
			hr = d3d.fence->SetEventOnCompletion(d3d.fenceValues[d3d.frameIndex], d3d.fenceEvent);
			Utils::Validate(hr, L"Error: failed to set fence value");

			WaitForSingleObjectEx(d3d.fenceEvent, INFINITE, FALSE);
		}

		d3d.fenceValues[d3d.frameIndex] = currentFenceValue + 1;
	}

	void Destroy(D3D12Global& d3d)
	{
		SAFE_RELEASE(d3d.fence);
		SAFE_RELEASE(d3d.backBuffer[0]);
		SAFE_RELEASE(d3d.backBuffer[1]);
		SAFE_RELEASE(d3d.swapChain);
		SAFE_RELEASE(d3d.cmdAlloc[0]);
		SAFE_RELEASE(d3d.cmdAlloc[1]);
		SAFE_RELEASE(d3d.cmdQueue);
		SAFE_RELEASE(d3d.cmdList);
		SAFE_RELEASE(d3d.device);
		SAFE_RELEASE(d3d.adapter);
		SAFE_RELEASE(d3d.factory);
	}
}

namespace DXR
{
	void Create_Bottom_Level_AS(D3D12Global& d3d, DXRGlobal& dxr, D3D12Resources& resources, Model& model)
	{
		D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc;
		geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometryDesc.Triangles.VertexBuffer.StartAddress = resources.vertexBuffer->GetGPUVirtualAddress();
		geometryDesc.Triangles.VertexBuffer.StrideInBytes = resources.vertexBufferView.StrideInBytes;
		geometryDesc.Triangles.VertexCount = static_cast<UINT>(model.vertices.size());
		geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geometryDesc.Triangles.IndexBuffer = resources.indexBuffer->GetGPUVirtualAddress();
		geometryDesc.Triangles.IndexFormat = resources.indexBufferView.Format;
		geometryDesc.Triangles.IndexCount = static_cast<UINT>(model.indices.size());
		geometryDesc.Triangles.Transform3x4 = 0;
		geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS ASInputs = {};
		ASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		ASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		ASInputs.pGeometryDescs = &geometryDesc;
		ASInputs.NumDescs = 1;
		ASInputs.Flags = buildFlags;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ASPreBuildInfo = {};
		d3d.device->GetRaytracingAccelerationStructurePrebuildInfo(&ASInputs, &ASPreBuildInfo);

		ASPreBuildInfo.ScratchDataSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ASPreBuildInfo.ScratchDataSizeInBytes);
		ASPreBuildInfo.ResultDataMaxSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ASPreBuildInfo.ResultDataMaxSizeInBytes);

		D3D12BufferCreateInfo bufferInfo(ASPreBuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		bufferInfo.alignment = max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		D3DResources::Create_Buffer(d3d, bufferInfo, &dxr.BLAS.pScratch);

#if NAME_D3D_RESOURCES
		dxr.BLAS.pScratch->SetName(L"DXR BLAS Scratch");
#endif

		bufferInfo.size = ASPreBuildInfo.ResultDataMaxSizeInBytes;
		bufferInfo.state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		D3DResources::Create_Buffer(d3d, bufferInfo, &dxr.BLAS.pResult);

#if NAME_D3D_RESOURCES
		dxr.BLAS.pResult->SetName(L"DXR BLAS");
#endif

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
		buildDesc.Inputs = ASInputs;
		buildDesc.ScratchAccelerationStructureData = dxr.BLAS.pScratch->GetGPUVirtualAddress();
		buildDesc.DestAccelerationStructureData = dxr.BLAS.pResult->GetGPUVirtualAddress();

		d3d.cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

		D3D12_RESOURCE_BARRIER uavBarrier;
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = dxr.BLAS.pResult;
		uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3d.cmdList->ResourceBarrier(1, &uavBarrier);
	}

	void Create_Top_Level_AS(D3D12Global& d3d, DXRGlobal& dxr, D3D12Resources& resources)
	{
		D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
		instanceDesc.InstanceID = 0;
		instanceDesc.InstanceContributionToHitGroupIndex = 0;
		instanceDesc.InstanceMask = 0xFF;
		instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
		instanceDesc.AccelerationStructure = dxr.BLAS.pResult->GetGPUVirtualAddress();
		instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;

		D3D12BufferCreateInfo instanceBufferInfo;
		instanceBufferInfo.size = sizeof(instanceDesc);
		instanceBufferInfo.heapType = D3D12_HEAP_TYPE_UPLOAD;
		instanceBufferInfo.flags = D3D12_RESOURCE_FLAG_NONE;
		instanceBufferInfo.state = D3D12_RESOURCE_STATE_GENERIC_READ;
		D3DResources::Create_Buffer(d3d, instanceBufferInfo, &dxr.TLAS.pInstanceDesc);

#if NAME_D3D_RESOURCES
		dxr.TLAS.pInstanceDesc->SetName(L"DXR TLAS Instance Descriptors");
#endif

		UINT8* pData;
		dxr.TLAS.pInstanceDesc->Map(0, nullptr, (void**)&pData);
		memcpy(pData, &instanceDesc, sizeof(instanceDesc));
		dxr.TLAS.pInstanceDesc->Unmap(0, nullptr);

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS ASInputs = {};
		ASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		ASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		ASInputs.InstanceDescs = dxr.TLAS.pInstanceDesc->GetGPUVirtualAddress();
		ASInputs.NumDescs = 1;
		ASInputs.Flags = buildFlags;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ASPreBuildInfo = {};
		d3d.device->GetRaytracingAccelerationStructurePrebuildInfo(&ASInputs, &ASPreBuildInfo);

		ASPreBuildInfo.ScratchDataSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ASPreBuildInfo.ScratchDataSizeInBytes);
		ASPreBuildInfo.ResultDataMaxSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ASPreBuildInfo.ResultDataMaxSizeInBytes);

		dxr.tlasSize = ASPreBuildInfo.ResultDataMaxSizeInBytes;

		D3D12BufferCreateInfo bufferInfo(ASPreBuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		bufferInfo.alignment = max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		D3DResources::Create_Buffer(d3d, bufferInfo, &dxr.TLAS.pScratch);

#if NAME_D3D_RESOURCES
		dxr.TLAS.pScratch->SetName(L"DXR TLAS Scratch");
#endif

		bufferInfo.size = ASPreBuildInfo.ResultDataMaxSizeInBytes;
		bufferInfo.state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		D3DResources::Create_Buffer(d3d, bufferInfo, &dxr.TLAS.pResult);

#if NAME_D3D_RESOURCES
		dxr.TLAS.pResult->SetName(L"DXR TLAS");
#endif

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
		buildDesc.Inputs = ASInputs;
		buildDesc.ScratchAccelerationStructureData = dxr.TLAS.pScratch->GetGPUVirtualAddress();
		buildDesc.DestAccelerationStructureData = dxr.TLAS.pResult->GetGPUVirtualAddress();

		d3d.cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

		D3D12_RESOURCE_BARRIER uavBarrier;
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = dxr.TLAS.pResult;
		uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3d.cmdList->ResourceBarrier(1, &uavBarrier);
	}

	void Create_RayGen_Program(D3D12Global& d3d, DXRGlobal& dxr, D3D12ShaderCompilerInfo& shaderCompiler)
	{
		dxr.rgs = RtProgram(D3D12ShaderInfo(L"shaders\\RayGen.hlsl", L"", L"lib_6_3"));
		D3DShaders::Compile_Shader(shaderCompiler, dxr.rgs);

		D3D12_DESCRIPTOR_RANGE ranges[3];

		ranges[0].BaseShaderRegister = 0;
		ranges[0].NumDescriptors = 2;
		ranges[0].RegisterSpace = 0;
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		ranges[0].OffsetInDescriptorsFromTableStart = 0;

		ranges[1].BaseShaderRegister = 0;
		ranges[1].NumDescriptors = 1;
		ranges[1].RegisterSpace = 0;
		ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		ranges[1].OffsetInDescriptorsFromTableStart = 2;

		ranges[2].BaseShaderRegister = 0;
		ranges[2].NumDescriptors = 4;
		ranges[2].RegisterSpace = 0;
		ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[2].OffsetInDescriptorsFromTableStart = 3;

		D3D12_ROOT_PARAMETER param0 = {};
		param0.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param0.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		param0.DescriptorTable.NumDescriptorRanges = _countof(ranges);
		param0.DescriptorTable.pDescriptorRanges = ranges;

		D3D12_ROOT_PARAMETER rootParams[1] = { param0 };

		D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
		rootDesc.NumParameters = _countof(rootParams);
		rootDesc.pParameters = rootParams;
		rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		dxr.rgs.pRootSignature = D3D12::Create_Root_Signature(d3d, rootDesc);

#if NAME_D3D_RESOURCES
		dxr.rgs.pRootSignature->SetName(L"DXR RGS Root Signature");
#endif
	}

	void Create_Miss_Program(D3D12Global& d3d, DXRGlobal& dxr, D3D12ShaderCompilerInfo& shaderCompiler)
	{
		dxr.miss = RtProgram(D3D12ShaderInfo(L"shaders\\Miss.hlsl", L"", L"lib_6_3"));
		D3DShaders::Compile_Shader(shaderCompiler, dxr.miss);
	}

	void Create_Closest_Hit_Program(D3D12Global& d3d, DXRGlobal& dxr, D3D12ShaderCompilerInfo& shaderCompiler)
	{
		dxr.hit = HitProgram(L"Hit");
		dxr.hit.chs = RtProgram(D3D12ShaderInfo(L"shaders\\ClosestHit.hlsl", L"", L"lib_6_3"));
		D3DShaders::Compile_Shader(shaderCompiler, dxr.hit.chs);
	}

	void Create_Pipeline_State_Object(D3D12Global& d3d, DXRGlobal& dxr)
	{
		UINT index = 0;
		std::vector<D3D12_STATE_SUBOBJECT> subObjects;
		subObjects.resize(10);

		D3D12_EXPORT_DESC rgsExportDesc = {};
		rgsExportDesc.Name = L"RayGen_12";
		rgsExportDesc.ExportToRename = L"RayGen";
		rgsExportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

		D3D12_DXIL_LIBRARY_DESC rgsLibDesc = {};
		rgsLibDesc.DXILLibrary.BytecodeLength = dxr.rgs.blob->GetBufferSize();
		rgsLibDesc.DXILLibrary.pShaderBytecode = dxr.rgs.blob->GetBufferPointer();
		rgsLibDesc.NumExports = 1;
		rgsLibDesc.pExports = &rgsExportDesc;

		D3D12_STATE_SUBOBJECT rgs = {};
		rgs.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		rgs.pDesc = &rgsLibDesc;

		subObjects[index++] = rgs;

		D3D12_EXPORT_DESC msExportDesc = {};
		msExportDesc.Name = L"Miss_5";
		msExportDesc.ExportToRename = L"Miss";
		msExportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

		D3D12_DXIL_LIBRARY_DESC msLibDesc = {};
		msLibDesc.DXILLibrary.BytecodeLength = dxr.miss.blob->GetBufferSize();
		msLibDesc.DXILLibrary.pShaderBytecode = dxr.miss.blob->GetBufferPointer();
		msLibDesc.NumExports = 1;
		msLibDesc.pExports = &msExportDesc;

		D3D12_STATE_SUBOBJECT ms = {};
		ms.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		ms.pDesc = &msLibDesc;

		subObjects[index++] = ms;

		D3D12_EXPORT_DESC chsExportDesc = {};
		chsExportDesc.Name = L"ClosestHit_76";
		chsExportDesc.ExportToRename = L"ClosestHit";
		chsExportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

		D3D12_DXIL_LIBRARY_DESC chsLibDesc = {};
		chsLibDesc.DXILLibrary.BytecodeLength = dxr.hit.chs.blob->GetBufferSize();
		chsLibDesc.DXILLibrary.pShaderBytecode = dxr.hit.chs.blob->GetBufferPointer();
		chsLibDesc.NumExports = 1;
		chsLibDesc.pExports = &chsExportDesc;

		D3D12_STATE_SUBOBJECT chs = {};
		chs.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		chs.pDesc = &chsLibDesc;

		subObjects[index++] = chs;

		D3D12_HIT_GROUP_DESC hitGroupDesc = {};
		hitGroupDesc.ClosestHitShaderImport = L"ClosestHit_76";
		hitGroupDesc.HitGroupExport = L"HitGroup";

		D3D12_STATE_SUBOBJECT hitGroup = {};
		hitGroup.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		hitGroup.pDesc = &hitGroupDesc;

		subObjects[index++] = hitGroup;

		D3D12_RAYTRACING_SHADER_CONFIG shaderDesc = {};
		shaderDesc.MaxPayloadSizeInBytes = sizeof(DirectX::XMFLOAT4);
		shaderDesc.MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;

		D3D12_STATE_SUBOBJECT shaderConfigObject = {};
		shaderConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
		shaderConfigObject.pDesc = &shaderDesc;

		subObjects[index++] = shaderConfigObject;

		const WCHAR* shaderExports[] = { L"RayGen_12", L"Miss_5", L"HitGroup" };

		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderPayloadAssociation = {};
		shaderPayloadAssociation.NumExports = _countof(shaderExports);
		shaderPayloadAssociation.pExports = shaderExports;
		shaderPayloadAssociation.pSubobjectToAssociate = &subObjects[index - 1];

		D3D12_STATE_SUBOBJECT shaderPayloadAssociationObject = {};
		shaderPayloadAssociationObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		shaderPayloadAssociationObject.pDesc = &shaderPayloadAssociation;

		subObjects[index++] = shaderPayloadAssociationObject;

		D3D12_STATE_SUBOBJECT rayGenRootSigObject = {};
		rayGenRootSigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		rayGenRootSigObject.pDesc = &dxr.rgs.pRootSignature;

		subObjects[index++] = rayGenRootSigObject;

		const WCHAR* rootSigExports[] = { L"RayGen_12", L"Miss_5", L"HitGroup" };

		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION rayGenShaderRootSigAssociation = {};
		rayGenShaderRootSigAssociation.NumExports = _countof(rootSigExports);
		rayGenShaderRootSigAssociation.pExports = rootSigExports;
		rayGenShaderRootSigAssociation.pSubobjectToAssociate = &subObjects[index - 1];

		D3D12_STATE_SUBOBJECT rayGenShaderRootSigAssociationObject = {};
		rayGenShaderRootSigAssociationObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		rayGenShaderRootSigAssociationObject.pDesc = &rayGenShaderRootSigAssociation;

		subObjects[index++] = rayGenShaderRootSigAssociationObject;

		D3D12_STATE_SUBOBJECT globalRootSig = {};
		globalRootSig.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
		globalRootSig.pDesc = &dxr.miss.pRootSignature;

		subObjects[index++] = globalRootSig;

		D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
		pipelineConfig.MaxTraceRecursionDepth = 1;

		D3D12_STATE_SUBOBJECT pipelineConfigObject = {};
		pipelineConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
		pipelineConfigObject.pDesc = &pipelineConfig;

		subObjects[index++] = pipelineConfigObject;

		D3D12_STATE_OBJECT_DESC pipelineDesc = {};
		pipelineDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
		pipelineDesc.NumSubobjects = static_cast<UINT>(subObjects.size());
		pipelineDesc.pSubobjects = subObjects.data();

		HRESULT hr = d3d.device->CreateStateObject(&pipelineDesc, IID_PPV_ARGS(&dxr.rtpso));
		Utils::Validate(hr, L"Error: failed to create state object");

#if NAME_D3D_RESOURCES
		dxr.rtpso->SetName(L"DXR Pipeline State Object");
#endif

		hr = dxr.rtpso->QueryInterface(IID_PPV_ARGS(&dxr.rtpsoInfo));
		Utils::Validate(hr, L"Error: failed to get RTPSO info object");
	}

	void Create_Shader_Table(D3D12Global& d3d, DXRGlobal& dxr, D3D12Resources& resources)
	{
		uint32_t shaderIdSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		uint32_t shaderTableSize = 0;

		dxr.shaderTableRecordSize = shaderIdSize;
		dxr.shaderTableRecordSize += 8;
		dxr.shaderTableRecordSize = ALIGN(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, dxr.shaderTableRecordSize);

		shaderTableSize = dxr.shaderTableRecordSize * 3;
		shaderTableSize = ALIGN(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, shaderTableSize);

		D3D12BufferCreateInfo bufferInfo(shaderTableSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
		D3DResources::Create_Buffer(d3d, bufferInfo, &dxr.shaderTable);

#if NAME_D3D_RESOURCES
		dxr.shaderTable->SetName(L"DXR Shader Table");
#endif

		uint8_t* pData;
		HRESULT hr = dxr.shaderTable->Map(0, nullptr, (void**)&pData);
		Utils::Validate(hr, L"Error: failed to map shader table");

		memcpy(pData, dxr.rtpsoInfo->GetShaderIdentifier(L"RayGen_12"), shaderIdSize);

		*reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(pData + shaderIdSize) = resources.descriptorHeap->GetGPUDescriptorHandleForHeapStart();

		pData += dxr.shaderTableRecordSize;
		memcpy(pData, dxr.rtpsoInfo->GetShaderIdentifier(L"Miss_5"), shaderIdSize);

		pData += dxr.shaderTableRecordSize;
		memcpy(pData, dxr.rtpsoInfo->GetShaderIdentifier(L"HitGroup"), shaderIdSize);

		*reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(pData + shaderIdSize) = resources.descriptorHeap->GetGPUDescriptorHandleForHeapStart();

		dxr.shaderTable->Unmap(0, nullptr);
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