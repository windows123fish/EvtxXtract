#include <windows.h>
#include "gui_window.h"

GuiWindow* g_pWindow = nullptr;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    g_pWindow = new GuiWindow();
    
    if (!g_pWindow->init(hInstance, L"EvtxXtract - EVTX文件解析器", 1200, 800)) {
        delete g_pWindow;
        return 1;
    }
    
    g_pWindow->run();
    
    delete g_pWindow;
    return 0;
}

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) {
    return TRUE;
}
