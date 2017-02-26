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

        const int DIM = 32;
        const float sep = 0.05f;
        const int NPOINTS = DIM*DIM;
        const int NINDICES = (DIM - 1) * (DIM - 1) * 6;
        float x;
        float y = DIM*sep*0.5f;
#define offs(i,j) ((j)*DIM+(i))
        VertexPositionNormalTexture points[NPOINTS];
        uint32_t indices[NINDICES];
        int c = 0;
        for (int j = 0; j < DIM; ++j)
        {
            x = -DIM*sep*0.5f;
            for (int i = 0; i < DIM; ++i, ++c, x+=sep)
            {
                auto& p = points[c];
                p.position = Vector3(x, y, 0);
                p.normal = Vector3(0, 0, -1);
                p.textureCoordinate = Vector2::Zero;
            }
            y -= sep;
        }
        c = 0;
        for (int j = 0; j < DIM-1; ++j)
        {
            for (int i = 0; i < DIM-1; ++i)
            {
                indices[c++] = offs(i, j);
                indices[c++] = offs(i+1, j);
                indices[c++] = offs(i+1, j+1);

                indices[c++] = offs(i, j);
                indices[c++] = offs(i+1, j+1);
                indices[c++] = offs(i, j+1);
            }
        }
        m_mesh->CreateIB(NINDICES, sizeof(uint32_t), indices);
        m_mesh->CreateVB(NPOINTS, sizeof(VertexPositionNormalTexture), false, points);
    }

    virtual void OnDestroyDemo() override
    {
        m_mesh.reset();
    }

    virtual void OnStepDemo() override
    {
        // Render Cloth Mesh
        m_framework->RenderObj(*m_mesh.get(), RenderMaterial::White, Matrix::Identity);
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
