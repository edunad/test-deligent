#pragma once

#include <Common/interface/RefCntAutoPtr.hpp>

#include <Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <Graphics/GraphicsEngine/interface/GraphicsTypes.h>
#include <Graphics/GraphicsEngine/interface/PipelineState.h>
#include <Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <Graphics/GraphicsEngine/interface/SwapChain.h>

#include <memory>

struct GLFWwindow;

namespace test {

	using TClock = std::chrono::high_resolution_clock;
	using TSeconds = std::chrono::duration<float>;

	class TestGame {
	protected:
		bool _initialized = false;

		GLFWwindow* _handle = nullptr;

		Diligent::RefCntAutoPtr<Diligent::IRenderDevice> _pDevice;
		Diligent::RefCntAutoPtr<Diligent::IDeviceContext> _pImmediateContext;
		Diligent::RefCntAutoPtr<Diligent::ISwapChain> _pSwapChain;
		Diligent::RefCntAutoPtr<Diligent::IPipelineState> _pPSO;

		TClock::time_point _lastUpdate = {};

	public:
		void init(Diligent::RENDER_DEVICE_TYPE type = Diligent::RENDER_DEVICE_TYPE::RENDER_DEVICE_TYPE_UNDEFINED);
		void createWindow(int api, const std::string& title);
		void createEngine(Diligent::RENDER_DEVICE_TYPE type);

		void initGame();
		void update();
		void shutdown();

		static void callbacks_resize(GLFWwindow* whandle, int width, int height);

		void draw();
	};
} // namespace test
