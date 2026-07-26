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
    if (!file.is_open()) {
        std::cout << "Failed to open file: " << argv[1] << std::endl;
        return 1;
    }
    
    // Skip file header (4096 bytes)
    file.seekg(4096);
    
    EVT_CHUNK_HEADER header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    
    std::cout << "=== Chunk Header Hex Dump (First 64 bytes) ===" << std::endl;
    print_hex(reinterpret_cast<uint8_t*>(&header), 64);
    
    std::cout << "\n=== Struct Fields ===" << std::endl;
    std::cout << "magic: " << std::string(reinterpret_cast<char*>(header.magic), 7) << std::endl;
    std::cout << "first_event_record_number: " << std::dec << header.first_event_record_number << std::endl;
    std::cout << "last_event_record_number: " << header.last_event_record_number << std::endl;
    std::cout << "first_event_record_id: " << header.first_event_record_id << std::endl;
    std::cout << "last_event_record_id: " << header.last_event_record_id << std::endl;
    std::cout << "header_size: 0x" << std::hex << header.header_size << std::endl;
    std::cout << "last_event_offset: 0x" << header.last_event_offset << std::endl;
    std::cout << "free_space_offset: 0x" << header.free_space_offset << std::endl;
    std::cout << "events_checksum: 0x" << header.events_checksum << std::endl;
    std::cout << "unknown1: 0x" << header.unknown1 << std::endl;
    std::cout << "flags: 0x" << header.flags << std::endl;
    std::cout << "chunk_checksum: 0x" << header.chunk_checksum << std::endl;
    std::cout << "string_offset_array_offset: 0x" << header.string_offset_array_offset << std::endl;
    
    return 0;
}
