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

struct RigidBody
{
    Vector3 m_linearVelocity;
    Vector3 m_angularVelocity;
    Matrix m_transform;
    int m_shape;
};


class PhysicsWorld
{
public:

public:
    int CreateShapeSphere(float radius);
    int CreateShapeBox(const Vector3& halfExtents);
    int CreateBody(int shape, const Matrix& transform, const Vector3& linearVel = Vector3::Zero, const Vector3& angVel = Vector3::Zero);

    inline ShapeBase* GetShape(int index) { return m_shapes[index].get(); }
    inline RigidBody* GetBody(int index) { return m_bodies[index].get(); }

protected:
    std::vector<std::shared_ptr<ShapeBase>> m_shapes;
    std::vector<std::shared_ptr<RigidBody>> m_bodies;
    std::vector<int> m_shapesFreeList;
    std::vector<int> m_bodiesFreeList;
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
