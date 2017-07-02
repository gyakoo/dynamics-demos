#pragma once

//
// ShapeBase
//
struct ShapeBase
{    
    struct Vertex
    {
        Vector3 m_pos;
    };

    struct ShapePlane
    {
        Plane m_plane;
    };

    enum Type
    {
        SHAPE_UNKNOWN = -1,
        SHAPE_SPHERE = 0,
        SHAPE_BOX
    };

    ShapeBase(Type type=SHAPE_UNKNOWN)
        : m_type(type)
    {

    }

    virtual float support(const Vector3& dir, Vector3* out) const = 0;
    virtual ShapeBase* clone() const = 0;

    virtual bool hasVertices() const { return false; }
    virtual int getNumVertices() const { return 0; }
    virtual const Vertex& getVertex(int index) const { return *(Vertex*)(0); }
    virtual bool hasPlanes() const { return false; }
    virtual int getNumPlanes() const { return 0; }
    virtual const ShapePlane& getPlane(int index) const { return *(ShapePlane*)(0); }

public: // factory
    static ShapeBase* makeSphere(float radius);
    static ShapeBase* makeBox(const Vector3& halfExtents);

    Type m_type;
};

//
// Shape
//
struct Shape : public ShapeBase
{
public: // shape api

    Shape()
        : m_verticesCount(0), m_planesCount(0)
    {
    }

    inline virtual bool hasVertices() const override { return m_verticesCount > 0; }
    inline virtual int getNumVertices() const override { return m_verticesCount; }
    inline virtual const Vertex& getVertex(int index) const override { return m_vertices[index]; }
    inline virtual bool hasPlanes() const override { return m_planesCount > 0; }
    inline virtual int getNumPlanes() const override { return m_planesCount; }
    inline virtual const ShapePlane& getPlane(int index) const override { return m_planes[index]; }

public: // data
    std::unique_ptr<Vertex[]> m_vertices;
    std::unique_ptr<ShapePlane[]> m_planes;
    int m_verticesCount;
    int m_planesCount;
};

//
// ShapeSphere
//
struct ShapeSphere : public ShapeBase
{
    ShapeSphere(float radius);

    virtual float support(const Vector3& dir, Vector3* out) const override;
    virtual ShapeBase* clone() const override;    

    float m_radius;
};

//
//
//
struct ShapeBox : public Shape
{
    ShapeBox(const Vector3& halfExtents);

    virtual float support(const Vector3& dir, Vector3* out) const override;
    virtual ShapeBase* clone() const override;
};


//
//
//
struct GjkOutput
{
    Vector3 m_vtInA;
    Vector3 m_vtInB;
    float m_minDist;
};
float GjkDistance(ShapeBase& a, ShapeBase& b, GjkOutput* out);
