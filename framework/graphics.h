#pragma once

class RenderMaterial;

template<typename T>
void SubdivideFaces(std::vector<T>& vertices);

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

	virtual bool PreRender(RenderMaterial& rm, Matrix& transf) const { return true; }
	virtual void PostRender() const { };

protected:
    void _CreateBuff(int n, int s, int f0, int f1, void* id, ComPtr<ID3D11Buffer>& b);
    void _UpdateBuff(void* data, ComPtr<ID3D11Buffer>& b);

public:
    ComPtr<ID3D11Buffer> m_vb;
    ComPtr<ID3D11Buffer> m_ib;
    D3D11_PRIMITIVE_TOPOLOGY m_topo;
    int m_vcount, m_vsize;
    int m_icount;
    int m_isize;
    bool m_vdyn;
};

class RenderMeshLines : public RenderMesh
{
public:
	RenderMeshLines();

	virtual bool PreRender(RenderMaterial& rm, Matrix& transf) const;
	virtual void PostRender() const;

	void DrawLine(const Vector3& from, const Vector3& to, const Color& color);

protected:
	struct LineVertex
	{
		Vector3 from, to;
		Color color;
	};

	mutable std::vector<LineVertex> m_lines;
};

class RenderMaterial
{
public:
    RenderMaterial(const Vector4& modColor);

    Vector4 m_modulateColor;
    ComPtr<ID3D11Texture2D> m_texture0;
    ComPtr<ID3D11ShaderResourceView> m_srv0;
	ComPtr<ID3D11InputLayout> m_inputLayout;
	ComPtr<ID3D11VertexShader> m_vertexShader;
	ComPtr<ID3D11PixelShader> m_pixelShader;
	UINT m_vertexStride;

    static RenderMaterial White;
};



template<typename T>
void SubdivideFaces<T>(std::vector<T>& vertices)
{
    std::vector<T> newVerts;
    newVerts.reserve(vertices.size() * 2);

    for (size_t i = 0; i < vertices.size(); i += 3)
    {
        const T v[3] = { vertices[i], vertices[i + 1], vertices[i + 2] };
        // computing the vertex with more open angle 
        const float dots[3] = 
        {
            (v[1].position - v[0].position).Dot(v[2].position - v[0].position),
            (v[2].position - v[1].position).Dot(v[0].position - v[1].position),
            (v[0].position - v[2].position).Dot(v[1].position - v[2].position) 
        };

        T d = v[0];
        if (dots[0] < dots[1] && dots[0] < dots[2])
        {
            d.position = (v[1].position + v[2].position)*0.5f;
            newVerts.push_back(v[0]); newVerts.push_back(v[1]); newVerts.push_back(d);
            newVerts.push_back(v[0]); newVerts.push_back(d); newVerts.push_back(v[2]);
        }
        else if (dots[1] < dots[2] && dots[1] < dots[0])
        {
            d.position = (v[2].position + v[0].position)*0.5f;
            newVerts.push_back(v[0]); newVerts.push_back(v[1]); newVerts.push_back(d);
            newVerts.push_back(v[1]); newVerts.push_back(v[2]); newVerts.push_back(d);
        }
        else
        {
            d.position = (v[0].position + v[1].position)*0.5f;
            newVerts.push_back(v[0]); newVerts.push_back(d); newVerts.push_back(v[2]);
            newVerts.push_back(d); newVerts.push_back(v[1]); newVerts.push_back(v[2]);
        }
    }
    vertices = std::move(newVerts);
}