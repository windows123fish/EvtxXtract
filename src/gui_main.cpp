#include <windows.h>
#include "gui_window.h"

// Global window instance
GuiWindow* g_pWindow = nullptr;

// WinMain entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Create window
    g_pWindow = new GuiWindow();
    
    if (!g_pWindow