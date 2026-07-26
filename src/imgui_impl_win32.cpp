#include <imgui.h>
#include <windows.h>

// Forward declarations
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static HWND                 g_hWnd = NULL;
static HDC                  g_hDC = NULL;
static ImGuiMouseCursor     g_LastMouseCursor = ImGuiMouseCursor_COUNT;
static bool                 g_bCursorDisabled = false;
static bool                 g_bPrevMouseVisible = true;

// Helper to create a window class
static WNDCLASSEXA CreateWindowClass(HINSTANCE hInstance) {
    WNDCLASSEXA wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = ImGui_ImplWin32_WndProcHandler;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "ImGui_Window_Class";
    return wc;
}

// Create the main window
IMGUI_IMPL_API HWND ImGui_ImplWin32_CreateWindow(HINSTANCE hInstance, const char* title, int width, int height) {
    WNDCLASSEXA wc = CreateWindowClass(hInstance);
    RegisterClassExA(&wc);

    // Calculate window size to get client area of desired size
    RECT rect = {0, 0, width, height};
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
    
    g_hWnd = CreateWindowExA(
        0,
        wc.lpszClassName,
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, hInstance, NULL
    );

    if (g_hWnd) {
        g_hDC = GetDC(g_hWnd);
        ShowWindow(g_hWnd, SW_SHOW);
        UpdateWindow(g_hWnd);
    }

    return g_hWnd;
}

// Destroy the window
IMGUI_IMPL_API void ImGui_ImplWin32_DestroyWindow() {
    if (g_hDC) {
        ReleaseDC(g_hWnd, g_hDC);
        g_hDC = NULL;
    }
    if (g_hWnd) {
        DestroyWindow(g_hWnd);
        g_hWnd = NULL;
    }
}

// Initialize ImGui for Win32
IMGUI_IMPL_API bool ImGui_ImplWin32_Init(HWND hWnd) {
    g_hWnd = hWnd;
    g_hDC = GetDC(hWnd);
    
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // Setup back-end capabilities flags
    io.BackendPlatformName = "imgui_impl_win32";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    
    return true;
}

// Shutdown ImGui for Win32
IMGUI_IMPL_API void ImGui_ImplWin32_Shutdown() {
    if (g_hDC) {
        ReleaseDC(g_hWnd, g_hDC);
        g_hDC = NULL;
    }
    g_hWnd = NULL;
}

// New frame
IMGUI_IMPL_API void ImGui_ImplWin32_NewFrame() {
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built!");

    // Set display size
    RECT rect;
    GetClientRect(g_hWnd, &rect);
    io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

    // Set time
    static double time = 0.0;
    if (time == 0.0) {
        LARGE_INTEGER freq, counter;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&counter);
        time = (double)counter.QuadPart / freq.QuadPart;
    }
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    double current_time = (double)counter.QuadPart / freq.QuadPart;
    io.DeltaTime = (float)(current_time - time);
    time = current_time;

    // Set mouse position
    POINT pos;
    GetCursorPos(&pos);
    ScreenToClient(g_hWnd, &pos);
    io.MousePos = ImVec2((float)pos.x, (float)pos.y);

    // Set mouse buttons
    io.MouseDown[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    io.MouseDown[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    io.MouseDown[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;

    // Set mouse wheel
    // Wheel messages are handled in WndProc
}

// Update mouse cursor
static void ImGui_ImplWin32_UpdateMouseCursor() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
        // Hide OS mouse cursor if ImGui is drawing it
        if (g_bPrevMouseVisible) {
            g_bPrevMouseVisible = false;
            ShowCursor(false);
        }
    } else {
        // Show OS mouse cursor
        if (!g_bPrevMouseVisible) {
            g_bPrevMouseVisible = true;
            ShowCursor(true);
        }

        // Change OS mouse cursor
        if (imgui_cursor != g_LastMouseCursor) {
            g_LastMouseCursor = imgui_cursor;
            LPCTSTR cursor_id = IDC_ARROW;
            switch (imgui_cursor) {
                case ImGuiMouseCursor_Arrow:        cursor_id = IDC_ARROW; break;
                case ImGuiMouseCursor_TextInput:    cursor_id = IDC_IBEAM; break;
                case ImGuiMouseCursor_ResizeAll:    cursor_id = IDC_SIZEALL; break;
                case ImGuiMouseCursor_ResizeNS:     cursor_id = IDC_SIZENS; break;
                case ImGuiMouseCursor_ResizeEW:     cursor_id = IDC_SIZEWE; break;
                case ImGuiMouseCursor_ResizeNESW:   cursor_id = IDC