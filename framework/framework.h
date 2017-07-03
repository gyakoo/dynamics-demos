#pragma once
#include "camera.h"
#include "stopwatch.h"
#include "graphics.h"

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
    Stopwatch m_clock;
    bool m_running;
};


class DemoFramework
{
public:

    static inline DemoFramework* GetInstance()
    {
        static DemoFramework s_instance;
        return &s_instance;
    }

    DemoFramework();
    ~DemoFramework();

    HRESULT Init(const wchar_t* title, int width, int height, bool fullscreen);
    HRESULT InitResources();
    void PreRender();
    void PostRender();
    void Destroy();
    HRESULT Frame();
    void RenderGui();
    void RenderObj(const RenderMesh& rm, const RenderMaterial& mat, const Matrix& transform);
    void CreateRenderSphere(float radius, int nsubdiv, RenderMesh& outMesh);
    void CreateRenderBox(const Vector3& halfExtents, RenderMesh& outMesh);
    template<typename T>
    void RegisterDemo(const std::string& name, const std::string& desc)
    {
        auto demo = std::make_shared<T>();
        demo->m_name = name;
        demo->m_desc = desc;
        demo->m_framework = this;
        m_demos.push_back(demo);
    }

    std::vector<std::shared_ptr<Demo>> m_demos;
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
    ComPtr<ID3D11RasterizerState> m_fillSolid;
    ComPtr<ID3D11RasterizerState> m_fillWireframe;
    bool m_wireframe;
    Camera m_camera;
    Stopwatch m_stopWatch;

protected:
    void CreateRenderBox(const Vector3& halfExtents, std::vector<VertexPositionNormalTexture>& outVertices);
    void CreateRenderTarget();
    void CleanupRenderTarget();
    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#define DEMO(demoClass, demoName, demoDesc) \
    struct _##demoClass \
    {\
        _##demoClass()\
        { \
            DemoFramework::GetInstance()->RegisterDemo<demoClass>(demoName, demoDesc);\
        }\
    }_##demoClass_Instance;


#define DEMOFRAMEWORK_MAIN() \
int CALLBACK wWinMain(_In_ HINSTANCE , _In_ HINSTANCE , _In_ LPWSTR, _In_ int )\
{\
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);\
    DemoFramework* demoFramework = DemoFramework::GetInstance();\
    if (SUCCEEDED(demoFramework->Init(L"DemoFramework", 1920, 1080, FALSE)))\
    {\
        while (SUCCEEDED(demoFramework->Frame()))\
        {\
        }\
    }\
    demoFramework->Destroy();\
    return 0;\
}

#define D3DDEVICE (DemoFramework::GetInstance()->m_pd3dDevice)
#define D3DCONTEXT (DemoFramework::GetInstance()->m_pd3dDeviceContext)