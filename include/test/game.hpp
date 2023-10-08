#pragma once

#include <Common/interface/BasicMath.hpp>
#include <Common/interface/RefCntAutoPtr.hpp>

#include <Graphics/GraphicsEngine/interface/Buffer.h>
#include <Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <Graphics/GraphicsEngine/interface/EngineFactory.h>
#include <Graphics/GraphicsEngine/interface/GraphicsTypes.h>
#include <Graphics/GraphicsEngine/interface/PipelineState.h>
#include <Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <Graphics/GraphicsEngine/interface/ShaderResourceBinding.h>
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
		Diligent::RefCntAutoPtr<Diligent::IEngineFactory> _pEngineFactory;

		// TEST ------
		Diligent::RefCntAutoPtr<Diligent::IPipelineState> _pPSO;
		Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> _pSRB;

		Diligent::RefCntAutoPtr<Diligent::IBuffer> _CubeVertexBuffer;
		Diligent::RefCntAutoPtr<Diligent::IBuffer> _CubeIndexBuffer;
		Diligent::RefCntAutoPtr<Diligent::IBuffer> _VSConstants;

		Diligent::float4x4 _WorldViewProjMatrix;

		// ------------------------

		TClock::time_point _lastUpdate = {};
		float _counter = 0.F;

	public:
		void init(Diligent::RENDER_DEVICE_TYPE type = Diligent::RENDER_DEVICE_TYPE::RENDER_DEVICE_TYPE_UNDEFINED);
		void createWindow(int api, const std::string& title);
		void createEngine(Diligent::RENDER_DEVICE_TYPE type);

		// TEST --------------------
		void createCube();

		[[nodiscard]] Diligent::float4x4 GetSurfacePretransformMatrix(const Diligent::float3& f3CameraViewAxis) const;
		[[nodiscard]] Diligent::float4x4 GetAdjustedProjectionMatrix(float FOV, float NearPlane, float FarPlane) const;
		// -------------------------

		void initGame();
		void update();
		void shutdown();

		static void callbacks_resize(GLFWwindow* whandle, int width, int height);

		void draw();
	};
} // namespace test
