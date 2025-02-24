#pragma once
//#define NDEBUG

#include "DX11Framework.h"
#include "Structures.h"
#include "Camera.h"
#include "ECSScene.h"
#include "Timer.h"
#include "Vector3.h"
#include "AABBTree.h"
#include "Material.h"
#include "Terrain.h"
#include <vector>

struct InstanceData
{
	Vector3 position;
	Vector3 Scale;
	Vector3 Color;
};

enum class ClickAction
{
	SELECT,
	PUSH,
	DRAG
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
	void OnMouseRelease(WPARAM releasedBtn, int x, int y) override;
	void OnMouseWheel(WPARAM wheelDelta, int x, int y) override;

private:
	// shader buffers
	TransformBuffer m_transformBufferData;
	GlobalBuffer m_globalBufferData;
	LightBuffer m_lightBufferData;

	// properties for handling cameras
	XMINT2 m_lastMousePos;
	Camera* m_camera;

	AABBTree m_aabbTree;
	ECSScene m_scene;
	double m_physicsAccumulator = 0.0;
	Timer m_timer;

	// default rendering states
	ID3D11RasterizerState* m_defaultRasterizerState;
	ID3D11SamplerState* m_defaultSamplerState;

	ID3D11DepthStencilState* m_skyboxDepthState;
	ID3D11RasterizerState* m_wireframeRasterizerState;

	Entity m_selectedEntity = INVALID_ENTITY;
	Entity m_draggingEntity = INVALID_ENTITY;
	float m_draggingDistance = 0.0f;

	ID3D11ShaderResourceView* m_skyboxSRV;
	std::vector<int> m_possibleMaterialIDs;

	std::vector<Vector3> m_debugPoints;
	bool m_showDebugPoints = false;
	bool m_showBoundingVolumes = false;

	float m_physicsDuration = 0.0f;

	ClickAction m_currentClickAction;

	Terrain* m_terrain;

	UINT maxDebugSprings = 20000;
	ID3D11Buffer* m_springVertexBuffer;

	/**
	 * @brief Builds a ray pointing towards the provided position on the screen from the camera's position
	 * @param x The X offset of the position on the screen.
	 * @param y The Y offset of the position on the screen.
	 * @return A ray pointing towards the screen position.
	 */
	Ray GetRayFromScreenPosition(int x, int y);
};

