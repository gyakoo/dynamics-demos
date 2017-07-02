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
    RenderMesh m_obj;
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
    m_framework->CreateRenderSphere(1.0f, 8.0f, m_obj);
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
    m_framework->RenderObj(m_obj, RenderMaterial::White, Matrix::Identity);

}

DEMO(RBDynamics, "Rigid Body Dynamics", "This demo shows basic Rigid Bodies");
