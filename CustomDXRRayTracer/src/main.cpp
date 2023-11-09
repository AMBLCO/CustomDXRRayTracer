#include "window.h"
#include "graphics.h"
#include "utils.h"

#include <sstream>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

class DXRApplication
{
public:
	void Init(ConfigInfo& config)
	{
		HRESULT hr = Window::Create(config.width, config.height, config.instance, window, config.windowName);
		Utils::Validate(hr, L"Error: failed to create a window");

		d3d.width = config.width;
		d3d.height = config.height;
		d3d.vsync = config.vsync;

		Utils::LoadModel(config.model, model, material);

		D3DShaders::Init_Shader_Compiler(shaderCompiler);

		D3D12::Create_Device(d3d);
		D3D12::Create_Command_Queue(d3d);
		D3D12::Create_Command_Allocator(d3d);
		D3D12::Create_Fence(d3d);
		D3D12::Create_SwapChain(d3d, window);
		D3D12::Create_CommandList(d3d);
		D3D12::Reset_CommandList(d3d);

		D3DResources::Create_Descriptor_Heaps(d3d, resources);
		D3DResources::Create_BackBuffer_RTV(d3d, resources);
		D3DResources::Create_Vertex_Buffer(d3d, resources, model);
		D3DResources::Create_Index_Buffer(d3d, resources, model);
		D3DResources::Create_Texture(d3d, resources, material);
		D3DResources::Create_View_CB(d3d, resources);
		D3DResources::Create_Material_CB(d3d, resources, material);

		DXR::Create_Bottom_Level_AS(d3d, dxr, resources, model);
		DXR::Create_Top_Level_AS(d3d, dxr, resources);
		DXR::Create_DXR_Output(d3d, resources);
		DXR::Create_Descriptor_Heaps(d3d, dxr, resources, model);
		DXR::Create_RayGen_Program(d3d, dxr, shaderCompiler);
		DXR::Create_Miss_Program(d3d, dxr, shaderCompiler);
		DXR::Create_Closest_Hit_Program(d3d, dxr, shaderCompiler);
		DXR::Create_Pipeline_State_Object(d3d, dxr);
		DXR::Create_Shader_Table(d3d, dxr, resources);

		d3d.cmdList->Close();
		ID3D12CommandList* pGraphicsList = { d3d.cmdList };
		d3d.cmdQueue->ExecuteCommandLists(1, &pGraphicsList);

		D3D12::WaitForGPU(d3d);
		D3D12::Reset_CommandList(d3d);
	}

	void Update()
	{
		D3DResources::Update_View_CB(d3d, resources);
	}

	void Render()
	{
		DXR::Build_Command_List(d3d, dxr, resources);
		D3D12::Present(d3d);
		D3D12::MoveToNextFrame(d3d);
		D3D12::Reset_CommandList(d3d);
	}

	void Cleanup()
	{
		D3D12::WaitForGPU(d3d);
		CloseHandle(d3d.fenceEvent);

		DXR::Destroy(dxr);
		D3DResources::Destroy(resources);
		D3DShaders::Destroy(shaderCompiler);
		D3D12::Destroy(d3d);

		DestroyWindow(window);
	}

	HWND window;
private:
	Model model;
	Material material;

	DXRGlobal dxr = {};
	D3D12Global d3d = {};
	D3D12Resources resources = {};
	D3D12ShaderCompilerInfo shaderCompiler = {};
};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	HRESULT hr = EXIT_SUCCESS;
	{
		MSG msg = { 0 };

		ConfigInfo config;
		config.windowName = L"Custom DX12 Realtime Raytracer";

		hr = Utils::ParseCommandLine(lpCmdLine, config);
		if (hr != EXIT_SUCCESS) return hr;

		DXRApplication app;
		app.Init(config);

		while (WM_QUIT != msg.message)
		{
			Utils::Timer timer;

			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			app.Update();
			app.Render();

			std::wstringstream windowName;
			float renderTime = timer.ElapsedMillis();

			windowName << config.windowName << " " << renderTime << " ms " << 1 / renderTime * 1000 << " FPS";
			SetWindowText(app.window, windowName.str().c_str());
		}

		app.Cleanup();
	}

#if defined _CRTDBG_MAP_ALLOC
	_CrtDumpMemoryLeaks();
#endif

	return hr;
}