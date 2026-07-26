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