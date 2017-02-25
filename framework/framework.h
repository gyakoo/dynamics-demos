// Forward
class RenderMesh;
class RenderMaterial;

extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND, UINT msg, WPARAM wParam, LPARAM lParam);
class DemoFramework
{
public:
    DemoFramework()
        : m_mainRenderTargetView(nullptr)
        , m_pd3dDevice(nullptr)
        , m_pd3dDeviceContext(nullptr)
        , m_pSwapChain(nullptr)
    {
        s_instance = this;

    }
    ~DemoFramework()
    {
        Destroy();
    }

    HRESULT Init(const wchar_t* title, int width, int height, bool fullscreen)
    {
        // Create application window
        WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DemoFramework::WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, L"demoframework", NULL };
        RegisterClassEx(&wc);
        HWND hwnd = CreateWindow(L"demoframework", title, WS_OVERLAPPEDWINDOW, 100, 100, width, height, NULL, NULL, wc.hInstance, NULL);

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
            pRState->Release();
        }

        // Show the window
        ShowWindow(hwnd, SW_SHOWDEFAULT);
        UpdateWindow(hwnd);

        // Setup ImGui binding
        ImGui_ImplDX11_Init(hwnd, m_pd3dDevice, m_pd3dDeviceContext);

        OnInit();
        return S_OK;
    }

    void Destroy()
    {
        OnDestroy();
        ImGui_ImplDX11_Shutdown();
        CleanupRenderTarget();
        if (m_pSwapChain) { m_pSwapChain->Release(); m_pSwapChain = NULL; }
        if (m_pd3dDeviceContext) { m_pd3dDeviceContext->Release(); m_pd3dDeviceContext = NULL; }
        if (m_pd3dDevice) { m_pd3dDevice->Release(); m_pd3dDevice = NULL; }
        UnregisterClass(L"demoframework", NULL);
    }

    HRESULT Frame()
    {
        bool show_test_window = true;
        bool show_another_window = false;
        ImVec4 clear_col = ImColor(114, 144, 154);

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

            OnFrame();

            ImGui_ImplDX11_NewFrame();

            // 1. Show a simple window
            // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
            {
                static float f = 0.0f;
                ImGui::Text("Hello, world!");
                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
                ImGui::ColorEdit3("clear color", (float*)&clear_col);
                if (ImGui::Button("Test Window")) show_test_window ^= 1;
                if (ImGui::Button("Another Window")) show_another_window ^= 1;
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            }

            // 2. Show another simple window, this time using an explicit Begin/End pair
            if (show_another_window)
            {
                ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
                ImGui::Begin("Another Window", &show_another_window);
                ImGui::Text("Hello");
                ImGui::End();
            }

            // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
            if (show_test_window)
            {
                ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);     // Normally user code doesn't need/want to call it because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
                ImGui::ShowTestWindow(&show_test_window);
            }

            m_pd3dDeviceContext->ClearRenderTargetView(m_mainRenderTargetView, (float*)&clear_col);
            ImGui::Render();
            m_pSwapChain->Present(0, 0);
        }
        return S_OK;
    }

    void RenderObj(const RenderMesh& rm, const RenderMaterial& mat);

    virtual void OnFrame() {};
    virtual void OnInit() {};
    virtual void OnDestroy() {};
    virtual void OnWndProc(HWND , UINT , WPARAM , LPARAM ) {}

    static DemoFramework* s_instance;

    ID3D11Device*            m_pd3dDevice;
    ID3D11DeviceContext*     m_pd3dDeviceContext;
    IDXGISwapChain*          m_pSwapChain;
    ID3D11RenderTargetView*  m_mainRenderTargetView;
protected:

    void CreateRenderTarget()
    {
        DXGI_SWAP_CHAIN_DESC sd;
        m_pSwapChain->GetDesc(&sd);

        // Create the render target
        ID3D11Texture2D* pBackBuffer;
        D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
        ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
        render_target_view_desc.Format = sd.BufferDesc.Format;
        render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        m_pd3dDevice->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &m_mainRenderTargetView);
        m_pd3dDeviceContext->OMSetRenderTargets(1, &m_mainRenderTargetView, NULL);
        pBackBuffer->Release();
    }

    void CleanupRenderTarget()
    {
        if (m_mainRenderTargetView) { m_mainRenderTargetView->Release(); m_mainRenderTargetView = NULL; }
    }

    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplDX11_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        DemoFramework* instance = DemoFramework::s_instance;
        switch (msg)
        {
        case WM_SIZE:
            if (instance->m_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
            {
                ImGui_ImplDX11_InvalidateDeviceObjects();
                instance->CleanupRenderTarget();

                instance->m_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                instance->CreateRenderTarget();
                ImGui_ImplDX11_CreateDeviceObjects();
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
};
#define D3DDEVICE (DemoFramework::s_instance->m_pd3dDevice)

class RenderMesh
{
public:
    RenderMesh()
        : m_vcount(0), m_icount(0)
    {
    }
    
    ~RenderMesh()
    {
        Destroy();
    }

    void Destroy()
    {
        m_vb.Reset();
        m_vb.Reset();
    }

    inline void CreateVB(int numVertices, int vertexSize, bool dynamic=true, void* initData=nullptr)
    {
        m_vcount = numVertices;
        _CreateBuff(numVertices, vertexSize, D3D11_BIND_VERTEX_BUFFER, 
            dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
            initData, m_vb);
    }

    inline void CreateIB(int numIndices, int indexSize, void* initData=nullptr)
    {
        m_icount = numIndices;
        _CreateBuff(numIndices, indexSize, D3D11_BIND_VERTEX_BUFFER,
            D3D11_USAGE_DEFAULT, initData, m_ib);        
    }
    
    ComPtr<ID3D11Buffer> m_vb;
    ComPtr<ID3D11Buffer> m_ib;
    int m_vcount;
    int m_icount;

private:
    void _CreateBuff(int n, int s, int f0, int f1, void* id, ComPtr<ID3D11Buffer>& b)
    {
        D3D11_SUBRESOURCE_DATA bd = { 0 };
        bd.pSysMem = id;
        bd.SysMemPitch = 0;
        bd.SysMemSlicePitch = 0;
        CD3D11_BUFFER_DESC extbd((UINT)(n*s),(UINT)f0, (D3D11_USAGE)f1);
        HRESULT hr = D3DDEVICE->CreateBuffer(&extbd, id ? &bd : nullptr,
            b.ReleaseAndGetAddressOf());
        assert(hr == S_OK);
    }
};

class RenderMaterial
{
public:
};

#define IMPLEMENT_DEMO(demoClass) \
DemoFramework* DemoFramework::s_instance = nullptr;\
int CALLBACK wWinMain(_In_ HINSTANCE , _In_ HINSTANCE , _In_ LPWSTR lpCmdLine, _In_ int )\
{\
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON); \
    demoClass m_renderer;\
    if (SUCCEEDED(m_renderer.Init(L#demoClass, 1600, 900, false)))\
    {\
        while (SUCCEEDED(m_renderer.Frame()))\
        {\
        }\
    }\
    m_renderer.Destroy();\
    return 0;\
}


void DemoFramework::RenderObj(const RenderMesh& rm, const RenderMaterial& mat)
{

}
