
#ifndef ENGINE_DLL
	#define ENGINE_DLL 1
#endif

#ifndef D3D11_SUPPORTED
	#define D3D11_SUPPORTED 0
#endif

#ifndef D3D12_SUPPORTED
	#define D3D12_SUPPORTED 0
#endif

#ifndef GL_SUPPORTED
	#define GL_SUPPORTED 0
#endif

#ifndef VULKAN_SUPPORTED
	#define VULKAN_SUPPORTED 0
#endif

#ifndef METAL_SUPPORTED
	#define METAL_SUPPORTED 0
#endif

#if D3D11_SUPPORTED
	#include <Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h>
#endif

#if D3D12_SUPPORTED
	#include <Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h>
#endif

#if GL_SUPPORTED
	#include <Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h>
#endif

#if VULKAN_SUPPORTED
	#include <Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h>
#endif

#if METAL_SUPPORTED
	#include <Graphics/GraphicsEngineMetal/interface/EngineFactoryMtl.h>
#endif

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if GLFW_VERSION_MINOR < 2
	#error "GLFW 3.2 or later is required"
#endif // GLFW_VERSION_MINOR < 2

#if PLATFORM_LINUX
	#if RAWRBOX_USE_WAYLAND
		#include <wayland-egl.h>
		#define GLFW_EXPOSE_NATIVE_WAYLAND
	#else
		#define GLFW_EXPOSE_NATIVE_X11
		#define GLFW_EXPOSE_NATIVE_GLX
	#endif
#elif PLATFORM_MACOS
	#define GLFW_EXPOSE_NATIVE_COCOA
	#define GLFW_EXPOSE_NATIVE_NSGL
#elif PLATFORM_WIN32
	#define GLFW_EXPOSE_NATIVE_WIN32
	#define GLFW_EXPOSE_NATIVE_WGL
#endif //

#include <Graphics/GraphicsTools/interface/MapHelper.hpp>
#include <Platforms/Basic/interface/DebugUtilities.hpp>
#include <test/game.hpp>

#include <GLFW/glfw3native.h>

#include <array>
#include <chrono>
#include <cmath>
#include <thread>

using namespace std::chrono_literals;

#define GLFWHANDLE (std::bit_cast<GLFWwindow*>(_handle))

namespace test {
	static TestGame& glfwHandleToRenderer(GLFWwindow* ptr) {
		return *static_cast<TestGame*>(glfwGetWindowUserPointer(ptr));
	}

	void TestGame::init(Diligent::RENDER_DEVICE_TYPE type) {
		Diligent::RENDER_DEVICE_TYPE devType = Diligent::RENDER_DEVICE_TYPE_UNDEFINED;
		std::string engine = "";

		// Select best renderer ----
		if (type == Diligent::RENDER_DEVICE_TYPE_UNDEFINED) {
#if PLATFORM_LINUX
	#if VULKAN_SUPPORTED
			devType = Diligent::RENDER_DEVICE_TYPE_VULKAN;
			engine = "VULKAN";
	#else
			devType = Diligent::RENDER_DEVICE_TYPE_GL;
			engine = "OPENGL";
	#endif
#else
	#if D3D12_SUPPORTED
			devType = Diligent::RENDER_DEVICE_TYPE_D3D12;
			engine = "D3D12";
	#elif D3D11_SUPPORTED
			devType = Diligent::RENDER_DEVICE_TYPE_D3D11;
			engine = "D3D11";
	#elif VULKAN_SUPPORTED
			devType = Diligent::RENDER_DEVICE_TYPE_VULKAN;
			engine = "VULKAN";
	#else
			devType = Diligent::RENDER_DEVICE_TYPE_GL;
			engine = "OPENGL";
	#endif
#endif
		}
		// ------

		int APIHint = GLFW_NO_API;
#if !PLATFORM_WIN32
		if (devType == Diligent::RENDER_DEVICE_TYPE_GL) {
			// On platforms other than Windows Diligent Engine
			// attaches to existing OpenGL context
			APIHint = GLFW_OPENGL_API;
		}
#endif

		this->createWindow(APIHint, "Test (" + engine + ")");
		this->createEngine(devType);
		this->initGame();
	}

	void TestGame::createWindow(int api, const std::string& title) {
		if (glfwInit() != GLFW_TRUE)
			throw std::runtime_error("Failed to initialize glfw");

		glfwWindowHint(GLFW_CLIENT_API, api);
		if (api == GLFW_OPENGL_API) {
			// We need compute shaders, so request OpenGL 4.2 at least
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
		}

		auto window = glfwCreateWindow(1280, 720, title.c_str(), nullptr, nullptr);
		if (window == nullptr) throw std::runtime_error("Failed to create window");

		this->_handle = window;

		glfwSetWindowUserPointer(window, this);
		glfwSetWindowSizeLimits(window, 320, 240, GLFW_DONT_CARE, GLFW_DONT_CARE);
		glfwSetWindowSizeCallback(GLFWHANDLE, callbacks_resize);
	}

	void TestGame::callbacks_resize(GLFWwindow* whandle, int width, int height) {
		auto& window = glfwHandleToRenderer(whandle);

		if (window._pSwapChain == nullptr) return;
		window._pSwapChain->Resize(width, height);
	}

	void TestGame::createEngine(Diligent::RENDER_DEVICE_TYPE type) {
#if PLATFORM_WIN32
		Diligent::Win32NativeWindow Window{glfwGetWin32Window(GLFWHANDLE)};
#endif

#if PLATFORM_LINUX
		Diligent::LinuxNativeWindow Window;
		Window.WindowId = glfwGetX11Window(GLFWHANDLE);
		Window.pDisplay = glfwGetX11Display();
		if (type == Diligent::RENDER_DEVICE_TYPE_GL)
			glfwMakeContextCurrent(GLFWHANDLE);
#endif

#if PLATFORM_MACOS
		Diligent::MacOSNativeWindow Window;
		if (type == Diligent::RENDER_DEVICE_TYPE_GL)
			glfwMakeContextCurrent(GLFWHANDLE);
		else
			Window.pNSView = GetNSWindowView(GLFWHANDLE);
#endif

		Diligent::SwapChainDesc SCDesc;

		switch (type) {
#if D3D11_SUPPORTED
			case Diligent::RENDER_DEVICE_TYPE_D3D11:
				{
	#if ENGINE_DLL
					auto* GetEngineFactoryD3D11 = Diligent::LoadGraphicsEngineD3D11(); // Load the dll and import GetEngineFactoryD3D11() function
	#endif
					auto* pFactoryD3D11 = GetEngineFactoryD3D11();
					this->_pEngineFactory = pFactoryD3D11;

					Diligent::EngineD3D11CreateInfo EngineCI;
					pFactoryD3D11->CreateDeviceAndContextsD3D11(EngineCI, &this->_pDevice, &this->_pImmediateContext);
					pFactoryD3D11->CreateSwapChainD3D11(this->_pDevice, this->_pImmediateContext, SCDesc, Diligent::FullScreenModeDesc{}, Window, &this->_pSwapChain);
				}
				break;
#endif

#if D3D12_SUPPORTED
			case Diligent::RENDER_DEVICE_TYPE_D3D12:
				{
	#if ENGINE_DLL
					// Load the dll and import GetEngineFactoryD3D12() function
					auto* GetEngineFactoryD3D12 = Diligent::LoadGraphicsEngineD3D12();
	#endif
					auto* pFactoryD3D12 = GetEngineFactoryD3D12();
					this->_pEngineFactory = pFactoryD3D12;

					Diligent::EngineD3D12CreateInfo EngineCI;
					pFactoryD3D12->CreateDeviceAndContextsD3D12(EngineCI, &this->_pDevice, &this->_pImmediateContext);
					pFactoryD3D12->CreateSwapChainD3D12(this->_pDevice, this->_pImmediateContext, SCDesc, Diligent::FullScreenModeDesc{}, Window, &this->_pSwapChain);
				}
				break;
#endif // D3D12_SUPPORTED

#if GL_SUPPORTED
			case Diligent::RENDER_DEVICE_TYPE_GL:
				{
	#if EXPLICITLY_LOAD_ENGINE_GL_DLL
					// Load the dll and import GetEngineFactoryOpenGL() function
					auto GetEngineFactoryOpenGL = Diligent::LoadGraphicsEngineOpenGL();
					auto* pFactoryOpenGL = GetEngineFactoryOpenGL();
	#else
					auto* pFactoryOpenGL = Diligent::GetEngineFactoryOpenGL();
	#endif
					this->_pEngineFactory = pFactoryOpenGL;

					Diligent::EngineGLCreateInfo EngineCI;
					EngineCI.Window = Window;
					pFactoryOpenGL->CreateDeviceAndSwapChainGL(EngineCI, &this->_pDevice, &this->_pImmediateContext, SCDesc, &this->_pSwapChain);
				}
				break;
#endif // GL_SUPPORTED

#if VULKAN_SUPPORTED
			case Diligent::RENDER_DEVICE_TYPE_VULKAN:
				{
	#if EXPLICITLY_LOAD_ENGINE_GL_DLL
					// Load the dll and import GetEngineFactoryVk() function
					auto* GetEngineFactoryVk = Diligent::LoadGraphicsEngineVk();
					auto* pFactoryVk = GetEngineFactoryVk();
	#else
					auto* pFactoryVk = Diligent::GetEngineFactoryVk();
	#endif
					this->_pEngineFactory = pFactoryVk;

					Diligent::EngineVkCreateInfo EngineCI;
					pFactoryVk->CreateDeviceAndContextsVk(EngineCI, &this->_pDevice, &this->_pImmediateContext);
					pFactoryVk->CreateSwapChainVk(this->_pDevice, this->_pImmediateContext, SCDesc, Window, &this->_pSwapChain);
				}
				break;
#endif // VULKAN_SUPPORTED
			default: throw std::runtime_error("Invalid engine");
		}

		if (this->_pDevice == nullptr || this->_pImmediateContext == nullptr || this->_pSwapChain == nullptr) throw std::runtime_error("Failed to initialize engine");
	}

	// -----------------------------------------------------------
	// -----------------------------------------------------------
	// -----------------------------------------------------------
	// -----------------------------------------------------------
	// -----------------------------------------------------------
	// -----------------------------------------------------------
	// -----------------------------------------------------------
	// -----------------------------------------------------------
	// -----------------------------------------------------------
	// -----------------------------------------------------------
	// -----------------------------------------------------------
	// -----------------------------------------------------------
	// -----------------------------------------------------------
	// -----------------------------------------------------------
	// -----------------------------------------------------------
	// -----------------------------------------------------------

	void TestGame::initGame() {
		// Pipeline state object encompasses configuration of all GPU stages

		Diligent::GraphicsPipelineStateCreateInfo PSOCreateInfo;

		// Pipeline state name is used by the engine to report issues.
		// It is always a good idea to give objects descriptive names.
		PSOCreateInfo.PSODesc.Name = "Cube PSO";

		// This is a graphics pipeline
		PSOCreateInfo.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;

		// clang-format off
    // This tutorial will render to a single render target
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    // Set render target format which is the format of the swap chain's color buffer
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = this->_pSwapChain->GetDesc().ColorBufferFormat;
    // Set depth buffer format which is the format of the swap chain's back buffer
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = this->_pSwapChain->GetDesc().DepthBufferFormat;
    // Primitive topology defines what kind of primitives will be rendered by this pipeline state
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // Cull back faces
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = Diligent::CULL_MODE_BACK;
    // Enable depth testing
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
		// clang-format on

		Diligent::ShaderCreateInfo ShaderCI;
		// Tell the system that the shader source code is in HLSL.
		// For OpenGL, the engine will convert this into GLSL under the hood.
		ShaderCI.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
		ShaderCI.CompileFlags = Diligent::SHADER_COMPILE_FLAG_ENABLE_UNBOUNDED_ARRAYS;
		ShaderCI.ShaderCompiler = Diligent::SHADER_COMPILER_DXC;
		ShaderCI.GLSLExtensions = "#extension GL_EXT_nonuniform_qualifier : require\n";

		// OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
		ShaderCI.Desc.UseCombinedTextureSamplers = true;

		// In this tutorial, we will load shaders from file. To be able to do that,
		// we need to create a shader source stream factory
		Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> pShaderSourceFactory;
		this->_pEngineFactory->CreateDefaultShaderSourceStreamFactory("assets", &pShaderSourceFactory);
		ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
		// Create a vertex shader
		Diligent::RefCntAutoPtr<Diligent::IShader> pVS;
		{
			ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
			ShaderCI.EntryPoint = "main";
			ShaderCI.Desc.Name = "Cube VS";
			ShaderCI.FilePath = "cube.vsh";
			this->_pDevice->CreateShader(ShaderCI, &pVS);
			// Create dynamic uniform buffer that will store our transformation matrix
			// Dynamic buffers can be frequently updated by the CPU
			Diligent::BufferDesc CBDesc;
			CBDesc.Name = "VS constants CB";
			CBDesc.Size = sizeof(Diligent::float4x4);
			CBDesc.Usage = Diligent::USAGE_DYNAMIC;
			CBDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
			CBDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
			this->_pDevice->CreateBuffer(CBDesc, nullptr, &this->_VSConstants);
		}

		{
			Diligent::BufferDesc CBDesc;
			CBDesc.Name = "Fake constant";
			CBDesc.Size = sizeof(FakeConstant);
			CBDesc.Usage = Diligent::USAGE_DYNAMIC;
			CBDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
			CBDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
			this->_pDevice->CreateBuffer(CBDesc, nullptr, &this->_PSConstants);
		}

		// Create a pixel shader
		Diligent::RefCntAutoPtr<Diligent::IShader> pPS;
		{
			ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
			ShaderCI.EntryPoint = "main";
			ShaderCI.Desc.Name = "Cube PS";
			ShaderCI.FilePath = "cube.psh";
			this->_pDevice->CreateShader(ShaderCI, &pPS);
		}

		// Define vertex shader input layout
		std::array<Diligent::LayoutElement, 2> LayoutElems =
		    {
			// Attribute 0 - vertex position
			Diligent::LayoutElement{0, 0, 3, Diligent::VT_FLOAT32, false},
			// Attribute 1 - vertex color
			Diligent::LayoutElement{1, 0, 4, Diligent::VT_FLOAT32, false}};

		PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems.data();
		PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements = static_cast<uint32_t>(LayoutElems.size());

		PSOCreateInfo.pVS = pVS;
		PSOCreateInfo.pPS = pPS;

		// Define variable type that will be used by default
		PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

		this->_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &this->_pPSO);

		// Since we did not explcitly specify the type for 'Constants' variable, default
		// type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables never
		// change and are bound directly through the pipeline state object.
		this->_pPSO->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "Constants")->Set(this->_VSConstants);
		this->_pPSO->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "Constants")->Set(this->_PSConstants);

		// Create a shader resource binding object and bind all static resources in it
		this->_pPSO->CreateShaderResourceBinding(&this->_pSRB, true);

		this->createCube();
	}

	void TestGame::createCube() {
		// Layout of this structure matches the one we defined in the pipeline state
		struct Vertex {
			Diligent::float3 pos;
			Diligent::float4 color;
		};

		// Cube vertices

		//      (-1,+1,+1)________________(+1,+1,+1)
		//               /|              /|
		//              / |             / |
		//             /  |            /  |
		//            /   |           /   |
		//(-1,-1,+1) /____|__________/(+1,-1,+1)
		//           |    |__________|____|
		//           |   /(-1,+1,-1) |    /(+1,+1,-1)
		//           |  /            |   /
		//           | /             |  /
		//           |/              | /
		//           /_______________|/
		//        (-1,-1,-1)       (+1,-1,-1)
		//

		constexpr Vertex CubeVerts[8] =
		    {
			{Diligent::float3{-1, -1, -1}, Diligent::float4{1, 0, 0, 1}},
			{Diligent::float3{-1, +1, -1}, Diligent::float4{0, 1, 0, 1}},
			{Diligent::float3{+1, +1, -1}, Diligent::float4{0, 0, 1, 1}},
			{Diligent::float3{+1, -1, -1}, Diligent::float4{1, 1, 1, 1}},

			{Diligent::float3{-1, -1, +1}, Diligent::float4{1, 1, 0, 1}},
			{Diligent::float3{-1, +1, +1}, Diligent::float4{0, 1, 1, 1}},
			{Diligent::float3{+1, +1, +1}, Diligent::float4{1, 0, 1, 1}},
			{Diligent::float3{+1, -1, +1}, Diligent::float4{0.2F, 0.2F, 0.2F, 1.F}},
		    };

		// Create a vertex buffer that stores cube vertices
		Diligent::BufferDesc VertBuffDesc;
		VertBuffDesc.Name = "Cube vertex buffer";
		VertBuffDesc.Usage = Diligent::USAGE_IMMUTABLE;
		VertBuffDesc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
		VertBuffDesc.Size = sizeof(CubeVerts);

		Diligent::BufferData VBData;
		VBData.pData = CubeVerts;
		VBData.DataSize = sizeof(CubeVerts);

		this->_pDevice->CreateBuffer(VertBuffDesc, &VBData, &this->_CubeVertexBuffer);

		// INDICES -----------------------
		constexpr uint32_t Indices[] =
		    {
			2, 0, 1, 2, 3, 0,
			4, 6, 5, 4, 7, 6,
			0, 7, 4, 0, 3, 7,
			1, 0, 4, 1, 4, 5,
			1, 5, 2, 5, 6, 2,
			3, 6, 7, 3, 2, 6};

		Diligent::BufferDesc IndBuffDesc;
		IndBuffDesc.Name = "Cube index buffer";
		IndBuffDesc.Usage = Diligent::USAGE_IMMUTABLE;
		IndBuffDesc.BindFlags = Diligent::BIND_INDEX_BUFFER;
		IndBuffDesc.Size = sizeof(Indices);

		Diligent::BufferData IBData;
		IBData.pData = Indices;
		IBData.DataSize = sizeof(Indices);
		this->_pDevice->CreateBuffer(IndBuffDesc, &IBData, &this->_CubeIndexBuffer);
		// -------------------------------

		this->_initialized = true;
	}

	Diligent::float4x4 TestGame::GetSurfacePretransformMatrix(const Diligent::float3& f3CameraViewAxis) const {
		const auto& SCDesc = this->_pSwapChain->GetDesc();

		switch (SCDesc.PreTransform) {
			case Diligent::SURFACE_TRANSFORM_ROTATE_90:
				// The image content is rotated 90 degrees clockwise.
				return Diligent::float4x4::RotationArbitrary(f3CameraViewAxis, -Diligent::PI_F / 2.F);

			case Diligent::SURFACE_TRANSFORM_ROTATE_180:
				// The image content is rotated 180 degrees clockwise.
				return Diligent::float4x4::RotationArbitrary(f3CameraViewAxis, -Diligent::PI_F);

			case Diligent::SURFACE_TRANSFORM_ROTATE_270:
				// The image content is rotated 270 degrees clockwise.
				return Diligent::float4x4::RotationArbitrary(f3CameraViewAxis, -Diligent::PI_F * 3.F / 2.F);

			case Diligent::SURFACE_TRANSFORM_OPTIMAL:
				UNEXPECTED("SURFACE_TRANSFORM_OPTIMAL is only valid as parameter during swap chain initialization.");
				return Diligent::float4x4::Identity();

			case Diligent::SURFACE_TRANSFORM_HORIZONTAL_MIRROR:
			case Diligent::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90:
			case Diligent::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180:
			case Diligent::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270:
				UNEXPECTED("Mirror transforms are not supported");
				return Diligent::float4x4::Identity();

			default:
				return Diligent::float4x4::Identity();
		}
	}

	Diligent::float4x4 TestGame::GetAdjustedProjectionMatrix(float FOV, float NearPlane, float FarPlane) const {
		const auto& SCDesc = this->_pSwapChain->GetDesc();

		float AspectRatio = static_cast<float>(SCDesc.Width) / static_cast<float>(SCDesc.Height);
		float XScale = 0, YScale = 0;

		if (SCDesc.PreTransform == Diligent::SURFACE_TRANSFORM_ROTATE_90 ||
		    SCDesc.PreTransform == Diligent::SURFACE_TRANSFORM_ROTATE_270 ||
		    SCDesc.PreTransform == Diligent::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90 ||
		    SCDesc.PreTransform == Diligent::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270) {
			// When the screen is rotated, vertical FOV becomes horizontal FOV
			XScale = 1.F / std::tan(FOV / 2.F);
			// Aspect ratio is inversed
			YScale = XScale * AspectRatio;
		} else {
			YScale = 1.F / std::tan(FOV / 2.F);
			XScale = YScale / AspectRatio;
		}

		Diligent::float4x4 Proj;
		Proj._11 = XScale;
		Proj._22 = YScale;
		Proj.SetNearFarClipPlanes(NearPlane, FarPlane, this->_pDevice->GetDeviceInfo().IsGLDevice());
		return Proj;
	}

	void TestGame::update() {
		this->_lastUpdate = TClock::now();

		for (;;) {
			if (glfwWindowShouldClose(GLFWHANDLE))
				return;

			glfwPollEvents();

			const auto time = TClock::now();
			const auto dt = std::chrono::duration_cast<TSeconds>(time - this->_lastUpdate).count();
			this->_lastUpdate = time;

			if (this->_initialized) {
				// Apply rotation
				Diligent::float4x4 CubeModelTransform = Diligent::float4x4::RotationY(this->_counter * 1.0F) * Diligent::float4x4::RotationX(-Diligent::PI_F * 0.1F);

				// Camera is at (0, 0, -5) looking along the Z axis
				Diligent::float4x4 View = Diligent::float4x4::Translation(0.F, 0.0F, 5.0F);
				// Get pretransform matrix that rotates the scene according the surface orientation
				auto SrfPreTransform = this->GetSurfacePretransformMatrix(Diligent::float3{0, 0, 1});

				// Get projection matrix adjusted to the current screen orientation
				auto Proj = GetAdjustedProjectionMatrix(Diligent::PI_F / 4.0F, 0.1F, 100.F);

				// Compute world-view-projection matrix
				this->_WorldViewProjMatrix = CubeModelTransform * View * SrfPreTransform * Proj;

				this->draw();

				this->_counter += 0.001F;
			}
		}
	}

	void TestGame::draw() {
		// Let the engine perform required state transitions
		auto* pRTV = this->_pSwapChain->GetCurrentBackBufferRTV();
		auto* pDSV = this->_pSwapChain->GetDepthBufferDSV();

		const std::array<float, 4> clearColor = {0.350F, 0.350F, 0.350F, 1.0F};

		this->_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		// Clear the back buffer
		this->_pImmediateContext->ClearRenderTarget(pRTV, clearColor.data(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		this->_pImmediateContext->ClearDepthStencil(pDSV, Diligent::CLEAR_DEPTH_FLAG, 1.F, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		{
			// Map the buffer and write current world-view-projection matrix
			Diligent::MapHelper<Diligent::float4x4> CBConstants(this->_pImmediateContext, this->_VSConstants, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
			*CBConstants = this->_WorldViewProjMatrix.Transpose();
		}

		{
			// Map the buffer and write current world-view-projection matrix
			Diligent::MapHelper<FakeConstant> CBConstants(this->_pImmediateContext, this->_PSConstants, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
			*CBConstants = {{1, 0, 0, 0}, {0.343F, 0, 0.45F, 0}};
		}

		// Bind vertex and index buffers
		const uint64_t offset = 0;
		Diligent::IBuffer* pBuffs[] = {this->_CubeVertexBuffer};
		this->_pImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
		this->_pImmediateContext->SetIndexBuffer(this->_CubeIndexBuffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		// Set the pipeline state in the immediate context
		this->_pImmediateContext->SetPipelineState(this->_pPSO);

		// Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
		// makes sure that resources are transitioned to required states.
		this->_pImmediateContext->CommitShaderResources(this->_pSRB, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		Diligent::DrawIndexedAttribs DrawAttrs;    // This is an indexed draw call
		DrawAttrs.IndexType = Diligent::VT_UINT32; // Index type
		DrawAttrs.NumIndices = 36;
		// Verify the state of vertex and index buffers
		DrawAttrs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
		this->_pImmediateContext->DrawIndexed(DrawAttrs);

		// RENDER ---
		this->_pSwapChain->Present();
	}

	void TestGame::shutdown() {
		this->_pImmediateContext->Flush();
	}
} // namespace test
