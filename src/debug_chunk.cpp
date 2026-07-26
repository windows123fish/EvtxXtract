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
    uint64_t last_event_record_id;
    uint32_t header_size;
    uint32_t last_event_offset;
    uint32_t free_space_offset;
    uint32_t events_checksum;
    uint32_t unknown1;
    uint32_t flags;
    uint32_t chunk_checksum;
    uint32_t string_offset_array_offset;
    uint8_t reserved[440];
};
#pragma pack(pop)

void print_hex(const uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; i += 16) {
        std::cout << std::hex << std::setw(8) << std::setfill('0') << i << ": ";
        for