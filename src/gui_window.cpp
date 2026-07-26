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
extern IMGUI_IMPL_API void ImGui_ImplGDI_Sh