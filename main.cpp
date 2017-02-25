#include "pch.h"
#include "framework.h"

class PDBClothDemo : public DemoFramework
{
public:
    virtual void OnInit() override
    {
        // Cetup camera

        // Create cloth mesh
        m_mesh = std::make_shared<Mesh>();
    }
        
    virtual void OnFrame() override
    {
        // Render Cloth Mesh
    }

    std::shared_ptr<Mesh> m_mesh;
};

IMPLEMENT_DEMO(PDBClothDemo);