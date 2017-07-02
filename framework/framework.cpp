#include "pch.h"
#include "framework.h"
#include <shellapi.h>

const D3D11_INPUT_ELEMENT_DESC VertexPositionNormalTexture::InputElements[] =
{
    { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

void DemoFramework::Destroy()
{
    if (!m_pd3dDevice)
        return;

    for (auto& d : m_demos)
        d->OnDestroyFramework();

    m_vsPNT.Reset();
    m_psPNT.Reset();
    m_ilPNT.Reset();
    m_cbMVP.Reset();
    m_cbF4.Reset();
    m_linearClamp.Reset();

    ImGui_ImplDX11_Shutdown();
    CleanupRenderTarget();
    if (m_pSwapChain) { m_pSwapChain->Release(); m_pSwapChain = NULL; }
    if (m_pd3dDeviceContext) { m_pd3dDeviceContext->Release(); m_pd3dDeviceContext = NULL; }
    if (m_pd3dDevice) { m_pd3dDevice->Release(); m_pd3dDevice = NULL; }
    UnregisterClass(L"demoframework", NULL);
}


HRESULT DemoFramework::Init(const wchar_t* title, int width, int height, bool fullscreen)
{
    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DemoFramework::WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, L"demoframework", NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(L"demoframework", title, WS_OVERLAPPEDWINDOW, 100, 100, width, height, NULL, NULL, wc.hInstance, NULL);
    m_width = width; 
    m_height = height;
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2;
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = !fullscreen;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;        
    }

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 1, D3D11_SDK_VERSION, &sd, &m_pSwapChain, &m_pd3dDevice, &featureLevel, &m_pd3dDeviceContext) != S_OK)
        return E_FAIL;

    // Setup rasterizer
    {
        D3D11_RASTERIZER_DESC RSDesc;
        memset(&RSDesc, 0, sizeof(D3D11_RASTERIZER_DESC));
        RSDesc.FillMode = D3D11_FILL_SOLID;
        RSDesc.CullMode = D3D11_CULL_NONE;
        RSDesc.FrontCounterClockwise = FALSE;
        RSDesc.DepthBias = 0;
        RSDesc.SlopeScaledDepthBias = 0.0f;
        RSDesc.DepthBiasClamp = 0;
        RSDesc.DepthClipEnable = TRUE;
        RSDesc.ScissorEnable = TRUE;
        RSDesc.AntialiasedLineEnable = FALSE;
        RSDesc.MultisampleEnable = (sd.SampleDesc.Count > 1) ? TRUE : FALSE;
        m_pd3dDevice->CreateRasterizerState(&RSDesc, m_fillSolid.GetAddressOf());
        m_pd3dDeviceContext->RSSetState(m_fillSolid.Get());

        RSDesc.FillMode = D3D11_FILL_WIREFRAME;
        RSDesc.AntialiasedLineEnable = TRUE;
        m_pd3dDevice->CreateRasterizerState(&RSDesc, m_fillWireframe.GetAddressOf());

        // Setup viewport
        D3D11_VIEWPORT vp;
        memset(&vp, 0, sizeof(D3D11_VIEWPORT));
        vp.Width = (FLOAT)m_width;
        vp.Height = (FLOAT)m_height;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        m_pd3dDeviceContext->RSSetViewports(1, &vp);

        D3D11_DEPTH_STENCIL_DESC DSDesc;
        DSDesc.DepthEnable = true;
        DSDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        DSDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        DSDesc.StencilEnable = false;
        DSDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
        DSDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
        DSDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        DSDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        DSDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        DSDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        DSDesc.BackFace = DSDesc.FrontFace;

        ID3D11DepthStencilState* pDSState = NULL;
        m_pd3dDevice->CreateDepthStencilState(&DSDesc, &pDSState);
        m_pd3dDeviceContext->OMSetDepthStencilState(pDSState, 0);
        pDSState->Release(); // not going to need it anymore?
    }

    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Setup ImGui binding
    ImGui_ImplDX11_Init(hwnd, m_pd3dDevice, m_pd3dDeviceContext);

	if (FAILED(InitResources()))
		return E_FAIL;

    // camera
    m_camera.SetView(Vector3(0, 0.0f, -25.0f), Vector3::Zero, Vector3::UnitY);
    m_camera.SetProj(70.0f*XM_PI / 180.0f, float(width) / height, 0.05f, 100.0f);

    m_timeStep = 1.0f / 60.0f; // default timestep for demo framework
    for (auto& d : m_demos)
        d->OnInitFramework();

    // command line
    {
        int nargs = 0;
        wchar_t** args = CommandLineToArgvW(GetCommandLineW(), &nargs);
        for (int i = 1; i < nargs; ++i)
        {
            if (wcscmp(args[i], L"-d") == 0 && (i+1)<nargs )
            {
                int d = _wtoi(args[i + 1]);
                if (d < (int)m_demos.size())
                {
                    m_demos[d]->m_running = true;
                    m_demos[d]->OnInitDemo();
                }
                ++i;
            }
        }
        LocalFree(args);
    }
    return S_OK;
}

static int ReadEntireFile(const std::string& filename, std::vector<byte>& out)
{
    FILE* f = nullptr;
    fopen_s(&f, filename.c_str(), "rb");
    if (!f)
        return 0;
    fseek(f, 0, SEEK_END);
    long sizeInBytes = ftell(f);
    out.resize(sizeInBytes);
    fseek(f, 0, SEEK_SET);
    byte* data = out.data();
    size_t r = 0;
    while ((r = fread(data, 1, 4096, f)))
        data += r;
    fclose(f);
    return int(data - out.data());
}

HRESULT DemoFramework::InitResources()
{
    std::vector<byte> data;
    // Position - Normal - Texture
    {
        // vertex shader
        if (!ReadEntireFile("VS_PNT.cso", data)) return E_FAIL;
        HRESULT hr = m_pd3dDevice->CreateVertexShader(data.data(), data.size(), nullptr, m_vsPNT.ReleaseAndGetAddressOf());
        if (FAILED(hr)) return hr;

        // input layout
        hr = m_pd3dDevice->CreateInputLayout(
            VertexPositionNormalTexture::InputElements, VertexPositionNormalTexture::InputElementCount,
            data.data(), data.size(), m_ilPNT.ReleaseAndGetAddressOf());
        if (FAILED(hr)) return hr;

        // pixel shader
        if (!ReadEntireFile("PS_PNT.cso", data)) return E_FAIL;
        hr = m_pd3dDevice->CreatePixelShader(data.data(), data.size(), nullptr, m_psPNT.ReleaseAndGetAddressOf());
        if (FAILED(hr)) return hr;
    }

    // constant buffers
    {
        {
            CD3D11_BUFFER_DESC constantBufferDesc(sizeof(XMFLOAT4X4) * 3, D3D11_BIND_CONSTANT_BUFFER);
            HRESULT hr = m_pd3dDevice->CreateBuffer(&constantBufferDesc, nullptr, m_cbMVP.ReleaseAndGetAddressOf());
            if (FAILED(hr)) return hr;
        }

        {
            CD3D11_BUFFER_DESC constantBufferDesc(sizeof(XMFLOAT4), D3D11_BIND_CONSTANT_BUFFER);
            HRESULT hr = m_pd3dDevice->CreateBuffer(&constantBufferDesc, nullptr, m_cbF4.ReleaseAndGetAddressOf());
            if (FAILED(hr)) return hr;
        }
    }

    // other states
    {
        {
            D3D11_SAMPLER_DESC desc = {  };
            desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.MaxAnisotropy = (m_pd3dDevice->GetFeatureLevel() > D3D_FEATURE_LEVEL_9_1) ? D3D11_MAX_MAXANISOTROPY : 2;
            desc.MaxLOD = FLT_MAX;
            desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            HRESULT hr = m_pd3dDevice->CreateSamplerState(&desc, m_linearClamp.ReleaseAndGetAddressOf());
            if (FAILED(hr)) return hr;
        }

        {
            D3D11_BLEND_DESC desc = {};
            desc.RenderTarget[0].BlendEnable = FALSE;
            desc.RenderTarget[0].SrcBlend = desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            desc.RenderTarget[0].DestBlend = desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
            desc.RenderTarget[0].BlendOp = desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

            desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

            HRESULT hr = m_pd3dDevice->CreateBlendState(&desc, m_opaque.ReleaseAndGetAddressOf());
            if (FAILED(hr)) return hr;
        }
    }

    // default materials
    {
        const uint32_t dim = 64;
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = dim;
        desc.Height = dim;
        desc.MipLevels = desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;// DYNAMIC;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;// D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;

        uint32_t whitedata[dim *dim];
        memset(whitedata, 0xffffffff, sizeof(whitedata));
        D3D11_SUBRESOURCE_DATA data;
        ZeroMemory(&data, sizeof(data));
        data.pSysMem = whitedata;      
        data.SysMemPitch = sizeof(uint32_t) * dim;
        HRESULT hr = m_pd3dDevice->CreateTexture2D(&desc, &data, RenderMaterial::White.m_texture0.ReleaseAndGetAddressOf());
        if (FAILED(hr)) return hr;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;        
        hr = m_pd3dDevice->CreateShaderResourceView(RenderMaterial::White.m_texture0.Get(), &srvDesc, RenderMaterial::White.m_srv0.ReleaseAndGetAddressOf());
        if (FAILED(hr)) return hr;
    }

    return S_OK;
}

void DemoFramework::RenderObj(const RenderMesh& rm, const RenderMaterial& mat, const Matrix& transform)
{
    // Prepare the constant buffer to send it to the graphics device.
    const UINT stride = sizeof(VertexPositionNormalTexture);
    UINT offset = 0;
    m_pd3dDeviceContext->IASetPrimitiveTopology(rm.m_topo);
    m_pd3dDeviceContext->IASetInputLayout(m_ilPNT.Get());
    m_pd3dDeviceContext->OMSetBlendState(m_opaque.Get(), nullptr, 0xffffffff);

    // VS    
    XMFLOAT4X4 mvpData[3] = { transform, m_camera.m_view, m_camera.m_proj};
    m_pd3dDeviceContext->UpdateSubresource(m_cbMVP.Get(), 0, NULL, mvpData, 0, 0);
    m_pd3dDeviceContext->VSSetShader(m_vsPNT.Get(), nullptr, 0);
    m_pd3dDeviceContext->VSSetConstantBuffers(0, 1, m_cbMVP.GetAddressOf());

    // PS
    m_pd3dDeviceContext->PSSetShader(m_psPNT.Get(), nullptr, 0);
    m_pd3dDeviceContext->UpdateSubresource(m_cbF4.Get(), 0, NULL, &mat.m_modulateColor, 0, 0);
    m_pd3dDeviceContext->PSSetConstantBuffers(0, 1,m_cbF4.GetAddressOf());

    m_pd3dDeviceContext->PSSetShaderResources(0, 1, mat.m_srv0.GetAddressOf());
    m_pd3dDeviceContext->PSSetSamplers(0, 1, m_linearClamp.GetAddressOf());

    // Draw the object
    {
        m_pd3dDeviceContext->IASetVertexBuffers(0, 1, rm.m_vb.GetAddressOf(), &stride, &offset);
        if (rm.m_ib)
        {
            m_pd3dDeviceContext->IASetIndexBuffer(rm.m_ib.Get(), rm.m_isize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
            m_pd3dDeviceContext->DrawIndexed(rm.m_icount, 0, 0);
        }
        else
        {
            m_pd3dDeviceContext->Draw(rm.m_vcount, 0);
        }
    }
}

void DemoFramework::PreRender()
{
    const D3D11_RECT r = { 0,0,m_width,m_height };

    m_pd3dDeviceContext->RSSetScissorRects(1,&r);
    m_pd3dDeviceContext->RSSetState(m_wireframe ? m_fillWireframe.Get() : m_fillSolid.Get());
}

void DemoFramework::PostRender()
{

}

void DemoFramework::CreateRenderSphere(float radius, int nsubdiv, RenderMesh& outMesh)
{
    // Create an unit cube. subdivision of its 6 faces by nsubdiv
    // iterate all vertices in the unit cube and compute the normal from the center
    // normalize normal and scale by radius
    const int dim = nsubdiv + 1;
    const int npoints = dim*dim* 6;    
    std::vector<VertexPositionNormalTexture> vertices; vertices.reserve(npoints);
    typedef VertexPositionNormalTexture vpnt;
    typedef Vector3 v3;
    const float r = radius;
    // front
    vertices.push_back(vpnt(v3(-r, -r, -r), v3(0, 0, -1), Vector2::Zero));
    vertices.push_back(vpnt(v3(-r,  r, -r), v3(0, 0, -1), Vector2::Zero));
    vertices.push_back(vpnt(v3( r,  r, -r), v3(0, 0, -1), Vector2::Zero));

    vertices.push_back(vpnt(v3(-r, -r, -r), v3(0, 0, -1), Vector2::Zero));
    vertices.push_back(vpnt(v3(r, r, -r), v3(0, 0, -1), Vector2::Zero));
    vertices.push_back(vpnt(v3( r, -r, -r), v3(0, 0, -1), Vector2::Zero));

    // top
    vertices.push_back(vpnt(v3(-r, r, -r), v3(0, 1, 0), Vector2::Zero));
    vertices.push_back(vpnt(v3(-r, r,  r), v3(0, 1, 0), Vector2::Zero));
    vertices.push_back(vpnt(v3(r,  r,  r), v3(0, 1, 0), Vector2::Zero));

    vertices.push_back(vpnt(v3(-r, r, -r), v3(0, 1, 0), Vector2::Zero));
    vertices.push_back(vpnt(v3(r, r, r), v3(0, 1, 0), Vector2::Zero));
    vertices.push_back(vpnt(v3(r,  r, -r), v3(0, 1, 0), Vector2::Zero));

    // back
    vertices.push_back(vpnt(v3(r, r,  r), v3(0, 0, 1), Vector2::Zero));
    vertices.push_back(vpnt(v3(r, -r, r), v3(0, 0, 1), Vector2::Zero));
    vertices.push_back(vpnt(v3(-r,-r, r), v3(0, 0, 1), Vector2::Zero));

    vertices.push_back(vpnt(v3(r, r, r), v3(0, 0, 1), Vector2::Zero));
    vertices.push_back(vpnt(v3(-r, -r, r), v3(0, 0, 1), Vector2::Zero));
    vertices.push_back(vpnt(v3(-r, r, r), v3(0, 0, 1), Vector2::Zero));

    // bottom
    vertices.push_back(vpnt(v3(-r, -r, -r), v3(0, -1, 0), Vector2::Zero));
    vertices.push_back(vpnt(v3(-r, -r, r), v3(0, -1, 0), Vector2::Zero));
    vertices.push_back(vpnt(v3( r, -r, r), v3(0, -1, 0), Vector2::Zero));

    vertices.push_back(vpnt(v3(-r, -r, -r), v3(0, -1, 0), Vector2::Zero));
    vertices.push_back(vpnt(v3(r, -r, r), v3(0, -1, 0), Vector2::Zero));
    vertices.push_back(vpnt(v3( r, -r, -r), v3(0, -1, 0), Vector2::Zero));

    // left
    vertices.push_back(vpnt(v3(-r, -r, r), v3(-1, 0, 0), Vector2::Zero));
    vertices.push_back(vpnt(v3(-r, r, r), v3(-1, 0, 0), Vector2::Zero));
    vertices.push_back(vpnt(v3(-r, r, -r), v3(-1, 0, 0), Vector2::Zero));

    vertices.push_back(vpnt(v3(-r, -r, r), v3(-1, 0, 0), Vector2::Zero));
    vertices.push_back(vpnt(v3(-r, r, -r), v3(-1, 0, 0), Vector2::Zero));
    vertices.push_back(vpnt(v3(-r, -r, -r), v3(-1, 0, 0), Vector2::Zero));

    // right
    vertices.push_back(vpnt(v3(r, -r, -r), v3(1, 0, 0), Vector2::Zero));
    vertices.push_back(vpnt(v3(r, r, -r), v3(1, 0, 0), Vector2::Zero));
    vertices.push_back(vpnt(v3(r, r, r), v3(1, 0, 0), Vector2::Zero));

    vertices.push_back(vpnt(v3(r, -r, -r), v3(1, 0, 0), Vector2::Zero));
    vertices.push_back(vpnt(v3(r, r, r), v3(1, 0, 0), Vector2::Zero));
    vertices.push_back(vpnt(v3(r, -r, r), v3(1, 0, 0), Vector2::Zero));
    

    outMesh.CreateVB(vertices.size(), sizeof(vpnt), false, vertices.data());
}

