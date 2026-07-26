#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstring>

#pragma pack(push, 1)
struct EVT_CHUNK_HEADER {
    uint8_t magic[8];
    uint64_t first_event_record_number;
    uint64_t last_event_record_number;
    uint64_t first_event_record_id;
    uint64_t last_event_record