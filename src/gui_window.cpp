#include "gui_window.h"
#include "evtx_parser.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <sstream>

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
        MessageBoxA(NULL, "窗口类