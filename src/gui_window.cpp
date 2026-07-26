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

bool GuiWindow::init(HINSTANCE hInstance, const