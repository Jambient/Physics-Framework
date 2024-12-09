#pragma once
#include <DirectXMath.h>

using namespace DirectX;

/**
 * @class Camera
 * @brief A class that implements a basic camera.
 */
class Camera
{
public:
	/**
	 * @brief Constructs a Camera object
	 */
	Camera();

	/**
	 * @brief Gets the current position of the camera.
	 * @return The position of the camera.
	 */
	XMFLOAT3 GetPosition() const { return m_Position; }

	/**
	 * @brief Get the vector pointing right relative to the camera's orientation.
	 * @return The camera's right vector.
	 */
	XMFLOAT3 GetRight() const { return m_Right; }

	/**
	 * @brief Get the vector pointing up relative to the camera's orientation. 
	 * @return The camera's up vector.
	 */
	XMFLOAT3 GetUp() const { return m_Up; }

	/**
	 * @brief Get the vector pointing forwards relative to the camera's orientation.
	 * @return The camera's forward/look vector.
	 */
	XMFLOAT3 GetLook() const { return m_Look; }

	/**
	 * @brief Get the camera's view matrix in SIMD form.
	 * @return An XMMATRIX representing the camera's view matrix.
	 */
	XMMATRIX GetView() const { return XMLoadFloat4x4(&m_View); }

	/**
	 * @brief Get the camera's projection matrix in SIMD form.
	 * @return an XMMATRIX representing the camera's projection matrix.
	 */
	XMMATRIX GetProjection() const { return XMLoadFloat4x4(&m_Proj); }

	/**
	 * @brief Sets the projection matrix of the camera with the provided properties.
	 * @param fovY The field of view.
	 * @param aspect The screen aspect ratio.
	 * @param zn The distance the near z plane should be from the camera.
	 * @param zf The distance the far z plane should be from the camera.
	 */
	void SetLens(float fovY, float aspect, float zn, float zf);

	/**
	 * @brief Sets the position of the camera. UpdateViewMatrix() must be used for the changes to be visible.
	 * @param position The new position for the camera.
	 */
	void SetPosition(const XMFLOAT3& position) { m_Position = position; m_ViewMatDirtyFlag = true; }

	/**
	 * @brief Move the camera using the specified movement delta relative to the camera's orientation. UpdateViewMatrix() must be used for the changes to be visible.
	 * @param movementDelta The relative movement applied to the camera.
	 */
	void Move(const XMFLOAT3& movementDelta);

	/**
	 * @brief Rotate the camera using the specified pitch and yaw rotations. Roll is not included as it there are not many use cases for it. UpdateViewMatrix() must be used for the changes to be visible.
	 * @param pitchDelta Rotation along the relative x axis of the camera.
	 * @param yawDelta Rotation along the relative y axis of the camera.
	 */
	void Rotate(float pitchDelta, float yawDelta);

	/**
	 * @brief Updates the camera's view matrix using the stored properties. This uses a dirty flag to avoid repeat view matrix updates. 
	 */
	void UpdateViewMatrix();

private:
	XMFLOAT3 m_Position;
	XMFLOAT3 m_Right;
	XMFLOAT3 m_Up;
	XMFLOAT3 m_Look;

	XMFLOAT4X4 m_View;
	XMFLOAT4X4 m_Proj;

	bool m_ViewMatDirtyFlag;
};

