#pragma once

#ifndef PLATFORM_WIN32
	#define PLATFORM_WIN32 1
#endif

#ifndef ENGINE_DLL
	#define ENGINE_DLL 1
#endif

#ifndef D3D11_SUPPORTED
	#define D3D11_SUPPORTED 1
#endif

#ifndef D3D12_SUPPORTED
	#define D3D12_SUPPORTED 1
#endif

#ifndef GL_SUPPORTED
	#define GL_SUPPORTED 1
#endif

#ifndef VULKAN_SUPPORTED
	#define VULKAN_SUPPORTED 1
#endif

// #include "Graphics/GraphicsEngine/interface/DeviceContext.h"
// #include "Graphics/GraphicsEngine/interface/RenderDevice.h"
// #include "Graphics/GraphicsEngine/interface/SwapChain.h"
// #include "Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h"
// #include "Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h"
// #include "Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h"
// #include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"

#include <chrono>
#include <thread>

namespace test {

	using namespace std::chrono_literals;

	class TestGame {
	protected:
		bool _shutdown = false;

	public:
		void init() {
			/*SwapChainDesc SCDesc;
			EngineD3D11CreateInfo EngineCI;
#if ENGINE_DLL
			// Load the dll and import GetEngineFactoryD3D11() function
			auto* GetEngineFactoryD3D11 = LoadGraphicsEngineD3D11();
#endif
			auto* pFactoryD3D11 = GetEngineFactoryD3D11();
			pFactoryD3D11->CreateDeviceAndContextsD3D11(EngineCI, &m_pDevice, &m_pImmediateContext);
			Win32NativeWindow Window{hWnd};
			pFactoryD3D11->CreateSwapChainD3D11(m_pDevice, m_pImmediateContext, SCDesc, FullScreenModeDesc{}, Window, &m_pSwapChain);
		*/
		}

		void update() {
			while (!this->_shutdown) {
				std::this_thread::sleep_for(1ms);
			}
		}

		void shutdown() {
		}
	};
} // namespace test
