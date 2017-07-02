#pragma once

// Vertex struct holding position, normal vector, and texture mapping information.
struct VertexPositionNormalTexture
{
    VertexPositionNormalTexture() = default;

    VertexPositionNormalTexture(Vector3 const& position, Vector3 const& normal, Vector2 const& textureCoordinate)
        : position(position),
        normal(normal),
        textureCoordinate(textureCoordinate)
    { }

    Vector3 position;
    Vector3 normal;
    Vector2 textureCoordinate;

    static const int InputElementCount = 3;
    static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};

class RenderMesh
{
public:
    RenderMesh();
    ~RenderMesh();

    void Destroy();
    void CreateVB(int numVertices, int vertexSize, bool dynamic = true, void* initData = nullptr);
    void CreateIB(int numIndices, int indexSize, void* initData = nullptr);
    void UpdateVB(void* data);

private:
    void _CreateBuff(int n, int s, int f0, int f1, void* id, ComPtr<ID3D11Buffer>& b);
    void _UpdateBuff(void* data, ComPtr<ID3D11Buffer>& b);

public:
    ComPtr<ID3D11Buffer> m_vb;
    ComPtr<ID3D11Buffer> m_ib;
    int m_vcount, m_vsize;
    int m_icount;
    int m_isize;
    bool m_vdyn;
};

class RenderMaterial
{
public:
    RenderMaterial(const Vector4& modColor);

    Vector4 m_modulateColor;
    ComPtr<ID3D11Texture2D> m_texture0;
    ComPtr<ID3D11ShaderResourceView> m_srv0;

    static RenderMaterial White;
};