#include <imgui.h>
#include <windows.h>

static HDC              g_hDC = NULL;
static HBITMAP          g_hBitmap = NULL;
static HBITMAP          g_hBitmapOld = NULL;
static int              g_Width = 0;
static int              g_Height = 0;
static unsigned char*   g_Buffer = NULL;

// Initialize GDI rendering
IMGUI_IMPL_API bool ImGui_ImplGDI_Init(HDC hDC) {
    g_hDC = hDC;
    g_hBitmap = NULL;
    g_hBitmapOld = NULL;
    g_Width = 0;
    g_Height = 0;
    g_Buffer = NULL;
    return true;
}

// Shutdown GDI rendering
IMGUI_IMPL_API void ImGui_ImplGDI_Shutdown() {
    if (g_hBitmap) {
