#pragma once

class Camera
{
public:
    Camera()
        : m_pitchYawRoll(Vector3::Zero), m_advanceFactor(0.5f)
    {
    }

    void SetView(const Vector3& eye, const Vector3& at, const Vector3& up = Vector3::UnitY);
    void SetProj(float fovYRad, float aspect, float _near, float _far);
    void DeltaMouse(int dx, int dy);
    void WheelMouse(int dir);
    void AdvanceForward(float d);
    void AdvanceRight(float d);
    void _ComputeView();

    Matrix m_view;
    Matrix m_proj;
    Vector3 m_pitchYawRoll;
    Vector3 m_position;
    float m_advanceFactor;
};

