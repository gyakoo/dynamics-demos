#include "pch.h"
#include "framework.h"

class RBDynamics: public Demo
{
public:
    RBDynamics();
    virtual void OnInitDemo() override;
    virtual void OnDestroyDemo() override;
    virtual void OnStepDemo() override;
    virtual void OnRenderGuiSetup() override;
    virtual void OnRenderGui() override;

protected:
    RenderMesh m_sphere;
    RenderMesh m_box;
    double m_simTime;
    double m_renderTime;
};

RBDynamics::RBDynamics()
    : m_simTime(0), m_renderTime(0)
{

}

void RBDynamics::OnInitDemo()
{
    m_framework->m_timeStep = 1.0f / 60.0f;
    m_framework->CreateRenderSphere(1.0f, 4, m_sphere);
    m_framework->CreateRenderBox(Vector3(1.0f, 1.0f, 1.0f), m_box);
}

void RBDynamics::OnDestroyDemo()
{
}

void RBDynamics::OnRenderGuiSetup()
{
    static float f = 0.0f;
    ImGui::Text("This demo shows a basic Rigid Body dynamics solver");    
}

void RBDynamics::OnRenderGui()
{
    const int height = 380;
    {
        static char text[128];

        ImGui::LabelText("", "Simulation/Render: %.3f/%.3f", m_simTime, m_renderTime);

    }
}

void RBDynamics::OnStepDemo()
{
    m_framework->RenderObj(m_box, RenderMaterial::White, Matrix::Identity);
    m_framework->RenderObj(m_sphere, RenderMaterial::White, Matrix::CreateTranslation(0, 1.0f, 0.0f) );
}

DEMO(RBDynamics, "Rigid Body Dynamics", "This demo shows basic Rigid Bodies");
