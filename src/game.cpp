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
#include <test/game.hpp>

#include <GLFW/glfw3native.h>

#include <array>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

#define GLFWHANDLE (std::bit_cast<GLFWwindow*>(_handle))

// SHADERS -------
static const char* VSSource = R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

void main(in  uint    VertId : SV_VertexID,
          out PSInput PSIn)
{
    float4 Pos[3];
    Pos[0] = float4(-0.5, -0.5, 0.0, 1.0);
    Pos[1] = float4( 0.0, +0.5, 0.0, 1.0);
    Pos[2] = float4(+0.5, -0.5, 0.0, 1.0);

    float3 Col[3];
    Col[0] = float3(1.0, 0.0, 0.0); // red
    Col[1] = float3(0.0, 1.0, 0.0); // green
    Col[2] = float3(0.0, 0.0, 1.0); // blue

    PSIn.Pos   = Pos[VertId];
    PSIn.Color = Col[VertId];
}
)";

// Pixel shader simply outputs interpolated vertex color
static const char* PSSource = R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

struct PSOutput
{
    float4 Color : SV_TARGET;
};

void main(in  PSInput  PSIn,
          out PSOutput PSOut)
{
    PSOut.Color = float4(PSIn.Color.rgb, 1.0);
}
)";
// ----------------------------

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
		if (devType == RENDER_DEVICE_TYPE_GL) {
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
		if (type == RENDER_DEVICE_TYPE_GL)
			glfwMakeContextCurrent(GLFWHANDLE);
#endif

#if PLATFORM_MACOS
		Diligent::MacOSNativeWindow Window;
		if (type == RENDER_DEVICE_TYPE_GL)
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
					auto* GetEngineFactoryD3D11 = Diligent::LoadGraphicsEngineVk(); // Load the dll and import GetEngineFactoryD3D11() function
	#endif
					auto* pFactoryD3D11 = GetEngineFactoryD3D11();

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
	#endif
					auto* pFactoryOpenGL = GetEngineFactoryOpenGL();

					Diligent::EngineGLCreateInfo EngineCI;
					EngineCI.Window = Window;
					pFactoryOpenGL->CreateDeviceAndSwapChainGL(EngineCI, &this->_pDevice, &this->_pImmediateContext, SCDesc, &this->_pSwapChain);
				}
				break;
#endif // GL_SUPPORTED

#if VULKAN_SUPPORTED
			case Diligent::RENDER_DEVICE_TYPE_VULKAN:
				{
	#if EXPLICITLY_LOAD_ENGINE_VK_DLL
					// Load the dll and import GetEngineFactoryVk() function
					auto* GetEngineFactoryVk = Diligent::LoadGraphicsEngineVk();
	#endif
					auto* pFactoryVk = GetEngineFactoryVk();

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
		PSOCreateInfo.PSODesc.Name = "Simple triangle PSO";

		// This is a graphics pipeline
		PSOCreateInfo.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;

		// This tutorial will render to a single render target
		PSOCreateInfo.GraphicsPipeline.NumRenderTargets = 1;
		// Set render target format which is the format of the swap chain's color buffer
		PSOCreateInfo.GraphicsPipeline.RTVFormats[0] = this->_pSwapChain->GetDesc().ColorBufferFormat;
		// Use the depth buffer format from the swap chain
		PSOCreateInfo.GraphicsPipeline.DSVFormat = this->_pSwapChain->GetDesc().DepthBufferFormat;
		// Primitive topology defines what kind of primitives will be rendered by this pipeline state
		PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		// No back face culling for this tutorial
		PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
		// Disable depth testing
		PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;

		Diligent::ShaderCreateInfo ShaderCI;
		// Tell the system that the shader source code is in HLSL.
		// For OpenGL, the engine will convert this into GLSL under the hood.
		ShaderCI.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
		// OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
		ShaderCI.Desc.UseCombinedTextureSamplers = true;
		// Create a vertex shader
		Diligent::RefCntAutoPtr<Diligent::IShader> pVS;
		{
			ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
			ShaderCI.EntryPoint = "main";
			ShaderCI.Desc.Name = "Triangle vertex shader";
			ShaderCI.Source = VSSource;
			this->_pDevice->CreateShader(ShaderCI, &pVS);
		}

		// Create a pixel shader
		Diligent::RefCntAutoPtr<Diligent::IShader> pPS;
		{
			ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
			ShaderCI.EntryPoint = "main";
			ShaderCI.Desc.Name = "Triangle pixel shader";
			ShaderCI.Source = PSSource;
			this->_pDevice->CreateShader(ShaderCI, &pPS);
		}

		// Finally, create the pipeline state
		PSOCreateInfo.pVS = pVS;
		PSOCreateInfo.pPS = pPS;
		this->_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &this->_pPSO);

		this->_initialized = true;
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

			this->draw();
		}
	}

	void TestGame::draw() {
		if (!this->_initialized) return;

		// Clear the back buffer
		const std::array<float, 4> clearColor = {0.350F, 0.350F, 0.350F, 1.0F};

		// Let the engine perform required state transitions
		auto* pRTV = this->_pSwapChain->GetCurrentBackBufferRTV();
		auto* pDSV = this->_pSwapChain->GetDepthBufferDSV();
		this->_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		// Let the engine perform required state transitions
		this->_pImmediateContext->ClearRenderTarget(pRTV, clearColor.data(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		this->_pImmediateContext->ClearDepthStencil(pDSV, Diligent::CLEAR_DEPTH_FLAG, 1.F, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		// Set the pipeline state in the immediate context
		this->_pImmediateContext->SetPipelineState(this->_pPSO);

		// Typically we should now call CommitShaderResources(), however shaders in this example don't
		// use any resources.

		Diligent::DrawAttribs drawAttrs;
		drawAttrs.NumVertices = 3; // Render 3 vertices
		this->_pImmediateContext->Draw(drawAttrs);

		// RENDER ---
		this->_pSwapChain->Present();
	}

	void TestGame::shutdown() {
		this->_pImmediateContext->Flush();
	}
} // namespace test
