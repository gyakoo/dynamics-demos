#include "pch.h"
#include "graphics.h"
#include "framework.h"

RenderMesh::RenderMesh()
    : m_vcount(0), m_vsize(0), m_vdyn(false), m_icount(0)
    , m_topo(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
{
}

RenderMesh::~RenderMesh()
{
    Destroy();
}

void RenderMesh::Destroy()
{
    m_vb.Reset();
    m_vb.Reset();
}

void RenderMesh::CreateVB(int numVertices, int vertexSize, bool dynamic, void* initData )
{
    m_vcount = numVertices;
    m_vsize = vertexSize;
    m_vdyn = dynamic;
    _CreateBuff(numVertices, vertexSize, D3D11_BIND_VERTEX_BUFFER,
        dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
        initData, m_vb);
}

void RenderMesh::CreateIB(int numIndices, int indexSize, void* initData )
{
    m_icount = numIndices;
    m_isize = indexSize;
    _CreateBuff(numIndices, indexSize, D3D11_BIND_INDEX_BUFFER,
        D3D11_USAGE_DEFAULT, initData, m_ib);
}

void RenderMesh::UpdateVB(void* data)
{
    if (m_vdyn)
        _UpdateBuff(data, m_vb);
    else
        CreateVB(m_vcount, m_vsize, m_vdyn, data);
} // entire subresource

void RenderMesh::_CreateBuff(int n, int s, int f0, int f1, void* id, ComPtr<ID3D11Buffer>& b)
{
    D3D11_SUBRESOURCE_DATA bd = { 0 };
    bd.pSysMem = id;
    bd.SysMemPitch = 0;
    bd.SysMemSlicePitch = 0;
    CD3D11_BUFFER_DESC extbd((UINT)(n*s), (UINT)f0, (D3D11_USAGE)f1, f1 ? D3D11_CPU_ACCESS_WRITE : 0);
    HRESULT hr = D3DDEVICE->CreateBuffer(&extbd, &bd, b.ReleaseAndGetAddressOf());
    assert(hr == S_OK);
}

void RenderMesh::_UpdateBuff(void* data, ComPtr<ID3D11Buffer>& b)
{
    D3DCONTEXT->UpdateSubresource(b.Get(), 0, 0, data, 0, 0);
}

RenderMaterial::RenderMaterial(const Vector4& modColor)
    : m_modulateColor(modColor)
{
}

RenderMaterial RenderMaterial::White(Vector4(1, 1, 1, 1));