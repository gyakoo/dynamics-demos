#include "pch.h"
#include "framework.h"
#include <shellapi.h>

const D3D11_INPUT_ELEMENT_DESC VertexPositionNormalTexture::InputElements[] =
{
    { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

DemoFramework::DemoFramework()
    : m_pd3dDevice(nullptr)
    , m_pd3dDeviceContext(nullptr)
    , m_pSwapChain(nullptr)
    , m_stepCount(0)
    , m_wireframe(false)
{
}

DemoFramework::~DemoFramework()
{
    Destroy();
}

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
    HWND hwnd = CreateWindow(L"demoframework", title, WS_MAXIMIZE|WS_OVERLAPPEDWINDOW, 100, 100, width, height, NULL, NULL, wc.hInstance, NULL);
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
    //m_camera.SetProj(70.0f*XM_PI / 180.0f, float(width) / height, 0.05f, 1024.0f);

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
    XMFLOAT4X4 mvpData[3] = { transform.Transpose(), m_camera.m_view, m_camera.m_proj};
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

void DemoFramework::CreateRenderBox(const Vector3& halfExtents, std::vector<VertexPositionNormalTexture>& vertices)
{
    vertices.reserve(36);
    typedef VertexPositionNormalTexture vpnt;
    typedef Vector3 v3;
    const float x2 = halfExtents.x;
    const float y2 = halfExtents.y;
    const float z2 = halfExtents.z;

    auto a = v3(-x2, -y2, -z2); auto e = v3(-x2, y2, z2);
    auto b = v3(-x2, y2, -z2); auto f = v3(x2, y2, z2);
    auto c = v3(x2, y2, -z2); auto g = v3(x2, -y2, z2);
    auto d = v3(x2, -y2, -z2); auto h = v3(-x2, -y2, z2);
    // front
    vertices.push_back(vpnt(a, v3(0, 0, -1), Vector2::Zero));
    vertices.push_back(vpnt(b, v3(0, 0, -1), Vector2::Zero));
    vertices.push_back(vpnt(c, v3(0, 0, -1), Vector2::Zero));

    vertices.push_back(vpnt(a, v3(0, 0, -1), Vector2::Zero));
    vertices.push_back(vpnt(c, v3(0, 0, -1), Vector2::Zero));
    vertices.push_back(vpnt(d, v3(0, 0, -1), Vector2::Zero));

    // top
    vertices.push_back(vpnt(b, v3(0, 1, 0), Vector2::Zero));
    vertices.push_back(vpnt(e, v3(0, 1, 0), Vector2::Zero));
    vertices.push_back(vpnt(f, v3(0, 1, 0), Vector2::Zero));

    vertices.push_back(vpnt(b, v3(0, 1, 0), Vector2::Zero));
    vertices.push_back(vpnt(f, v3(0, 1, 0), Vector2::Zero));
    vertices.push_back(vpnt(c, v3(0, 1, 0), Vector2::Zero));

    // back
    vertices.push_back(vpnt(f, v3(0, 0, 1), Vector2::Zero));
    vertices.push_back(vpnt(g, v3(0, 0, 1), Vector2::Zero));
    vertices.push_back(vpnt(h, v3(0, 0, 1), Vector2::Zero));

    vertices.push_back(vpnt(f, v3(0, 0, 1), Vector2::Zero));
    vertices.push_back(vpnt(h, v3(0, 0, 1), Vector2::Zero));
    vertices.push_back(vpnt(e, v3(0, 0, 1), Vector2::Zero));

    // bottom
    vertices.push_back(vpnt(a, v3(0, -1, 0), Vector2::Zero));
    vertices.push_back(vpnt(h, v3(0, -1, 0), Vector2::Zero));
    vertices.push_back(vpnt(g, v3(0, -1, 0), Vector2::Zero));

    vertices.push_back(vpnt(a, v3(0, -1, 0), Vector2::Zero));
    vertices.push_back(vpnt(g, v3(0, -1, 0), Vector2::Zero));
    vertices.push_back(vpnt(d, v3(0, -1, 0), Vector2::Zero));

    // left
    vertices.push_back(vpnt(h, v3(-1, 0, 0), Vector2::Zero));
    vertices.push_back(vpnt(e, v3(-1, 0, 0), Vector2::Zero));
    vertices.push_back(vpnt(b, v3(-1, 0, 0), Vector2::Zero));

    vertices.push_back(vpnt(h, v3(-1, 0, 0), Vector2::Zero));
    vertices.push_back(vpnt(b, v3(-1, 0, 0), Vector2::Zero));
    vertices.push_back(vpnt(a, v3(-1, 0, 0), Vector2::Zero));

    // right
    vertices.push_back(vpnt(d, v3(1, 0, 0), Vector2::Zero));
    vertices.push_back(vpnt(c, v3(1, 0, 0), Vector2::Zero));
    vertices.push_back(vpnt(f, v3(1, 0, 0), Vector2::Zero));

    vertices.push_back(vpnt(d, v3(1, 0, 0), Vector2::Zero));
    vertices.push_back(vpnt(f, v3(1, 0, 0), Vector2::Zero));
    vertices.push_back(vpnt(g, v3(1, 0, 0), Vector2::Zero));
}


void DemoFramework::CreateRenderBox(const Vector3& halfExtents, RenderMesh& outMesh)
{
    std::vector<VertexPositionNormalTexture> vertices;
    CreateRenderBox(halfExtents, vertices);
    outMesh.CreateVB(vertices.size(), sizeof(VertexPositionNormalTexture), false, vertices.data());
}

void DemoFramework::CreateRenderSphere(float radius, int nsubdiv, RenderMesh& outMesh)
{
    // Create an unit cube. subdivision of its 6 faces by nsubdiv
    // iterate all vertices in the unit cube and compute the normal from the center
    // normalize normal and scale by radius   
    std::vector<VertexPositionNormalTexture> vertices;
    CreateRenderBox(Vector3(0.5f,0.5f,0.5f), vertices);

    for ( int i = 0; i < nsubdiv;++i)
        SubdivideFaces(vertices);

    Vector3 normal;
    for (auto& v : vertices)
    {
        normal = v.position;
        normal.Normalize();
        v.position = normal * radius;
        v.normal = normal;
    }

    outMesh.CreateVB(vertices.size(), sizeof(VertexPositionNormalTexture), false, vertices.data());
}

void DemoFramework::CreateRenderTarget()
{
    DXGI_SWAP_CHAIN_DESC sd;
    m_pSwapChain->GetDesc(&sd);
    m_width = sd.BufferDesc.Width;
    m_height = sd.BufferDesc.Height;
    ID3D11Texture2D* pDepthStencil = NULL;
    {
        D3D11_TEXTURE2D_DESC descDepth;
        descDepth.Width = m_width;
        descDepth.Height = m_height;
        descDepth.MipLevels = 1;
        descDepth.ArraySize = 1;
        descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        descDepth.SampleDesc.Count = 1;
        descDepth.SampleDesc.Quality = 0;
        descDepth.Usage = D3D11_USAGE_DEFAULT;
        descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        descDepth.CPUAccessFlags = 0;
        descDepth.MiscFlags = 0;
        HRESULT hr = m_pd3dDevice->CreateTexture2D(&descDepth, NULL, &pDepthStencil);
        // Create the depth stencil view
        D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
        ZeroMemory(&descDSV, sizeof(descDSV));
        descDSV.Format = descDepth.Format;
        descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        descDSV.Texture2D.MipSlice = 0;;
        hr = m_pd3dDevice->CreateDepthStencilView(pDepthStencil, &descDSV, m_pDSView.ReleaseAndGetAddressOf());
    }

    {
        // Create the render target
        ID3D11Texture2D* pBackBuffer;
        D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
        ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
        render_target_view_desc.Format = sd.BufferDesc.Format;
        render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        m_pd3dDevice->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, m_mainRenderTargetView.ReleaseAndGetAddressOf());
        m_pd3dDeviceContext->OMSetRenderTargets(1, m_mainRenderTargetView.GetAddressOf(), m_pDSView.Get());
        pBackBuffer->Release();
    }

    pDepthStencil->Release();

}

void DemoFramework::CleanupRenderTarget()
{
    m_mainRenderTargetView.Reset();
    m_pDSView.Reset();
}

LRESULT WINAPI DemoFramework::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DemoFramework* instance = DemoFramework::GetInstance();
    static int lastMouseXPos = 0;
    static int lastMouseYPos = 0;
    switch (msg)
    {
    case WM_MOUSEMOVE:
    {
        if (wParam == MK_RBUTTON)
        {
            const int xPos = LOWORD(lParam);
            const int yPos = HIWORD(lParam);
            const int deltaX = lastMouseXPos - xPos;
            const int deltaY = lastMouseYPos - yPos;
            if (deltaX || deltaY)
                instance->m_camera.DeltaMouse(deltaX, deltaY);
            lastMouseXPos = xPos;
            lastMouseYPos = yPos;
        }
    }break;
    case WM_MOUSEWHEEL:
        instance->m_camera.WheelMouse(GET_WHEEL_DELTA_WPARAM(wParam));
        break;
    }

    if (ImGui_ImplDX11_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (instance->m_pd3dDevice != NULL && wParam != SIZE_MINIMIZED && instance->m_pSwapChain)
        {
            ImGui_ImplDX11_InvalidateDeviceObjects();
            UINT w, h;
            instance->m_width = w = LOWORD(lParam);
            instance->m_height = h = HIWORD(lParam);
            instance->CleanupRenderTarget();
            instance->m_pSwapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
            instance->CreateRenderTarget();
            ImGui_ImplDX11_CreateDeviceObjects();
            instance->m_camera.SetProj(70.0f*XM_PI / 180.0f, float(w) / h, 0.05f, 1024.0f);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void DemoFramework::RenderGui()
{
    for (auto& d : m_demos)
        if (d->m_running)
            d->OnRenderGui();

    ImGuiWindowFlags window_flags = 0;
    //window_flags |= ImGuiWindowFlags_ShowBorders;
    //window_flags |= ImGuiWindowFlags_NoTitleBar;
    //window_flags |= ImGuiWindowFlags_NoResize;
    //window_flags |= ImGuiWindowFlags_NoMove;
    //window_flags |= ImGuiWindowFlags_NoScrollbar;
    //window_flags |= ImGuiWindowFlags_NoCollapse;
    //window_flags |= ImGuiWindowFlags_MenuBar;
    static bool opened = true;
    ImGui::SetNextWindowPos(ImVec2(m_width - 500.0f, 10.0f), ImGuiSetCond_FirstUseEver);
    if (!ImGui::Begin("Demo Framework", &opened, ImVec2(400, 400), 0.65f, window_flags))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("Select a demo below");
    ImGui::Checkbox("Wireframe", &m_wireframe);
    for (auto& d : m_demos)
    {
        if (ImGui::CollapsingHeader(d->m_name.c_str()))
        {
            ImGui::TextWrapped(d->m_desc.c_str());
            ImGui::Spacing();
            ImGui::Text("Configure and press Run");
            if (!d->m_running)
            {
                if (ImGui::Button("Run"))
                {
                    // stop all others running
                    for (size_t i = 0; i < m_demos.size(); ++i)
                    {
                        auto otherDemo = m_demos[i];
                        if (otherDemo->m_running)
                        {
                            otherDemo->OnDestroyDemo();
                            otherDemo->m_running = false;
                        }
                    }

                    // start this one
                    d->m_running = true;
                    d->OnInitDemo();

                }
                ImGui::SameLine();
            }
            if (d->m_running && ImGui::Button("Stop"))
            {
                d->OnDestroyDemo();
                d->m_running = false;
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            if (!d->m_running)
                d->OnRenderGuiSetup();
        }
    }
    ImGui::End();
}

HRESULT DemoFramework::Frame()
{
    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                return E_FAIL;
            continue;
        }

        if (ImGui::GetIO().KeysDown['W'])
            m_camera.AdvanceForward(m_timeStep * 50.0f);
        else if (ImGui::GetIO().KeysDown['S'])
            m_camera.AdvanceForward(-m_timeStep * 50.0f);

        if (ImGui::GetIO().KeysDown['A'])
            m_camera.AdvanceRight(-m_timeStep * 50.0f);
        else if (ImGui::GetIO().KeysDown['D'])
            m_camera.AdvanceRight(m_timeStep * 50.0f);


        const float rgba[4] = { 0.0f, 0.0f, 0.3f, 1.0f };
        m_pd3dDeviceContext->ClearDepthStencilView(m_pDSView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        m_pd3dDeviceContext->ClearRenderTargetView(m_mainRenderTargetView.Get(), rgba);

        PreRender();
        for (auto& d : m_demos)
        {
            d->OnStepFramework();
            if (d->m_running)
                d->OnStepDemo();
        }
        PostRender();
        ImGui_ImplDX11_NewFrame();
        RenderGui();
        ImGui::Render();
        m_pSwapChain->Present(1, 0);
        m_stepCount++;
    }
    return S_OK;
}