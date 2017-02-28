#pragma once
#define DEMO(demoClass, demoName, demoDesc) \
    struct _##demoClass \
    {\
        _##demoClass()\
        { \
            DemoFramework::GetInstance()->RegisterDemo<demoClass>(demoName, demoDesc);\
        }\
    }_##demoClass_Instance;

//
//
//
#define DEMOFRAMEWORK_MAIN() \
int CALLBACK wWinMain(_In_ HINSTANCE , _In_ HINSTANCE , _In_ LPWSTR, _In_ int )\
{\
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);\
    DemoFramework* demoFramework = DemoFramework::GetInstance();\
    if (SUCCEEDED(demoFramework->Init(L"DemoFramework", 1600, 900, false)))\
    {\
        while (SUCCEEDED(demoFramework->Frame()))\
        {\
        }\
    }\
    demoFramework->Destroy();\
    return 0;\
}

// Forward
class RenderMesh;
class RenderMaterial;
class DemoFramework;

extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND, UINT msg, WPARAM wParam, LPARAM lParam);

class Demo
{
public:
    Demo()
        : m_framework(nullptr)
        , m_running(false)
    {}

    virtual void OnInitDemo() {};
    virtual void OnDestroyDemo() {};
    virtual void OnStepDemo() {};
    virtual void OnRenderGui() {};
    virtual void OnRenderGuiSetup() {};

    virtual void OnInitFramework() {};
    virtual void OnDestroyFramework() {};
    virtual void OnStepFramework() {};
    virtual void OnWndProc(HWND, UINT, WPARAM, LPARAM) {}

    DemoFramework* m_framework;
    std::string m_name;
    std::string m_desc;
    bool m_running;
};

class Stopwatch
{
public:
    Stopwatch()
    {
        QueryPerformanceFrequency(&m_ticksPerSecond);
    }

    void Start()
    {
        QueryPerformanceCounter(&m_t0);
    }

    double Stop()
    {
        LARGE_INTEGER t1;
        QueryPerformanceCounter(&t1);
        return (t1.QuadPart - m_t0.QuadPart) * 1000.0 / m_ticksPerSecond.QuadPart;
    }

    LARGE_INTEGER m_ticksPerSecond;
    LARGE_INTEGER m_t0;
};

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

class Camera
{
public:
    Camera()
        : m_pitchYawRoll(Vector3::Zero)
    {
    }

    void SetView(const Vector3& eye, const Vector3& at, const Vector3& up = Vector3::UnitY);
    void SetProj(float fovYRad, float aspect, float _near, float _far);
    void DeltaMouse(int dx, int dy);
    void AdvanceForward(float d);
    void AdvanceRight(float d);
    void _ComputeView();

    Matrix m_view;
    Matrix m_proj;
    Vector3 m_pitchYawRoll;
    Vector3 m_position;
};

class DemoFramework
{
public:

    static inline DemoFramework* GetInstance()
    {
        static DemoFramework s_instance;
        return &s_instance;
    }

    DemoFramework()
        : m_pd3dDevice(nullptr)
        , m_pd3dDeviceContext(nullptr)
        , m_pSwapChain(nullptr)
        , m_stepCount(0)
    {
    }

    ~DemoFramework()
    {
        Destroy();
    }

    HRESULT Init(const wchar_t* title, int width, int height, bool fullscreen);
    HRESULT InitResources();
    void PreRender();
    void PostRender();
    void Destroy();

    HRESULT Frame()
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

            if ( ImGui::GetIO().KeysDown['W'] )
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
                if ( d->m_running )
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

    void RenderGui()
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
        ImGui::SetNextWindowPos(ImVec2(m_width - 500, 10), ImGuiSetCond_FirstUseEver);
        if (!ImGui::Begin("Demo Framework", &opened, ImVec2(400, 400), 0.65f, window_flags))
        {
            ImGui::End();
            return;
        }

        ImGui::Text("Select a demo below");

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

    void RenderObj(const RenderMesh& rm, const RenderMaterial& mat, const Matrix& transform);
    void CreateRenderSphere(float radius, int nsubdiv, RenderMesh& outMesh);
    
    std::vector<std::shared_ptr<Demo>> m_demos;
    
    template<typename T>
    void RegisterDemo(const std::string& name, const std::string& desc)
    {
        auto demo = std::make_shared<T>();
        demo->m_name = name;
        demo->m_desc = desc;
        demo->m_framework = this;
        m_demos.push_back(demo);
    }

    int                      m_width, m_height;
    float                    m_timeStep;
    int                      m_stepCount;
    ID3D11Device*            m_pd3dDevice;
    ID3D11DeviceContext*     m_pd3dDeviceContext;
    IDXGISwapChain*          m_pSwapChain;
    ComPtr<ID3D11RenderTargetView>  m_mainRenderTargetView;
    ComPtr<ID3D11DepthStencilView>  m_pDSView;
    ComPtr<ID3D11VertexShader> m_vsPNT;
    ComPtr<ID3D11PixelShader> m_psPNT;
    ComPtr<ID3D11InputLayout> m_ilPNT;
    ComPtr<ID3D11Buffer>      m_cbMVP;
    ComPtr<ID3D11Buffer>      m_cbF4;
    ComPtr<ID3D11SamplerState> m_linearClamp;
    ComPtr<ID3D11BlendState> m_opaque;
    Camera m_camera;
    Stopwatch m_stopWatch;
protected:

    void CreateRenderTarget()
    {
        DXGI_SWAP_CHAIN_DESC sd;
        m_pSwapChain->GetDesc(&sd);

        ID3D11Texture2D* pDepthStencil = NULL;
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
        //descDSV.Texture2DMS.UnusedField_NothingToDefine = 0;  
        hr = m_pd3dDevice->CreateDepthStencilView(pDepthStencil, &descDSV, m_pDSView.ReleaseAndGetAddressOf());


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
        pDepthStencil->Release();
    }

    void CleanupRenderTarget()
    {
        m_mainRenderTargetView.Reset();        
        m_pDSView.Reset();
    }

    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
        }

        if (ImGui_ImplDX11_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;
        
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
#define D3DDEVICE (DemoFramework::GetInstance()->m_pd3dDevice)
#define D3DCONTEXT (DemoFramework::GetInstance()->m_pd3dDeviceContext)

class RenderMesh
{
public:
    RenderMesh()
        : m_vcount(0), m_vsize(0), m_vdyn(false), m_icount(0)
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
        m_vsize = vertexSize;
        m_vdyn = dynamic;
        _CreateBuff(numVertices, vertexSize, D3D11_BIND_VERTEX_BUFFER, 
            dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
            initData, m_vb);
    }

    inline void CreateIB(int numIndices, int indexSize, void* initData=nullptr)
    {
        m_icount = numIndices;
        m_isize = indexSize;
        _CreateBuff(numIndices, indexSize, D3D11_BIND_INDEX_BUFFER,
            D3D11_USAGE_DEFAULT, initData, m_ib);        
    }

    inline void UpdateVB(void* data) 
    { 
        if (m_vdyn)
            _UpdateBuff(data, m_vb);
        else
            CreateVB(m_vcount, m_vsize, m_vdyn, data);
    } // entire subresource
    
    ComPtr<ID3D11Buffer> m_vb;
    ComPtr<ID3D11Buffer> m_ib;
    int m_vcount, m_vsize;
    int m_icount;
    int m_isize;
    bool m_vdyn;

private:
    void _CreateBuff(int n, int s, int f0, int f1, void* id, ComPtr<ID3D11Buffer>& b)
    {
        D3D11_SUBRESOURCE_DATA bd = { 0 };
        bd.pSysMem = id;
        bd.SysMemPitch = 0;
        bd.SysMemSlicePitch = 0;
        CD3D11_BUFFER_DESC extbd((UINT)(n*s),(UINT)f0, (D3D11_USAGE)f1, f1 ? D3D11_CPU_ACCESS_WRITE : 0);
        HRESULT hr = D3DDEVICE->CreateBuffer(&extbd, &bd, b.ReleaseAndGetAddressOf());
        assert(hr == S_OK);
    }

    inline void _UpdateBuff(void* data, ComPtr<ID3D11Buffer>& b)
    {
        D3DCONTEXT->UpdateSubresource(b.Get(), 0, 0, data, 0, 0);
    }
};

class RenderMaterial
{
public:
    RenderMaterial(const Vector4& modColor)
        : m_modulateColor(modColor)
    {
    }

    Vector4 m_modulateColor;
    ComPtr<ID3D11Texture2D> m_texture0;
    ComPtr<ID3D11ShaderResourceView> m_srv0;

    static RenderMaterial White;
};
