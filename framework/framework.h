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

// Vertex struct holding position, normal vector, and texture mapping information.
struct VertexPositionNormalTexture
{
    VertexPositionNormalTexture() = default;

    VertexPositionNormalTexture(XMFLOAT3 const& position, XMFLOAT3 const& normal, XMFLOAT2 const& textureCoordinate)
        : position(position),
        normal(normal),
        textureCoordinate(textureCoordinate)
    { }

    VertexPositionNormalTexture(FXMVECTOR position, FXMVECTOR normal, FXMVECTOR textureCoordinate)
    {
        XMStoreFloat3(&this->position, position);
        XMStoreFloat3(&this->normal, normal);
        XMStoreFloat2(&this->textureCoordinate, textureCoordinate);
    }

    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT2 textureCoordinate;

    static const int InputElementCount = 3;
    static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};

class DemoFramework
{
public:

    static DemoFramework* GetInstance()
    {
        static DemoFramework s_instance;
        return &s_instance;
    }

    DemoFramework()
        : m_mainRenderTargetView(nullptr)
        , m_pd3dDevice(nullptr)
        , m_pd3dDeviceContext(nullptr)
        , m_pSwapChain(nullptr)
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
            const float rgba[4] = { 0.5f, 0.5f, 0.9f, 1.0f };
            m_pd3dDeviceContext->ClearRenderTargetView(m_mainRenderTargetView, rgba);

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
            m_pSwapChain->Present(0, 0);
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

    int m_width, m_height;
    ID3D11Device*            m_pd3dDevice;
    ID3D11DeviceContext*     m_pd3dDeviceContext;
    IDXGISwapChain*          m_pSwapChain;
    ID3D11RenderTargetView*  m_mainRenderTargetView;
    ComPtr<ID3D11VertexShader> m_vsPNT;
    ComPtr<ID3D11PixelShader> m_psPNT;
    ComPtr<ID3D11InputLayout> m_ilPNT;
    ComPtr<ID3D11Buffer>      m_cbMVP;
    ComPtr<ID3D11Buffer>      m_cbF4;
    ComPtr<ID3D11SamplerState> m_linearClamp;

    Matrix m_view;
    Matrix m_proj;
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

        DemoFramework* instance = DemoFramework::GetInstance();
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
    bool m_vdyn;

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

    inline void _UpdateBuff(void* data, ComPtr<ID3D11Buffer>& b)
    {
        D3DCONTEXT->UpdateSubresource(b.Get(), 0, 0, data, 0, 0);
    }
};

class RenderMaterial
{
public:
    RenderMaterial(const Vector4& modColor)
        : m_modulateColor(1,1,1,1)
    {
    }

    Vector4 m_modulateColor;
    ComPtr<ID3D11Texture2D> m_texture0;
    ComPtr<ID3D11ShaderResourceView> m_srv0;

    static RenderMaterial White;
};
