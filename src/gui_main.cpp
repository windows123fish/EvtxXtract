#include <windows.h>
#include "gui_window.h"

// Global window instance
GuiWindow* g_pWindow = nullptr;

// WinMain entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Create window
    g_pWindow = new GuiWindow();
    
    if (!g_pWindow->init(hInstance, "EvtxXtract - EVTX文件解析器", 1200, 800)) {
        delete g_pWindow;
        return 1;
    }
    
    // Run main loop
    g_pWindow->run();
    
    // Cleanup
    delete g_pWindow;
    return 0;
}

// DllMain - required for Windows DLL/EXE
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH: