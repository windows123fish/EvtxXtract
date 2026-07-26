#include <windows.h>
#include "gui_window.h"

GuiWindow* g_pWindow = nullptr;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    g_pWindow = new GuiWindow();
    
    if