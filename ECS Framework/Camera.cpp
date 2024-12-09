#include "Camera.h"

Camera::Camera()
{
    // initialize the camera with suitable defaults.
    m_Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_Look = XMFLOAT3(0.0f, 0.0f, 1.0f);
    m_Right = XMFLOAT3(1.0f, 0.0f, 0.0f);
    m_Up = XMFLOAT3(0.0f, 1.0f, 0.0f);

    // update the view matrix with these defaults.
    m_ViewMatDirtyFlag = true;
    UpdateViewMatrix();
}

void Camera::SetLens(float fovY, float aspect, float zn, float zf)
{
    // sets the projection matrix using XMMatrixPerspective with the provided parameters.
	XMMATRIX P = XMMatrixPerspectiveFovLH(fovY, aspect, zn, zf);
	XMStoreFloat4x4(&m_Proj, P);
}

void Camera::Move(const XMFLOAT3& movementDelta)
{
    // retrieve each axis of the movement delta
	XMVECTOR xDelta = XMVectorReplicate(movementDelta.x);
	XMVECTOR yDelta = XMVectorReplicate(movementDelta.y);
	XMVECTOR zDelta = XMVectorReplicate(movementDelta.z);

    // load the camera direction vectors into SIMD
	XMVECTOR right = XMLoadFloat3(&m_Right);
	XMVECTOR up = XMLoadFloat3(&m_Up);
	XMVECTOR look = XMLoadFloat3(&m_Look);

    // create the new position by cloning the current position in SIMD
	XMVECTOR newPosition = XMLoadFloat3(&m_Position);

    // update each axis of the new position by multiplying the delta by the appropriate direction vector
	newPosition = XMVectorMultiplyAdd(xDelta, right, newPosition);
	newPosition = XMVectorMultiplyAdd(yDelta, up, newPosition);
	newPosition = XMVectorMultiplyAdd(zDelta, look, newPosition);

    // update the camera's position property with the new position
	XMStoreFloat3(&m_Position, newPosition);
    m_ViewMatDirtyFlag = true;
}

void Camera::Rotate(float pitchDelta, float yawDelta)
{
    // load the camera direction vectors into SIMD
    XMVECTOR right = XMLoadFloat3(&m_Right);
    XMVECTOR up = XMLoadFloat3(&m_Up);
    XMVECTOR look = XMLoadFloat3(&m_Look);

    // check if any pitch is being applied
    if (pitchDelta != 0.0f)
    {
        // create a rotation axis using the pitch delta and apply it to the appropriate direction vectors
        XMMATRIX pitchRotationMatrix = XMMatrixRotationAxis(right, pitchDelta);
        look = XMVector3TransformNormal(look, pitchRotationMatrix);
        up = XMVector3TransformNormal(up, pitchRotationMatrix);
    }

    // check if any yaw is being applied
    if (yawDelta != 0.0f)
    {
        // create a rotation axis using the yaw delta and apply it to the appropriate direction vectors
        XMMATRIX yawRotationMatrix = XMMatrixRotationY(yawDelta);
        look = XMVector3TransformNormal(look, yawRotationMatrix);
        right = XMVector3TransformNormal(right, yawRotationMatrix);
    }

    // store the new look, right, and up vectors back into the camera's properties.
    XMStoreFloat3(&m_Look, look);
    XMStoreFloat3(&m_Right, right);
    XMStoreFloat3(&m_Up, up);
    m_ViewMatDirtyFlag = true;
}

void Camera::UpdateViewMatrix()
{
    // check if the view matrix actually needs changing
    if (!m_ViewMatDirtyFlag) { return; }

    // load the camera's properties into SIMD
    XMVECTOR right = XMLoadFloat3(&m_Right);
    XMVECTOR up = XMLoadFloat3(&m_Up);
    XMVECTOR look = XMLoadFloat3(&m_Look);
    XMVECTOR position = XMLoadFloat3(&m_Position);

    // orthonormalize vectors
    look = XMVector3Normalize(look);
    up = XMVector3Normalize(XMVector3Cross(look, right));
    right = XMVector3Cross(up, look);

    // get the transform position of the camera
    float x = -XMVectorGetX(XMVector3Dot(position, right));
    float y = -XMVectorGetX(XMVector3Dot(position, up));
    float z = -XMVectorGetX(XMVector3Dot(position, look));
    
    // store the orthonormalized vectors back into the camera's properties
    XMStoreFloat3(&m_Look, look);
    XMStoreFloat3(&m_Right, right);
    XMStoreFloat3(&m_Up, up);

    // create view matrix
    m_View = XMFLOAT4X4(
        m_Right.x, m_Up.x, m_Look.x, 0.0f,
        m_Right.y, m_Up.y, m_Look.y, 0.0f,
        m_Right.z, m_Up.z, m_Look.z, 0.0f,
        x, y, z, 1.0f
    );

    // mark the view matrix as updated
    m_ViewMatDirtyFlag = false;
}
