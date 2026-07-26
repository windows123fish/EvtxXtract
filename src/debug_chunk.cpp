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
        for (size_t j = 0; j < 16 && i + j < size; ++j) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i + j]) << " ";
        }
        std::cout << " | ";
        for (size_t j = 0; j < 16 && i + j < size; ++j) {
            char c = data[i + j];
            std::cout << (c >= 32 && c < 127 ? c : '.');
        }
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: debug_chunk <evtx_file>" << std::endl;
        return 1;
    }
    
    std::ifstream file(argv[1], std::ios::binary);
    if (!file.is_open())