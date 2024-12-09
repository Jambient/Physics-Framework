#pragma once
#include <windows.h>
#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "OBJLoader.h"
#include "Camera.h"

using namespace DirectX;

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 768

class DX11Framework
{
protected:
	// required direct x variables
	ID3D11DeviceContext* m_immediateContext = nullptr;
	ID3D11Device* m_device;
	IDXGIDevice* m_dxgiDevice = nullptr;
	IDXGIFactory2* m_dxgiFactory = nullptr;
	ID3D11RenderTargetView* m_frameBufferView = nullptr;
	IDXGISwapChain1* m_swapChain;
	D3D11_VIEWPORT m_viewport;
	ID3D11Texture2D* m_depthStencilBuffer;
	ID3D11DepthStencilView* m_depthStencilView;

	// window handle for use in applications
	HWND m_windowHandle;

public:
	~DX11Framework();

	/**
	 * @brief Runs each initialise method in the framework, also stores the windowHandle for use in the application
	 * @param windowHandle The handle of the window created for the application
	 * @return HRESULT indicating the initialisation was successful
	 */
	HRESULT Initialise(HWND windowHandle);

	/**
	 * @brief Create the framework devices
	 * @return HRESULT if everything was successful
	 */
	HRESULT CreateD3DDevice();

	/**
	 * @brief Create the swap chain and frame buffer using the device
	 * @return HRESULT if everything was successful
	 */
	HRESULT CreateSwapChainAndFrameBuffer();

	/**
	 * @brief Create and set the viewport in the pipeline
	 * @return HRESULT if everything was successful
	 */
	HRESULT InitPipelineVariables();

	/**
	 * @brief Call the virtual initialise method so the application can initialise
	 * @return HRESULT if the application succesfully intialised
	 */
	HRESULT InitRunTimeData();

	/**
	 * @brief Virtual method hooked onto the windows mouse move callback
	 * @param btnState The current state of the mouse when the callback happened
	 * @param x The x position of the mouse relative to the application
	 * @param y The y position of the mouse relative to the application
	 */
	virtual void OnMouseMove(WPARAM btnState, int x, int y) {};

	/**
	 * @brief Virtual method hooked onto the windows mouse click callback
	 * @param pressedBtn The button that was pressed resulting in the callback
	 * @param x The x position of the mouse relative to the application 
	 * @param y The y position of the mouse relative to the application
	 */
	virtual void OnMouseClick(WPARAM pressedBtn, int x, int y) {};

	/**
	 * @brief Virtual method hooked onto the windows mouse wheel callback
	 * @param wheelDelta The delta offset of the mouse wheel, 120 for forwards, -120 for backwards.
	 * @param x The x position of the mouse relative to the application
	 * @param y The y position of the mouse relative to the application
	 */
	virtual void OnMouseWheel(WPARAM wheelDelta, int x, int y) {};

	/**
	 * @brief Virtual method for initialising the application.
	 * @return HRESULT if the initialisation was successful
	 */
	virtual HRESULT Init() { return S_OK; }

	/**
	 * @brief Called every frame before draw.
	 */
	virtual void Update() {};

	/**
	 * @brief Called every frame after update.
	 */
	virtual void Draw() {};
};