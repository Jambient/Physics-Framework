#pragma once
#define NDEBUG

#include "DX11Framework.h"
#include "Structures.h"
#include "Camera.h"
#include "OBJLoader.h"
#include "Scene.h"
#include "PhysicsSystem.h"
#include <vector>

struct InstanceData
{
	XMFLOAT3 Position;
};

/**
 * @class DX11App
 * @brief A class that represents the rendering project application using the framework.
 */
class DX11App : public DX11Framework
{
public:
	/**
	 * @brief Deconstructs the application and deletes pointer references
	 */
	~DX11App();

	/**
	 * @brief Overriden Init() method taken from the framework, used to build shaders and the scene.
	 * @return HRESULT indicating if the intialization was successful or not.
	 */
	HRESULT Init() override;

	/**
	 * @brief Override Update() method taken from the framework, used to detect inputs for the camera + update the scene
	 */
	void Update() override;

	/**
	 * @brief Overriden Draw() method taken from the framework, used to update buffers and draw the scene.
	 */
	void Draw() override;

	void OnMouseMove(WPARAM btnState, int x, int y) override;
	void OnMouseClick(WPARAM pressedBtn, int x, int y) override;
	void OnMouseWheel(WPARAM wheelDelta, int x, int y) override;

private:
	// shader buffers
	TransformBuffer m_transformBufferData;
	GlobalBuffer m_globalBufferData;
	LightBuffer m_lightBufferData;

	// properties for handling cameras
	XMINT2 m_lastMousePos;
	Camera* m_camera;

	// stores the sum of all delta time values
	float elapsedTime;

	Scene m_scene;
	std::shared_ptr<PhysicsSystem> m_physicsSystem;

	// mesh data
	MeshData m_cubeMeshData;

	ID3D11Buffer* m_instanceBuffer;

	// fps title variables
	float m_timeSinceLastTitleUpdate = 1.0f;
	int m_framesSinceLastTitleUpdate = 0;

	// rendering variables
	std::vector<InstanceData> m_instanceData;

	// default rendering states
	ID3D11RasterizerState* m_defaultRasterizerState;
	ID3D11SamplerState* m_defaultSamplerState;
};

