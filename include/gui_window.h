#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "evtx_structs.h"

// Forward declarations
struct EVT_FILE_HEADER;
struct EVT_CHUNK_HEADER;

struct ChunkInfo {
    uint64_t offset;
    uint32_t event_count;
    uint32_t checksum;
};

class GuiWindow {
public:
    GuiWindow();
    ~GuiWindow();
    
    bool init(HINSTANCE hInstance, const std::string& title, int width, int height);
    void shutdown();
    void run();
    
private:
    void render();
    void renderFileSelection();
    void renderFileInfo();
    void renderFooter();
    
    void scanEvtxFiles();
    void parseSelectedFile();
    
    LRESULT wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
