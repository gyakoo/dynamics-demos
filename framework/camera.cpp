#include "pch.h"
#include "camera.h"
#include "framework.h"

void Camera::SetView(const Vector3& eye, const Vector3& at, const Vector3& up)
{
    m_view = Matrix::CreateLookAt(eye, at, up).Transpose();
    m_position = eye;
    Vector3 fw = -m_view.Forward();
    m_pitchYawRoll.y = atan2f(fw.x, fw.z);
    //if (m_pitchYawRoll.y < 0.0f) m_pitchYawRoll.y += XM_PI;
    //m_pitchYawRoll.x = atan2f(fw.y, fw.x);
    //if (m_pitchYawRoll.x < 0.0f) m_pitchYawRoll.x += XM_PI;
}

void Camera::SetProj(float fovYRad, float aspect, float _near, float _far)
{
    m_proj = Matrix::CreatePerspectiveFieldOfView(fovYRad, aspect, _near, _far).Transpose();
}

static inline float sign(int d)
{
    return float((d > 0) - (d < 0));
}

void Camera::_ComputeView()
{
    XMMATRIX ry = XMMatrixRotationY(m_pitchYawRoll.y);
    XMMATRIX rx = XMMatrixRotationX(m_pitchYawRoll.x);
    XMMATRIX t = XMMatrixTranslation(-m_position.x, -m_position.y, -m_position.z);
    XMMATRIX m = XMMatrixMultiply(ry, rx);
    m_view = XMMatrixTranspose(XMMatrixMultiply(t, m));
}

void Camera::DeltaMouse(int dx, int dy)
{
    DemoFramework* df = DemoFramework::GetInstance();
    m_pitchYawRoll.y -= sign(dx) * XM_PIDIV2 * df->m_timeStep;
    m_pitchYawRoll.x -= sign(dy) * XM_PIDIV2* df->m_timeStep;
    _ComputeView();
}

void Camera::WheelMouse(int dir)
{
    if (dir > 0)
        m_advanceFactor += 0.01f;
    else
        m_advanceFactor -= 0.01f;

    m_advanceFactor = clamp(m_advanceFactor, 0.01f, 1.0f);
}

void Camera::AdvanceForward(float d)
{
    m_position += m_view.Forward()*d*m_advanceFactor;
    _ComputeView();
}

void Camera::AdvanceRight(float d)
{
    m_position += m_view.Right()*d*m_advanceFactor;
    _ComputeView();
}
