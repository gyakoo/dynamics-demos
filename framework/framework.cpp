#include "pch.h"
#include "framework.h"

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

        ID3D11RasterizerState* pRState = NULL;
        m_pd3dDevice->CreateRasterizerState(&RSDesc, &pRState);
        m_pd3dDeviceContext->RSSetState(pRState);
        pRState->Release(); // not going to need it anymore?

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

    InitResources();

    // camera
    m_view.CreateLookAt(Vector3(0, 0.0f, -5.0f), Vector3::Zero, Vector3::UnitY);
    m_proj.CreatePerspectiveFieldOfView(50.0f*XM_PI / 180.0f, float(width) / height, 0.05f, 100.0f);

    for (auto& d : m_demos)
        d->OnInitFramework();
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
    m_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pd3dDeviceContext->IASetInputLayout(m_ilPNT.Get());

    // VS
    XMFLOAT4X4 mvpData[3] = {m_view, m_proj, transform};
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
        m_pd3dDeviceContext->IASetIndexBuffer(rm.m_ib.Get(), DXGI_FORMAT_R32_UINT, 0);
        m_pd3dDeviceContext->DrawIndexed(rm.m_icount, 0, 0);        
    }
}

void DemoFramework::PreRender()
{
    const D3D11_RECT r = { 0,0,m_width,m_height };

    m_pd3dDeviceContext->RSSetScissorRects(1, &r);
}

void DemoFramework::PostRender()
{

}

RenderMaterial RenderMaterial::White( Vector4(1,0,0,1) );