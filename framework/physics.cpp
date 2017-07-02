#include "pch.h"
#include "physics.h"
#include "framework.h"

ShapeBase* ShapeBase::makeSphere(float radius)
{
    return new ShapeSphere(radius);
}

ShapeBase* ShapeBase::makeBox(const Vector3& halfExtents)
{
    return new ShapeBox(halfExtents);
}



ShapeSphere::ShapeSphere(float radius)
    : ShapeBase(SHAPE_SPHERE), m_radius(radius)
{
    assert(radius > 0);
}

float ShapeSphere::support(const Vector3& dir, Vector3* out) const
{
    Vector3 dirNorm = dir;
    dirNorm.Normalize();
    (*out) = dirNorm * m_radius;
    return m_radius;
}

ShapeBase* ShapeSphere::clone() const
{
    return Shape::makeSphere(m_radius);
}

// --------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------
ShapeBox::ShapeBox(const Vector3& halfExtents)
{
    m_type = SHAPE_BOX;

    // building vertices
    m_vertices = std::make_unique<Vertex[]>(8);
    m_verticesCount = 8;
    m_planes = std::make_unique<ShapePlane[]>(6);
    m_planesCount = 6;

    const float x2 = halfExtents.x;
    const float y2 = halfExtents.y;
    const float z2 = halfExtents.z;
    m_vertices[0].m_pos = Vector3(-x2, -y2, -z2);
    m_vertices[1].m_pos = Vector3(-x2,  y2, -z2);
    m_vertices[2].m_pos = Vector3( x2,  y2, -z2);
    m_vertices[3].m_pos = Vector3( x2, -y2, -z2);
    m_vertices[4].m_pos = Vector3(-x2, -y2, z2);
    m_vertices[5].m_pos = Vector3(-x2, y2, z2);
    m_vertices[6].m_pos = Vector3(x2, y2, z2);
    m_vertices[7].m_pos = Vector3(x2, -y2, z2);
    m_planes[0].m_plane = Plane(1, 0, 0, x2);
    m_planes[1].m_plane = Plane(-1, 0, 0, x2);
    m_planes[2].m_plane = Plane(0, 1, 0, y2);
    m_planes[3].m_plane = Plane(0,-1, 0, y2);
    m_planes[4].m_plane = Plane(0, 0, 1, z2);
    m_planes[5].m_plane = Plane(0, 0, -1, z2);
}

float ShapeBox::support(const Vector3& dir, Vector3* out) const
{
    float maxD = -FLT_MAX;
    int maxI = -1;
    for (int i = 0; i < m_verticesCount; ++i)
    {
        const float projD = dir.Dot(m_vertices[i].m_pos);
        if (projD > maxD)
        {
            maxD = projD;
            maxI = i;
        }
    }
    *out = m_vertices[maxI].m_pos;
    return maxD;
}

ShapeBase* ShapeBox::clone() const
{
    return nullptr;
}


float GjkDistance(Shape& a, Shape& b, GjkOutput* out)
{
    return 0;
}