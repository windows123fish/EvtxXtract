#include <imgui.h>
#include <windows.h>

// Forward declarations
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static HWND                 g_hWnd = NULL;
static HDC                  g_hDC = NULL;
static ImGuiMouseCursor     g_LastMouseCursor = ImGuiMouseCursor_COUNT;
static bool                 g_bCursorDisabled = false;
static bool