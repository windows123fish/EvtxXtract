#include "gui_window.h"
#include "evtx_parser.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <d3d11.h>
#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "advapi32.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace fs = std::filesystem;

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2,
            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (FAILED(res))
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

GuiWindow::GuiWindow() : 
    m_hWnd(nullptr), 
    m_isRunning(false),
    m_selectedFileIndex(-1),
    m_isParsing(false),
    m_validChunkCount(0)
{
}

GuiWindow::~GuiWindow() {
    shutdown();
}

bool GuiWindow::init(HINSTANCE hInstance, const std::wstring& title, int width, int height) {
    WNDCLASSW wc{};
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = s_wndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.lpszClassName = L"EvtxXtract_GUI_Class";
    
    // 注册窗口类前先取消注册（如果已存在）
    UnregisterClassW(wc.lpszClassName, hInstance);
    
    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"窗口类注册失败", L"错误", MB_OK | MB_ICONERROR);
        return false;
    }

    RECT rect = {0, 0, width, height};
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
    
    m_hWnd = CreateWindowExW(
        WS_EX_ACCEPTFILES,
        wc.lpszClassName,
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, hInstance, this
    );

    if (!m_hWnd) {
        MessageBoxW(NULL, L"窗口创建失败", L"错误", MB_OK | MB_ICONERROR);
        return false;
    }

    if (!CreateDeviceD3D(m_hWnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, hInstance);
        return false;
    }

    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;

    // Setup fonts with Chinese support
    const char* font_paths[] = {
        "C:\\Windows\\Fonts\\msyh.ttc",
        "C:\\Windows\\Fonts\\simsun.ttc"
    };
    
    ImFont* loaded_font = nullptr;
    for (const char* font_path : font_paths) {
        if (fs::exists(font_path)) {
            loaded_font = io.Fonts->AddFontFromFileTTF(font_path, 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
            if (loaded_font) {
                break;
            }
        }
    }
    
    if (!loaded_font) {
        loaded_font = io.Fonts->AddFontDefault();
    }

    io.FontDefault = loaded_font;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(m_hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    scanEvtxFiles();
    
    m_isRunning = true;
    return true;
}

void GuiWindow::shutdown() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    
    if (m_hWnd) {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
}

void GuiWindow::run() {
    MSG msg{};
    
    while (msg.message != WM_QUIT && m_isRunning) {
        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            continue;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        render();
        
        ImGui::Render();
        const float clear_color[] = { 0.1f, 0.1f, 0.1f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }
}

void GuiWindow::render() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("EvtxXtract - EVTX文件解析器", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    
    // 可拖拽标题栏
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::BeginChild("TitleBar", ImVec2(0, 30), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::Text("EvtxXtract - EVTX文件解析器");
    ImGui::PopStyleColor();
    
    // 允许拖拽窗口
    if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && ImGui::GetMousePos().y < 30) {
        ReleaseCapture();
        SendMessageW(m_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
    }
    
    ImGui::EndChild();
    
    renderFileSelection();
    renderFileInfo();
    renderFooter();
    
    ImGui::End();
}

void GuiWindow::renderFileSelection() {
    ImGui::BeginChild("文件选择", ImVec2(300, ImGui::GetWindowHeight() - 130), true);
    
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "EVTX文件列表");
    ImGui::Separator();
    
    // 文件选择按钮
    if (ImGui::Button("选择文件")) {
        openFileDialog();
    }
    
    if (ImGui::Button("重新扫描")) {
        scanEvtxFiles();
    }
    
    ImGui::Separator();
    
    if (m_evtxFiles.empty()) {
        ImGui::Text("未找到EVTX文件");
    } else {
        for (size_t i = 0; i < m_evtxFiles.size(); i++) {
            bool isSelected = (m_selectedFileIndex == (int)i);
            ImGui::PushID((int)i);
            std::string filename = fs::path(m_evtxFiles[i]).filename().string();
            if (ImGui::Selectable(filename.c_str(), isSelected)) {
                m_selectedFileIndex = (int)i;
                m_isParsing = false;
                m_fileHeader = Evtx::EVT_FILE_HEADER();
                m_chunkInfo.clear();
                m_validChunkCount = 0;
            }
            ImGui::PopID();
        }
        
        ImGui::Separator();
        
        if (m_selectedFileIndex >= 0 && ImGui::Button("解析文件")) {
            parseSelectedFile();
        }
    }
    
    ImGui::EndChild();
}

void GuiWindow::renderFileInfo() {
    ImGui::SameLine();
    ImGui::BeginChild("文件信息", ImVec2(0, ImGui::GetWindowHeight() - 130), true);
    
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "文件信息");
    ImGui::Separator();
    
    if (!m_isParsing && m_fileHeader.magic[0] == 0) {
        ImGui::Text("请选择一个文件并点击\"解析文件\"");
    } else if (m_isParsing) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "正在解析文件...");
        ImGui::ProgressBar(0.5f, ImVec2(-1, 20), "处理中");
    } else {
        ImGui::Text("魔术数: %s", m_fileHeader.validate_magic() ? "有效 (ElfFile)" : "无效");
        
        uint16_t major = m_fileHeader.get_major_version();
        uint16_t minor = m_fileHeader.get_minor_version();
        ImGui::Text("版本: %u.%u", major, minor);
        
        ImGui::Text("标志位: 0x%04X", m_fileHeader.flags);
        ImGui::Text("块数量: %u", m_fileHeader.chunk_count);
        ImGui::Text("有效块数: %u", m_validChunkCount);
        
        if (m_fileHeader.chunk_count != m_validChunkCount) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "警告: 文件头块数与实际不符");
        }
        
        ImGui::Text("文件大小: %llu bytes", m_fileHeader.file_size);
        ImGui::Text("最旧块偏移: 0x%llX", m_fileHeader.oldest_chunk_offset);
        ImGui::Text("最新块偏移: 0x%llX", m_fileHeader.newest_chunk_offset);
        ImGui::Text("校验和: 0x%08X", m_fileHeader.checksum);
        
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "块信息");
        
        if (m_chunkInfo.empty()) {
            ImGui::Text("未解析块信息");
        } else {
            ImGui::BeginTable("chunks", 3, ImGuiTableFlags_Borders);
            ImGui::TableSetupColumn("块偏移");
            ImGui::TableSetupColumn("事件数");
            ImGui::TableSetupColumn("校验和");
            ImGui::TableHeadersRow();
            
            for (const auto& info : m_chunkInfo) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("0x%llX", info.offset);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%u", info.event_count);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("0x%08X", info.checksum);
            }
            ImGui::EndTable();
        }
    }
    
    ImGui::EndChild();
}

void GuiWindow::renderFooter() {
    ImGui::Separator();
    ImGui::Text("EvtxXtract v1.0.0 | 高性能EVTX文件流式解析器");
    ImGui::SameLine(ImGui::GetWindowWidth() - 150);
    ImGui::Text("点击窗口右上角关闭");
}

void GuiWindow::openFileDialog() {
    OPENFILENAMEW ofn{};
    wchar_t szFile[260] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"EVTX文件\0*.evtx\0所有文件\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    if (GetOpenFileNameW(&ofn) == TRUE) {
        std::wstring filepath = ofn.lpstrFile;
        // 检查是否已在列表中
        auto it = std::find(m_evtxFiles.begin(), m_evtxFiles.end(), filepath);
        if (it == m_evtxFiles.end()) {
            m_evtxFiles.push_back(filepath);
        }
        m_selectedFileIndex = static_cast<int>(std::find(m_evtxFiles.begin(), m_evtxFiles.end(), filepath) - m_evtxFiles.begin());
        m_isParsing = false;
        m_fileHeader = Evtx::EVT_FILE_HEADER();
        m_chunkInfo.clear();
        m_validChunkCount = 0;
    }
}

void GuiWindow::scanEvtxFiles() {
    m_evtxFiles.clear();
    
    const std::wstring default_dir = L"C:\\Windows\\System32\\winevt\\Logs\\";
    if (fs::exists(default_dir) && fs::is_directory(default_dir)) {
        for (const auto& entry : fs::directory_iterator(default_dir)) {
            if (entry.path().extension() == L".evtx") {
                m_evtxFiles.push_back(entry.path().wstring());
            }
        }
    }
    
    if (m_evtxFiles.empty()) {
        const std::wstring temp_dir = fs::temp_directory_path().wstring();
        const std::vector<std::wstring> log_names = {L"System", L"Application"};
        
        for (const auto& log_name : log_names) {
            std::wstring output_path = temp_dir + L"\\" + log_name + L"_export.evtx";
            try { fs::remove(output_path); } catch (...) {}
            
            std::wstring command = L"wevtutil epl " + log_name + L" \"" + output_path + L"\"";
            if (_wsystem(command.c_str()) == 0 && fs::exists(output_path)) {
                m_evtxFiles.push_back(output_path);
            }
        }
    }
}

void GuiWindow::parseSelectedFile() {
    if (m_selectedFileIndex < 0 || m_selectedFileIndex >= (int)m_evtxFiles.size()) {
        return;
    }
    
    m_isParsing = true;
    
    const std::wstring& filepath = m_evtxFiles[m_selectedFileIndex];
    
    try {
        Evtx::EvtxParser parser(filepath);
        if (!parser.open()) {
            MessageBoxW(m_hWnd, (L"无法打开文件: " + filepath).c_str(), L"错误", MB_OK | MB_ICONERROR);
            m_isParsing = false;
            return;
        }
        
        if (!parser.read_file_header()) {
            MessageBoxW(m_hWnd, L"读取文件头失败", L"错误", MB_OK | MB_ICONERROR);
            m_isParsing = false;
            return;
        }
        
        m_fileHeader = parser.get_file_header();
        m_validChunkCount = parser.validate_chunks();
        
        const auto& chunks = parser.get_valid_chunks();
        m_chunkInfo.clear();
        
        uint64_t current_offset = Evtx::EVTX_FILE_HEADER_SIZE;
        const uint64_t chunk_size = Evtx::EVTX_CHUNK_SIZE;
        
        for (const auto& chunk_header : chunks) {
            ChunkInfo info;
            info.offset = current_offset;
            info.event_count = static_cast<uint32_t>(chunk_header.last_event_record_number - chunk_header.first_event_record_number + 1);
            info.checksum = chunk_header.chunk_checksum;
            m_chunkInfo.push_back(info);
            current_offset += chunk_size;
        }
        
        m_fileHeader.file_size = fs::file_size(filepath);
        
    } catch (const std::exception& e) {
        std::wstring error_msg = L"解析错误: ";
        // MSVC下what()通常为ANSI编码(CP_ACP)，而非UTF-8
        std::string what = e.what();
        int wlen = MultiByteToWideChar(CP_ACP, 0, what.c_str(), -1, nullptr, 0);
        if (wlen > 0) {
            std::wstring wwhat(wlen, L'\0');
            MultiByteToWideChar(CP_ACP, 0, what.c_str(), -1, &wwhat[0], wlen);
            error_msg += wwhat;
        } else {
            error_msg += L"(无法转换错误信息)";
        }
        MessageBoxW(m_hWnd, error_msg.c_str(), L"错误", MB_OK | MB_ICONERROR);
    }
    
    m_isParsing = false;
}

bool GuiWindow::isAdmin() {
    BOOL is_admin = FALSE;
    SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;
    PSID admin_sid = nullptr;
    
    // 创建管理员组的SID
    if (!AllocateAndInitializeSid(
        &nt_authority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &admin_sid)) {
        return false;
    }
    
    // 检查当前进程是否属于管理员组
    CheckTokenMembership(NULL, admin_sid, &is_admin);
    
    FreeSid(admin_sid);
    return is_admin == TRUE;
}

bool GuiWindow::runAsAdmin() {
    wchar_t path[MAX_PATH];
    if (GetModuleFileNameW(NULL, path, MAX_PATH) == 0) {
        return false;
    }
    
    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = m_hWnd;
    sei.lpVerb = L"runas";
    sei.lpFile = path;
    sei.nShow = SW_NORMAL;
    
    if (!ShellExecuteExW(&sei)) {
        // 用户取消了UAC提示
        DWORD error = GetLastError();
        if (error == ERROR_CANCELLED) {
            MessageBoxW(m_hWnd, L"用户取消了管理员权限请求", L"提示", MB_OK | MB_ICONINFORMATION);
        }
        return false;
    }
    
    // 等待新进程启动后退出当前进程
    WaitForSingleObject(sei.hProcess, 500);
    CloseHandle(sei.hProcess);
    
    m_isRunning = false;
    PostQuitMessage(0);
    return true;
}

LRESULT CALLBACK GuiWindow::s_wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    GuiWindow* window = nullptr;
    
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        window = reinterpret_cast<GuiWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
        window = reinterpret_cast<GuiWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }
    
    if (window) {
        return window->wndProc(hWnd, msg, wParam, lParam);
    }
    
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT GuiWindow::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // 优先处理窗口关闭消息，避免被 ImGui 拦截
    if (msg == WM_CLOSE || msg == WM_DESTROY) {
        m_isRunning = false;
        PostQuitMessage(0);
        return 0;
    }
    
    // 处理 WM_SYSCOMMAND (点击窗口右上角按钮)
    if (msg == WM_SYSCOMMAND) {
        // SC_CLOSE = 0xF060，点击关闭按钮
        if ((wParam & 0xFFF0) == SC_CLOSE) {
            m_isRunning = false;
            PostQuitMessage(0);
            return 0;
        }
        // 其他系统命令交给 DefWindowProc 处理
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return 0;
    }
    
    switch (msg) {
        case WM_DROPFILES: {
            HDROP hDrop = (HDROP)wParam;
            UINT numFiles = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
            
            for (UINT i = 0; i < numFiles; i++) {
                wchar_t szFileName[MAX_PATH] = {0};
                DragQueryFileW(hDrop, i, szFileName, MAX_PATH);
                
                std::wstring filepath = szFileName;
                // 只处理 .evtx 文件
                if (fs::path(filepath).extension() == L".evtx") {
                    auto it = std::find(m_evtxFiles.begin(), m_evtxFiles.end(), filepath);
                    if (it == m_evtxFiles.end()) {
                        m_evtxFiles.push_back(filepath);
                    }
                    m_selectedFileIndex = static_cast<int>(std::find(m_evtxFiles.begin(), m_evtxFiles.end(), filepath) - m_evtxFiles.begin());
                    m_isParsing = false;
                    m_fileHeader = Evtx::EVT_FILE_HEADER();
                    m_chunkInfo.clear();
                    m_validChunkCount = 0;
                }
            }
            
            DragFinish(hDrop);
            return 0;
        }
            
        case WM_SIZE:
            if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;
            
        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}
