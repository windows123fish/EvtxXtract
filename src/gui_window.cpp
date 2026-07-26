#include "gui_window.h"
#include "evtx_parser.h"
#include <imgui.h>
#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>

// Forward declarations
extern IMGUI_IMPL_API bool ImGui_ImplWin32_Init(HWND hWnd);
extern IMGUI_IMPL_API void ImGui_ImplWin32_Shutdown();
extern IMGUI_IMPL_API void ImGui_ImplWin32_NewFrame();
extern IMGUI_IMPL_API bool ImGui_ImplGDI_Init(HDC hDC);
extern IMGUI_IMPL_API void ImGui_ImplGDI_Shutdown();
extern IMGUI_IMPL_API void ImGui_ImplGDI_RenderDrawData(ImDrawData* draw_data);
extern IMGUI_IMPL_API void ImGui_ImplGDI_NewFrame();

namespace fs = std::filesystem;

GuiWindow::GuiWindow() : 
    m_hWnd(nullptr), 
    m_hDC(nullptr), 
    m_isRunning(false),
    m_selectedFileIndex(-1),
    m_isParsing(false),
    m_validChunkCount(0)
{
}

GuiWindow::~GuiWindow() {
    shutdown();
}

bool GuiWindow::init(HINSTANCE hInstance, const std::string& title, int width, int height) {
    // Create window
    WNDCLASSEXA wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = s_wndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "EvtxXtract_GUI_Class";
    
    if (!RegisterClassExA(&wc)) {
        MessageBoxA(NULL, "窗口类注册失败", "错误", MB_OK | MB_ICONERROR);
        return false;
    }

    RECT rect = {0, 0, width, height};
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
    
    m_hWnd = CreateWindowExA(
        0,
        wc.lpszClassName,
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, hInstance, this
    );

    if (!m_hWnd) {
        MessageBoxA(NULL, "窗口创建失败", "错误", MB_OK | MB_ICONERROR);
        return false;
    }

    m_hDC = GetDC(m_hWnd);
    
    // Initialize ImGui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL; // Disable .ini file
    
    // Build font atlas
    unsigned char* pixels;
    int width_, height_, bytes_per_pixel;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width_, &height_, &bytes_per_pixel);
    
    // Initialize backends
    ImGui_ImplWin32_Init(m_hWnd);
    ImGui_ImplGDI_Init(m_hDC);
    
    // Setup style
    ImGui::StyleColorsDark();
    
    // Scan for .evtx files
    scanEvtxFiles();
    
    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);
    m_isRunning = true;
    
    return true;
}

void GuiWindow::shutdown() {
    ImGui_ImplGDI_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    if (m_hDC) {
        ReleaseDC(m_hWnd, m_hDC);
        m_hDC = nullptr;
    }
    if (m_hWnd) {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
}

void GuiWindow::run() {
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    
    while (msg.message != WM_QUIT && m_isRunning) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            render();
        }
    }
}

void GuiWindow::render() {
    // Start ImGui frame
    ImGui_ImplGDI_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    // Main window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("EvtxXtract - EVTX文件解析器", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    
    // File selection panel
    renderFileSelection();
    
    // File info panel
    renderFileInfo();
    
    // Footer
