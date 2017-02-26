#include "pch.h"
#include "framework.h"

class ClothSimpleDemo : public Demo
{
public:
    ClothSimpleDemo()
    {
    }

    virtual void OnInitDemo() override
    {
        // Cetup camera

        // Create cloth mesh
        m_mesh = std::make_shared<RenderMesh>();
    }

    virtual void OnDestroyDemo() override
    {
        m_mesh.reset();
    }

    virtual void OnStepDemo() override
    {
        // Render Cloth Mesh
    }

    virtual void OnRenderGuiSetup() override
    {
        static float f = 0.0f;
        ImGui::Text("Hello, world!");
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        static ImVec4 clear_col = ImColor(114, 144, 154);
        ImGui::ColorEdit3("clear color", (float*)&clear_col);
    }

    virtual void OnRenderGui() override
    {
        ImGui::Text("Hello, world!");
    }

    std::shared_ptr<RenderMesh> m_mesh;
};

DEMO(ClothSimpleDemo, "Simple Cloth", "This demo shows basic Distance and Collision Constraints");
