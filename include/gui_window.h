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

class