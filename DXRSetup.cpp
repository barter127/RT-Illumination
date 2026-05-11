#include "stdafx.h"
#include "DXRSetup.h"
#include "DXRContext.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include "nv_helpers_dx12/BottomLevelASGenerator.h"

#include "nv_helpers_dx12/RaytracingPipelineGenerator.h"
#include "nv_helpers_dx12/RootSignatureGenerator.h"


#include "DXRHelper.h"

#include "DrawableGameObject.h"
#include "DebugCamera.h"
#include "PointLight.h"
#include "TextureLoader.h"


DXRSetup::DXRSetup(DXRApp* app)
{
	m_app = app;
	m_device = m_app->GetContext()->m_device;
}

void DXRSetup::initialise()
{
	LoadPipeline();
	LoadAssets();

	// Check the raytracing capabilities of the device
	CheckRaytracingSupport();

	// Setup the acceleration structures (AS) for raytracing. When setting up
	// geometry, each bottom-level AS has its own transform matrix.
	CreateAccelerationStructures();

	// Command lists are created in the recording state, but there is
	// nothing to record yet. The main loop expects it to be closed, so
	// close it now.
	ThrowIfFailed(m_app->GetContext()->m_commandList->Close());

	// Create the raytracing pipeline, associating the shader code to symbol names
	// and to their root signatures, and defining the amount of memory carried by
	// rays (ray payload)
	CreateRaytracingPipeline(); // #DXR

	// Allocate the buffer storing the raytracing output, with the same dimensions
	// as the target image
	CreateRaytracingOutputBuffer(); // #DXR

	constexpr XMFLOAT3 camStartPos = { 0,0,5 };
	constexpr XMFLOAT3 camStartLookAt = { 0,0,1 };
	constexpr XMFLOAT3 camStartUp = { 0,1,0 };
	CreateCamera(camStartPos, camStartLookAt, camStartUp);
	CreateLightBuffer();
	CreateDebugBuffer();

	// Create the buffer containing the raytracing result (always output in a
	// UAV), and create the heap referencing the resources used by the raytracing,
	// such as the acceleration structure
	CreateShaderResourceHeap(); // #DXR

	// Create the shader binding table and indicating which shaders
	// are invoked for each instance in the  AS
	CreateShaderBindingTable();

	SetupIMGUI();
}

void DXRSetup::SetupIMGUI()
{
	// Setup Dear ImGui context

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 10000;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_app->GetContext()->m_IMGUIDescHeap)));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls


	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(Win32Application::GetHwnd());

	ImGui_ImplDX12_Init(m_device.Get(), FrameCount, DXGI_FORMAT_R8G8B8A8_UNORM,
		m_app->GetContext()->m_IMGUIDescHeap.Get(),
		// You'll need to designate a descriptor from your descriptor heap for Dear ImGui to use internally for its font texture's SRV
		m_app->GetContext()->m_IMGUIDescHeap.Get()->GetCPUDescriptorHandleForHeapStart(),
		m_app->GetContext()->m_IMGUIDescHeap.Get()->GetGPUDescriptorHandleForHeapStart()
	);
}

void DXRSetup::CheckRaytracingSupport() {
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
	ThrowIfFailed(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
		&options5, sizeof(options5)));
	if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
		throw std::runtime_error("Raytracing not supported on device");
}


// Load the rendering pipeline dependencies.
void DXRSetup::LoadPipeline() 
{
	UINT dxgiFactoryFlags = 0;
	DXRContext* context = m_app->GetContext();

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the
	// active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	if (m_app->m_useWarpDevice) {
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)));
	}
	else {
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		m_app->GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)));
	}

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(
		m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&context->m_commandQueue)));

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_app->m_width;
	swapChainDesc.Height = m_app->m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		context->m_commandQueue.Get(), // Swap chain needs the queue so that it can force a
		// flush on it.
		Win32Application::GetHwnd(), &swapChainDesc, nullptr, nullptr,
		&swapChain));

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(),
		DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&context->m_swapChain));
	context->m_frameIndex = context->m_swapChain->GetCurrentBackBufferIndex();

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(
			m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&context->m_rtvHeap)));

		context->m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
			context->m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (UINT n = 0; n < FrameCount; n++) {
			ThrowIfFailed(
				context->m_swapChain->GetBuffer(n, IID_PPV_ARGS(&context->m_renderTargets[n])));
			m_device->CreateRenderTargetView(context->m_renderTargets[n].Get(), nullptr,
				rtvHandle);
			rtvHandle.Offset(1, context->m_rtvDescriptorSize);
		}
	}

	ThrowIfFailed(m_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&context->m_commandAllocator)));
}

// Load the sample assets.
void DXRSetup::LoadAssets() 
{
	DXRContext* context = m_app->GetContext();

	// Create an empty root signature.
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(
			0, nullptr, 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(
			&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(
			0, signature->GetBufferPointer(), signature->GetBufferSize(),
			IID_PPV_ARGS(&context->m_rootSignature)));
	}

	// Create the pipeline state, which includes compiling and loading shaders.
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

	}

	// Create the command list.
	ThrowIfFailed(m_device->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT, context->m_commandAllocator.Get(),
		nullptr, IID_PPV_ARGS(&context->m_commandList)));

	//DrawableGameObject* copiedObj = new DrawableGameObject();
	//copiedObj->initPlaneMesh(m_device);
	//copiedObj->setPosition({ 0.3f, 0.3f, -5.0f });
	//copiedObj->update(0);

	DrawableGameObject* bunnyOBJ = new DrawableGameObject();
	bunnyOBJ->initMeshFromPath(m_device, "Models/Bunny/Bunny.obj");
	bunnyOBJ->setPosition({ 0.0f, -1.5f, -3.0 });
	bunnyOBJ->setScale({ 0.01f, 0.01f, 0.01f });
	bunnyOBJ->update(0);

	DrawableGameObject* bunnyCopy = bunnyOBJ->createCopy();
	bunnyCopy->setPosition({ 2.0f, -1.5f, -3.0 });
	bunnyCopy->setScale({ 0.01f, 0.01f, 0.01f });
	bunnyCopy->update(0);

	DrawableGameObject* FloorOBJ = new DrawableGameObject();
	FloorOBJ->initMeshFromPath(m_device, "Models/Cube.obj");
	FloorOBJ->setScale({ 10.0f, 0.2f, 10.0f });
	FloorOBJ->setPosition({ 0.0f, -2.0f, 0.0f });
	FloorOBJ->setEulerRotation({ 180.0f, 0.0f, 0.0f });
	FloorOBJ->update(0);

	DrawableGameObject* DragonOBJ = new DrawableGameObject();
	DragonOBJ->initMeshFromPath(m_device, "Models/Dragon/Dragon.obj");
	DragonOBJ->setScale({ 0.01f, 0.01f, 0.01f });
	DragonOBJ->setPosition({ 0.0f, -1.7f, 3.0 });
	DragonOBJ->setEulerRotation({ 180.0f, 0.0f, 180.0f });
	DragonOBJ->update(0);

	DrawableGameObject* DragonCopy = new DrawableGameObject();
	DragonCopy->initMeshFromPath(m_device, "Models/Dragon/Dragon.obj");
	DragonCopy->setScale({ 0.01f, 0.01f, 0.01f });
	DragonCopy->setPosition({ 2.0f, -1.7f, 3.0 });
	DragonCopy->setEulerRotation({ 180.0f, 0.0f, 180.0f });
	DragonCopy->update(0);

	m_app->m_drawableObjects.push_back(bunnyOBJ);
	m_app->m_drawableObjects.push_back(bunnyCopy);
	m_app->m_drawableObjects.push_back(FloorOBJ);
	m_app->m_drawableObjects.push_back(DragonOBJ);
	m_app->m_drawableObjects.push_back(DragonCopy);

	PointLight* light = new PointLight(
		{0.0f, 1.0f, 0.0f, 0.0f }, // Position.
		{ 0.2f, 0.2f, 0.2f,1.0f }, // Diffuse Col.
		{ 0.1f,1.0f,0.1f,1.0f }, // Ambient Col.
		{ 1.0f, 1.0f, 1.0f, 1.0f }, // Specular Col.
		28.0f, // Shininess.
		8.0f); // Attenuation.

	m_app->m_lightVector.push_back(light);

	LoadTextureFromPath(L"Models/Bunny/DefaultMaterial.png", context, 0);
	LoadTextureFromPath(L"Models/Bunny/metalnessMap1.png", context, 1);
	LoadTextureFromPath(L"Models/Bunny/normalMap1.png", context, 2);
	LoadTextureFromPath(L"Models/Dragon/DefaultMaterial.png", context, 3);
	LoadTextureFromPath(L"Models/Dragon/metalnessMap1.png", context, 4);
	LoadTextureFromPath(L"Models/Dragon/normalMap1.png", context, 5);

	// Create synchronization objects and wait until assets have been uploaded to
	// the GPU.
	{
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&context->m_fence)));
		context->m_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		context->m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (context->m_fenceEvent == nullptr) {
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Wait for the command list to execute; we are reusing the same command
		// list in our main loop but for now, we just want to wait for setup to
		// complete before continuing.
		m_app->WaitForPreviousFrame();
	}
}

//-----------------------------------------------------------------------------
//
// Combine the BLAS and TLAS builds to construct the entire acceleration
// structure required to raytrace the scene
//
void DXRSetup::CreateAccelerationStructures() 
{
	DXRContext* context = m_app->GetContext();

	// Build the bottom AS from the Triangle vertex buffer
	AccelerationStructureBuffers bottomLevelBunny =
		CreateBottomLevelAS({ {m_app->m_drawableObjects[0]->getVertexBuffer().Get(), m_app->m_drawableObjects[0]->getVertexCount()} }, 
			{ {m_app->m_drawableObjects[0]->getIndexBuffer().Get(), m_app->m_drawableObjects[0]->getIndexCount()}});

	AccelerationStructureBuffers bottomLevelBunnyCpyBuffers =
		CreateBottomLevelAS({ {m_app->m_drawableObjects[1]->getVertexBuffer().Get(), m_app->m_drawableObjects[1]->getVertexCount()} },
			{ {m_app->m_drawableObjects[1]->getIndexBuffer().Get(), m_app->m_drawableObjects[1]->getIndexCount()} });

	AccelerationStructureBuffers bottomLevelPlaneBuffers =
		CreateBottomLevelAS({ {m_app->m_drawableObjects[2]->getVertexBuffer().Get(), m_app->m_drawableObjects[2]->getVertexCount()} }, 
			{ {m_app->m_drawableObjects[2]->getIndexBuffer().Get(), m_app->m_drawableObjects[2]->getIndexCount()}});

	AccelerationStructureBuffers bottomLevelDragonBuffers =
		CreateBottomLevelAS({ {m_app->m_drawableObjects[3]->getVertexBuffer().Get(), m_app->m_drawableObjects[3]->getVertexCount()} }, 
			{ {m_app->m_drawableObjects[3]->getIndexBuffer().Get(), m_app->m_drawableObjects[3]->getIndexCount()}});

	AccelerationStructureBuffers bottomLevelDragonBuffersCpy =
		CreateBottomLevelAS({ {m_app->m_drawableObjects[4]->getVertexBuffer().Get(), m_app->m_drawableObjects[4]->getVertexCount()} }, 
			{ {m_app->m_drawableObjects[4]->getIndexBuffer().Get(), m_app->m_drawableObjects[4]->getIndexCount()}});

	m_app->m_instances.push_back(std::make_pair(bottomLevelBunny.pResult, m_app->m_drawableObjects[0]->getTransform()));
	m_app->m_instances.push_back(std::make_pair(bottomLevelBunnyCpyBuffers.pResult, m_app->m_drawableObjects[1]->getTransform()));
	m_app->m_instances.push_back(std::make_pair(bottomLevelPlaneBuffers.pResult, m_app->m_drawableObjects[2]->getTransform()));
	m_app->m_instances.push_back(std::make_pair(bottomLevelDragonBuffers.pResult, m_app->m_drawableObjects[3]->getTransform()));
	m_app->m_instances.push_back(std::make_pair(bottomLevelDragonBuffersCpy.pResult, m_app->m_drawableObjects[4]->getTransform()));
	CreateTopLevelAS(m_app->m_instances, false);

	// Flush the command list and wait for it to finish
	context->m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { context->m_commandList.Get() };
	context->m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
	context->m_fenceValue++;
	context->m_commandQueue->Signal(context->m_fence.Get(), context->m_fenceValue);

	context->m_fence->SetEventOnCompletion(context->m_fenceValue, context->m_fenceEvent);
	WaitForSingleObject(context->m_fenceEvent, INFINITE);

	// Once the command list is finished executing, reset it to be reused for
	// rendering
	ThrowIfFailed(
		context->m_commandList->Reset(context->m_commandAllocator.Get(), nullptr));

	// Store the AS buffers. The rest of the buffers will be released once we exit
	// the function
	context->m_bottomLevelAS = bottomLevelPlaneBuffers.pResult;
}

//-----------------------------------------------------------------------------
// The ray generation shader needs to access 2 resources: the raytracing output
// and the top-level acceleration structure
//

ComPtr<ID3D12RootSignature> DXRSetup::CreateRayGenSignature() {
	nv_helpers_dx12::RootSignatureGenerator rsc;
	rsc.AddHeapRangesParameter(
		{ {0 /*u0*/, 1 /*1 descriptor */, 0 /*use the implicit register space 0*/,
		  D3D12_DESCRIPTOR_RANGE_TYPE_UAV /* UAV representing the output buffer*/,
		  0 /*heap slot where the UAV is defined*/},
		 {0 /*t0*/, 1, 0,
		  D3D12_DESCRIPTOR_RANGE_TYPE_SRV /*Top-level acceleration structure*/,
		  1},
		 {0 /*b0*/, 1, 0,
		  D3D12_DESCRIPTOR_RANGE_TYPE_CBV /* Constant buffer view */,
		  2}
		}

		);

	return rsc.Generate(m_device.Get(), true);
}

//-----------------------------------------------------------------------------
// The hit shader communicates only through the ray payload, and therefore does
// not require any resources
//
ComPtr<ID3D12RootSignature> DXRSetup::CreateHitSignature() {
	nv_helpers_dx12::RootSignatureGenerator rsc;
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 0 /*t0*/); // vertex data
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 1 /*t1*/); // indices
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 0 /*b0*/); // Lighting
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 1 /*b1*/); // Debug

	rsc.AddHeapRangesParameter({ { 2 /*t2*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1 /*2nd slot of the heap (see CreateShaderResourceHeap() */ } }); /*Top-level acceleration structure*/
	rsc.AddHeapRangesParameter({ { 3 /*t3*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3 /*4th slot of the heap (see CreateShaderResourceHeap() */ } }); /*Texture*/
	rsc.AddHeapRangesParameter({ { 4 /*t4*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4 /*5th slot of the heap (see CreateShaderResourceHeap() */ } }); /*Texture*/
	rsc.AddHeapRangesParameter({ { 5 /*t5*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5 /*6th slot of the heap (see CreateShaderResourceHeap() */ } }); /*Texture*/
	rsc.AddHeapRangesParameter({ { 6 /*t5*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6 /*7th slot of the heap (see CreateShaderResourceHeap() */ } }); /*Texture*/
	rsc.AddHeapRangesParameter({ { 7 /*t5*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7 /*8th slot of the heap (see CreateShaderResourceHeap() */ } }); /*Texture*/
	rsc.AddHeapRangesParameter({ { 8 /*t5*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8 /*9th slot of the heap (see CreateShaderResourceHeap() */ } }); /*Texture*/

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	return rsc.Generate(m_device.Get(), true, 1, &sampler);
}

//-----------------------------------------------------------------------------
// The miss shader communicates only through the ray payload, and therefore
// does not require any resources
//
ComPtr<ID3D12RootSignature> DXRSetup::CreateMissSignature() {
	nv_helpers_dx12::RootSignatureGenerator rsc;
	return rsc.Generate(m_device.Get(), true);
}

//-----------------------------------------------------------------------------
//
// The raytracing pipeline binds the shader code, root signatures and pipeline
// characteristics in a single structure used by DXR to invoke the shaders and
// manage temporary memory during raytracing
//
//
void DXRSetup::CreateRaytracingPipeline() 
{
	DXRContext* context = m_app->GetContext();

	nv_helpers_dx12::RayTracingPipelineGenerator pipeline(m_device.Get());

	// The pipeline contains the DXIL code of all the shaders potentially executed
	// during the raytracing process. This section compiles the HLSL code into a
	// set of DXIL libraries. We chose to separate the code in several libraries
	// by semantic (ray generation, hit, miss) for clarity. Any code layout can be
	// used.
	context->m_rayGenLibrary = nv_helpers_dx12::CompileShaderLibrary(L"RayGen.hlsl");
	context->m_missLibrary = nv_helpers_dx12::CompileShaderLibrary(L"Miss.hlsl");
	context->m_hitLibrary = nv_helpers_dx12::CompileShaderLibrary(L"Hit.hlsl");
	context->m_shadowLibrary = nv_helpers_dx12::CompileShaderLibrary(L"Shadow.hlsl");

	// In a way similar to DLLs, each library is associated with a number of
	// exported symbols. This
	// has to be done explicitly in the lines below. Note that a single library
	// can contain an arbitrary number of symbols, whose semantic is given in HLSL
	// using the [shader("xxx")] syntax
	pipeline.AddLibrary(context->m_rayGenLibrary.Get(), { L"RayGen" });
	pipeline.AddLibrary(context->m_missLibrary.Get(), { L"Miss"});
	pipeline.AddLibrary(context->m_hitLibrary.Get(), { L"ClosestHit", L"DragonClosestHit", L"PlaneClosestHit"});
	pipeline.AddLibrary(context->m_shadowLibrary.Get(), { L"ShadowMiss", L"ShadowClosestHit" });

	// To be used, each DX12 shader needs a root signature defining which
	// parameters and buffers will be accessed.
	context->m_rayGenSignature = CreateRayGenSignature();
	context->m_missSignature = CreateMissSignature();
	context->m_hitSignature = CreateHitSignature();

	// 3 different shaders can be invoked to obtain an intersection: an
	// intersection shader is called
	// when hitting the bounding box of non-triangular geometry. This is beyond
	// the scope of this tutorial. An any-hit shader is called on potential
	// intersections. This shader can, for example, perform alpha-testing and
	// discard some intersections. Finally, the closest-hit program is invoked on
	// the intersection point closest to the ray origin. Those 3 shaders are bound
	// together into a hit group.

	// Note that for triangular geometry the intersection shader is built-in. An
	// empty any-hit shader is also defined by default, so in our simple case each
	// hit group contains only the closest hit shader. Note that since the
	// exported symbols are defined above the shaders can be simply referred to by
	// name.

	// Hit group for the triangles, with a shader simply interpolating vertex
	// colors
	pipeline.AddHitGroup(L"HitGroup", L"ClosestHit");
	pipeline.AddHitGroup(L"DragonHitGroup", L"DragonClosestHit");
	pipeline.AddHitGroup(L"PlaneHitGroup", L"PlaneClosestHit");
	pipeline.AddHitGroup(L"ShadowHitGroup", L"ShadowClosestHit");

	// The following section associates the root signature to each shader. Note
	// that we can explicitly show that some shaders share the same root signature
	// (eg. Miss and ShadowMiss). Note that the hit shaders are now only referred
	// to as hit groups, meaning that the underlying intersection, any-hit and
	// closest-hit shaders share the same root signature.
	pipeline.AddRootSignatureAssociation(context->m_rayGenSignature.Get(), { L"RayGen" });
	pipeline.AddRootSignatureAssociation(context->m_missSignature.Get(), { L"Miss" });
	pipeline.AddRootSignatureAssociation(context->m_hitSignature.Get(), { L"HitGroup", L"DragonHitGroup", L"PlaneHitGroup" });

	// The payload size defines the maximum size of the data carried by the rays,
	// ie. the the data
	// exchanged between shaders, such as the HitInfo structure in the HLSL code.
	// It is important to keep this value as low as possible as a too high value
	// would result in unnecessary memory consumption and cache trashing.
	pipeline.SetMaxPayloadSize(4 * sizeof(float) + sizeof(unsigned int)); // RGB + distance

	// Upon hitting a surface, DXR can provide several attributes to the hit. In
	// our sample we just use the barycentric coordinates defined by the weights
	// u,v of the last two vertices of the triangle. The actual barycentrics can
	// be obtained using float3 barycentrics = float3(1.f-u-v, u, v);
	pipeline.SetMaxAttributeSize(2 * sizeof(float)); // barycentric coordinates

	// The raytracing process can shoot rays from existing hit points, resulting
	// in nested TraceRay calls. Our sample code traces only primary rays, which
	// then requires a trace depth of 1. Note that this recursion depth should be
	// kept to a minimum for best performance. Path tracing algorithms can be
	// easily flattened into a simple loop in the ray generation.
	pipeline.SetMaxRecursionDepth(8);

	// Compile the pipeline for execution on the GPU
	context->m_rtStateObject = pipeline.Generate();

	// Cast the state object into a properties object, allowing to later access
	// the shader pointers by name
	ThrowIfFailed(context->m_rtStateObject->QueryInterface(IID_PPV_ARGS(&context->m_rtStateObjectProps)));
}

//-----------------------------------------------------------------------------
//
// Allocate the buffer holding the raytracing output, with the same size as the
// output image
//
void DXRSetup::CreateRaytracingOutputBuffer() 
{
	DXRContext* context = m_app->GetContext();

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	// The backbuffer is actually DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, but sRGB
	// formats cannot be used with UAVs. For accuracy we should convert to sRGB
	// ourselves in the shader
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resDesc.Width = m_app->GetWidth();
	resDesc.Height = m_app->GetHeight();
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	ThrowIfFailed(m_device->CreateCommittedResource(
		&nv_helpers_dx12::kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr,
		IID_PPV_ARGS(&context->m_outputResource)));
}

//-----------------------------------------------------------------------------
//
// Create the main heap used by the shaders, which will give access to the
// raytracing output and the top-level acceleration structure
//
void DXRSetup::CreateShaderResourceHeap() 
{
	DXRContext* context = m_app->GetContext();

	constexpr int numOfDescHeaps = 9;

	// Create a SRV/UAV/CBV descriptor heap. We need 2 entries - 1 UAV for the
	// raytracing output and 1 SRV for the TLAS
	context->m_srvUavHeap = nv_helpers_dx12::CreateDescriptorHeap(
		m_device.Get(), numOfDescHeaps, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	// Get a handle to the heap memory on the CPU side, to be able to write the
	// descriptors directly
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle =
		context->m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();

	// Create the UAV. Based on the root signature we created it is the first
	// entry. The Create*View methods write the view information directly into
	// srvHandle
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	m_device->CreateUnorderedAccessView(context->m_outputResource.Get(), nullptr, &uavDesc,
		srvHandle);

	// Add the Top Level AS SRV right after the raytracing output buffer
	srvHandle.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location =
		context->m_topLevelASBuffers.pResult->GetGPUVirtualAddress();
	// Write the acceleration structure view in the heap
	m_device->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);

	// Add the camera to the descriptor heap.
	srvHandle.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = context->m_cameraBuffer.Get()->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = context->m_cameraBufferSize;
	m_device->CreateConstantBufferView(&cbvDesc, srvHandle);

	// 4a. Add the texture shader resource view after the Camera (increment the handle so it is after the constant buffer view)
	srvHandle.ptr += m_device -> GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CreateTextureUploadHeap(srvHandle, context, 0);

	srvHandle.ptr += m_device -> GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CreateTextureUploadHeap(srvHandle, context, 1);

	srvHandle.ptr += m_device -> GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CreateTextureUploadHeap(srvHandle, context, 2);

	srvHandle.ptr += m_device -> GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CreateTextureUploadHeap(srvHandle, context, 3);

	srvHandle.ptr += m_device -> GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CreateTextureUploadHeap(srvHandle, context, 4);

	srvHandle.ptr += m_device -> GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CreateTextureUploadHeap(srvHandle, context, 5);
}

//-----------------------------------------------------------------------------
//
// The Shader Binding Table (SBT) is the cornerstone of the raytracing setup:
// this is where the shader resources are bound to the shaders, in a way that
// can be interpreted by the raytracer on GPU. In terms of layout, the SBT
// contains a series of shader IDs with their resource pointers. The SBT
// contains the ray generation shader, the miss shaders, then the hit groups.
// Using the helper class, those can be specified in arbitrary order.
//
void DXRSetup::CreateShaderBindingTable() 
{
	DXRContext* context = m_app->GetContext();

	// The SBT helper class collects calls to Add*Program.  If called several
	// times, the helper must be emptied before re-adding shaders.
	context->m_sbtHelper.Reset();

	// The pointer to the beginning of the heap is the only parameter required by
	// shaders without root parameters
	D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle =
		context->m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();

	// The helper treats both root parameter pointers and heap pointers as void*,
	// while DX12 uses the
	// D3D12_GPU_DESCRIPTOR_HANDLE to define heap pointers. The pointer in this
	// struct is a UINT64, which then has to be reinterpreted as a pointer.
	auto heapPointer = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);

	// The ray generation only uses heap data
	context->m_sbtHelper.AddRayGenerationProgram(L"RayGen", { heapPointer });

	// The miss and hit shaders do not access any external resources: instead they
	// communicate their results through the ray payload
	context->m_sbtHelper.AddMissProgram(L"Miss", {});
	context->m_sbtHelper.AddMissProgram(L"ShadowMiss", {});

	// Adding the triangle hit shader
	context->m_sbtHelper.AddHitGroup(L"HitGroup",
		{ (void*)(m_app->m_drawableObjects[0]->getVertexBuffer()->GetGPUVirtualAddress()),
			(void*)(m_app->m_drawableObjects[0]->getIndexBuffer()->GetGPUVirtualAddress()),
			(void*)(m_app->GetContext()->m_lightBuffer.Get()->GetGPUVirtualAddress()),
			(void*)(m_app->GetContext()->m_debugBuffer.Get()->GetGPUVirtualAddress()),
			heapPointer,	
			heapPointer,	
			heapPointer,	
			heapPointer,	
			heapPointer,	
			heapPointer,	
			heapPointer
		});

	// Adding Shadow Hit Group.
	context->m_sbtHelper.AddHitGroup(L"ShadowHitGroup", {});

	// Adding the plane hit shader
	context->m_sbtHelper.AddHitGroup(L"PlaneHitGroup",
		{ (void*)(m_app->m_drawableObjects[2]->getVertexBuffer()->GetGPUVirtualAddress()),
			(void*)(m_app->m_drawableObjects[2]->getIndexBuffer()->GetGPUVirtualAddress()),
			(void*)(m_app->GetContext()->m_lightBuffer.Get()->GetGPUVirtualAddress()),
			(void*)(m_app->GetContext()->m_debugBuffer.Get()->GetGPUVirtualAddress()),
			heapPointer,
			heapPointer,
			heapPointer,
			heapPointer,
			heapPointer,
			heapPointer,
			heapPointer
		});

	// Adding the plane shadow hit shader.
	context->m_sbtHelper.AddHitGroup(L"ShadowHitGroup", { });

	context->m_sbtHelper.AddHitGroup(L"DragonHitGroup",
		{ (void*)(m_app->m_drawableObjects[3]->getVertexBuffer()->GetGPUVirtualAddress()),
			(void*)(m_app->m_drawableObjects[3]->getIndexBuffer()->GetGPUVirtualAddress()),
			(void*)(m_app->GetContext()->m_lightBuffer.Get()->GetGPUVirtualAddress()),
			(void*)(m_app->GetContext()->m_debugBuffer.Get()->GetGPUVirtualAddress()),
			heapPointer,
			heapPointer,
			heapPointer,
			heapPointer,
			heapPointer,
			heapPointer,
			heapPointer
		});

	// Adding Shadow Hit Group.
	context->m_sbtHelper.AddHitGroup(L"ShadowHitGroup", {});


	// Compute the size of the SBT given the number of shaders and their
	// parameters
	uint32_t sbtSize = context->m_sbtHelper.ComputeSBTSize();

	// Create the SBT on the upload heap. This is required as the helper will use
	// mapping to write the SBT contents. After the SBT compilation it could be
	// copied to the default heap for performance.
	context->m_sbtStorage = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), sbtSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
	if (!context->m_sbtStorage) {
		throw std::logic_error("Could not allocate the shader binding table");
	}
	// Compile the SBT from the shader and parameters info
	context->m_sbtHelper.Generate(context->m_sbtStorage.Get(), context->m_rtStateObjectProps.Get());
}

void DXRSetup::CreateCamera(XMFLOAT3 eye, XMFLOAT3 lookAt, XMFLOAT3 up)
{
	DXRContext* context = m_app->GetContext();

	context->m_camera = make_unique<DebugCamera>(eye, lookAt, up);
	context->m_camera.get()->Initialise(0.0f, 0.0f, 0.5f, 5.0f);

	context->m_cameraBufferSize = 256;
	context->m_cameraBuffer = nv_helpers_dx12::CreateBuffer(m_device.Get(), 
		context->m_cameraBufferSize, 
		D3D12_RESOURCE_FLAG_NONE, 
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		nv_helpers_dx12::kUploadHeapProps);
}

void DXRSetup::UpdateCameraBuffer()
{
	DXRContext* context = m_app->GetContext();

	std::vector<XMMATRIX> matrices(4);

	float fovAngleY = 45.0f * XM_PI / 180.0f;
	XMMATRIX perspectiveMat = XMMatrixPerspectiveFovRH(fovAngleY, m_app->m_aspectRatio, 0.1f, 1000.0f);

	// Set 1st and 2nd index to view and perps matrices respectively/ 
	matrices[0] = XMMatrixInverse(nullptr, context->m_camera.get()->GetView());
	matrices[1] = XMMatrixInverse(nullptr, perspectiveMat);

	// Copy data to cb.
	uint8_t* pData;
	ThrowIfFailed(context->m_cameraBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, matrices.data(), context->m_cameraBufferSize);
	context->m_cameraBuffer->Unmap(0, nullptr);
}

void DXRSetup::CreateLightBuffer()
{
	DXRContext* context = m_app->GetContext();

	context->m_lightBufferSize = 256;
	context->m_lightBuffer = nv_helpers_dx12::CreateBuffer(m_device.Get(),
		context->m_lightBufferSize,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nv_helpers_dx12::kUploadHeapProps);

	// Maybe I should make a function that returns a light buffer;
	LightBuffer lb = {
		m_app->m_lightVector[0]->m_position,
		m_app->m_lightVector[0]->m_ambientColour,
		m_app->m_lightVector[0]->m_diffuseColour,
		m_app->m_lightVector[0]->m_specularColour,
		m_app->m_lightVector[0]->m_shininess,
		m_app->m_lightVector[0]->m_attenuationRadius
	};

	// Copy data to cb.
	uint8_t* pData;
	ThrowIfFailed(context->m_lightBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, &lb, context->m_lightBufferSize);
	context->m_lightBuffer->Unmap(0, nullptr);
}

void DXRSetup::UpdateLightBuffer()
{
	DXRContext* context = m_app->GetContext();

	// Maybe I should make a function that returns a light buffer;
	LightBuffer lb = {
		m_app->m_lightVector[0]->m_position,
		m_app->m_lightVector[0]->m_ambientColour,
		m_app->m_lightVector[0]->m_diffuseColour,
		m_app->m_lightVector[0]->m_specularColour,
		m_app->m_lightVector[0]->m_shininess,
		m_app->m_lightVector[0]->m_attenuationRadius
	};

	// Copy data to cb.
	uint8_t* pData;
	ThrowIfFailed(context->m_lightBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, &lb, context->m_lightBufferSize);
	context->m_lightBuffer->Unmap(0, nullptr);
}

void DXRSetup::CreateDebugBuffer()
{
	DXRContext* context = m_app->GetContext();

	context->m_debugBufferSize = 256;
	context->m_debugBuffer = nv_helpers_dx12::CreateBuffer(m_device.Get(),
		context->m_debugBufferSize,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nv_helpers_dx12::kUploadHeapProps);

	DebugBuffer db = { m_app->m_shadowSampleCount, m_app->m_materialAlbedo, m_app->m_materialRoughness, m_app->m_materialMetalness};

	// Copy data to cb.
	uint8_t* pData;
	ThrowIfFailed(context->m_debugBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, &db, context->m_debugBufferSize);
	context->m_debugBuffer->Unmap(0, nullptr);
}

void DXRSetup::UpdateDebugBuffer()
{
	DXRContext* context = m_app->GetContext();

	DebugBuffer db = { m_app->m_shadowSampleCount, m_app->m_materialAlbedo, m_app->m_materialRoughness, m_app->m_materialMetalness};

	// Copy data to cb.
	uint8_t* pData;
	ThrowIfFailed(context->m_debugBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, &db, context->m_debugBufferSize);
	context->m_debugBuffer->Unmap(0, nullptr);
}

void DXRSetup::LoadTextureFromPath(LPCWSTR path, DXRContext* context, int index)
{
	TextureLoader tl;
	int imageBytesPerRow;
	BYTE* imageData;
	int imageSize = tl.LoadImageDataFromFile(&imageData, context->m_textureDesc, path,
		imageBytesPerRow);
	// make sure we have data
	if (imageSize <= 0)
	{
		// Texture failed to load.
		assert(0);
	}
	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&context->m_textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&context->m_texArray[index])));

	// CREATE AN UPLOAD HEAP
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(context->m_texArray[index].Get(), 0, 1);
	// Create the GPU upload buffer.
	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&context->m_texUploadHeapArray[index])));

	// SCHEDULE A COPY FROM THE UPLOAD HEAP TO THE DEFAULT HEAP TEXTURE
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = imageData;
	textureData.RowPitch = imageBytesPerRow;
	textureData.SlicePitch = textureData.RowPitch * context->m_textureDesc.Height;
	UpdateSubresources(context->m_commandList.Get(), context->m_texArray[index].Get(), context->m_texUploadHeapArray[index].Get(), 0, 0, 1, &textureData);
	context->m_commandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(context->m_texArray[index].Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

void DXRSetup::CreateTextureUploadHeap(D3D12_CPU_DESCRIPTOR_HANDLE srvHandle, DXRContext* context, int index)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDescTex = {};
	srvDescTex.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDescTex.Format = context->m_textureDesc.Format;
	srvDescTex.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDescTex.Texture2D.MipLevels = 1;
	m_device->CreateShaderResourceView(context->m_texArray[index].Get(), &srvDescTex, srvHandle);
}

//-----------------------------------------------------------------------------
//
// Create a bottom-level acceleration structure based on a list of vertex
// buffers in GPU memory along with their vertex count. The build is then done
// in 3 steps: gathering the geometry, computing the sizes of the required
// buffers, and building the actual AS
//
AccelerationStructureBuffers DXRSetup::CreateBottomLevelAS(std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers,
	std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers)
{
	DXRContext* context = m_app->GetContext();
	nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS;

	// Adding all vertex buffers and not transforming their position.
	// Adding all vertex buffers and not transforming their position.
	for (size_t i = 0; i < vVertexBuffers.size(); i++) {

		if (i < vIndexBuffers.size() && vIndexBuffers[i].second > 0)
		{
			bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0,
				vVertexBuffers[i].second, sizeof(Vertex),
				vIndexBuffers[i].first.Get(), 0,
				vIndexBuffers[i].second, nullptr, 0, true);
		}
		else
		{
			bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0, vVertexBuffers[i].second,
				sizeof(Vertex), 0, 0);
		}
	}

	// The AS build requires some scratch space to store temporary information.
	// The amount of scratch memory is dependent on the scene complexity.
	UINT64 scratchSizeInBytes = 0;
	// The final AS also needs to be stored in addition to the existing vertex
	// buffers. It size is also dependent on the scene complexity.
	UINT64 resultSizeInBytes = 0;

	bottomLevelAS.ComputeASBufferSizes(m_device.Get(), false, &scratchSizeInBytes,
		&resultSizeInBytes);

	// Once the sizes are obtained, the application is responsible for allocating
	// the necessary buffers. Since the entire generation will be done on the GPU,
	// we can directly allocate those on the default heap
	AccelerationStructureBuffers buffers;
	buffers.pScratch = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), scratchSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
		nv_helpers_dx12::kDefaultHeapProps);
	buffers.pResult = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), resultSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nv_helpers_dx12::kDefaultHeapProps);

	// Build the acceleration structure. Note that this call integrates a barrier
	// on the generated AS, so that it can be used to compute a top-level AS right
	// after this method.
	bottomLevelAS.Generate(context->m_commandList.Get(), buffers.pScratch.Get(),
		buffers.pResult.Get(), false, nullptr);

	return buffers;
}

//-----------------------------------------------------------------------------
// Create the main acceleration structure that holds all instances of the scene.
// Similarly to the bottom-level AS generation, it is done in 3 steps: gathering
// the instances, computing the memory requirements for the AS, and building the
// AS itself
//
void DXRSetup::CreateTopLevelAS(
	const std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>
	& instances, // pair of bottom level AS and matrix of the instance
	bool updateDirtyFlag
) 
{

	DXRContext* context = m_app->GetContext();

	if (updateDirtyFlag)
	{
		context->m_topLevelASGenerator.RemoveAllInstances();
	}

		context->m_topLevelASGenerator.AddInstance(instances[0].first.Get(),
			instances[0].second, static_cast<UINT>(0),
			static_cast<UINT>(0));
		context->m_topLevelASGenerator.AddInstance(instances[1].first.Get(),
			instances[1].second, static_cast<UINT>(1),
			static_cast<UINT>(0));
		context->m_topLevelASGenerator.AddInstance(instances[2].first.Get(),
			instances[2].second, static_cast<UINT>(2),
			static_cast<UINT>(2));
		context->m_topLevelASGenerator.AddInstance(instances[3].first.Get(),
			instances[3].second, static_cast<UINT>(3),
			static_cast<UINT>(4));
		context->m_topLevelASGenerator.AddInstance(instances[4].first.Get(),
			instances[4].second, static_cast<UINT>(4),
			static_cast<UINT>(4));


	
	if (!updateDirtyFlag)
	{
		// Gather all the instances into the builder helper
		//for (size_t i = 0; i < instances.size(); i++) {
		//}
		// 
			//context->m_topLevelASGenerator.AddInstance(instances[2].first.Get(),
			//	instances[2].second, static_cast<UINT>(2),
			//	static_cast<UINT>(1));

		// As for the bottom-level AS, the building the AS requires some scratch space
		// to store temporary data in addition to the actual AS. In the case of the
		// top-level AS, the instance descriptors also need to be stored in GPU
		// memory. This call outputs the memory requirements for each (scratch,
		// results, instance descriptors) so that the application can allocate the
		// corresponding memory
		UINT64 scratchSize, resultSize, instanceDescsSize;

		context->m_topLevelASGenerator.ComputeASBufferSizes(m_device.Get(), true, &scratchSize,
			&resultSize, &instanceDescsSize);

		// Create the scratch and result buffers. Since the build is all done on GPU,
		// those can be allocated on the default heap
		context->m_topLevelASBuffers.pScratch = nv_helpers_dx12::CreateBuffer(
			m_device.Get(), scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nv_helpers_dx12::kDefaultHeapProps);

		context->m_topLevelASBuffers.pResult = nv_helpers_dx12::CreateBuffer(
			m_device.Get(), resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			nv_helpers_dx12::kDefaultHeapProps);

		// The buffer describing the instances: ID, shader binding information,
		// matrices ... Those will be copied into the buffer by the helper through
		// mapping, so the buffer has to be allocated on the upload heap.
		context->m_topLevelASBuffers.pInstanceDesc = nv_helpers_dx12::CreateBuffer(
			m_device.Get(), instanceDescsSize, D3D12_RESOURCE_FLAG_NONE,
			D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
	}


	// After all the buffers are allocated, or if only an update is required, we
	// can build the acceleration structure. Note that in the case of the update
	// we also pass the existing AS as the 'previous' AS, so that it can be
	// refitted in place.
	context->m_topLevelASGenerator.Generate(context->m_commandList.Get(),
		context->m_topLevelASBuffers.pScratch.Get(),
		context->m_topLevelASBuffers.pResult.Get(),
		context->m_topLevelASBuffers.pInstanceDesc.Get());
}