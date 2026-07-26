#include <cstdint>
#include <fstream>
#include <iostream>
#include <iomanip>

void print_hex(const uint8_t* data, size_t size, size_t bytes_per_line = 16) {
  for (size_t i = 0; i < size; i += bytes_per_line) {
    std::cout << std::hex << std::setw(8) << std::setfill('0') << i << ": ";
    
    for (size_t j = 0; j < bytes_per_line && i + j < size; ++j) {
      std::cout << std::setw(2) << std::setfill('0') << static_cast<int>(data[i + j]) << " ";
    }
    
    std::cout << " ";
    
    for (size_t j = 0; j < bytes_per_line && i + j < size; ++j) {
      char c = data[i + j];
      if (c >= 0x20 && c < 0x7F) {
        std::cout << c;
      } else {
        std::cout << ".";
      }
    }
    
    std::cout << "\n";
  }
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <evtx_file_path>\n";
    return 1;
  }

  std::ifstream file(argv[1], std::ios::binary);
  if (!file) {
    std::cerr << "Failed to open file\n";
    return 1;
  }

  // Read first 128 bytes of file header
  uint8_t header[128];
  file.read(reinterpret_cast<char*>(header), sizeof(header));
  
  std::cout << "=== EVTX File Header - First 128 bytes ===\n";
  print_hex(header, sizeof(header));
  
  // Also read first chunk header
  file.seekg(4096);
  uint8_t chunk_header[128];
  file.read(reinterpret_cast<char*>(chunk_header), sizeof(chunk_header));
  
  std::cout << "\n=== First Chunk Header - First 128 bytes ===\n";
  print_hex(chunk_header, sizeof(chunk_header));
  
  // Try different offset interpretations
  std::cout << "\n=== Field Interpretation Tests ===\n";
  
  // Test: What if version is at offset 0x2C?
  uint32_t version_at_2c = *reinterpret_cast<uint32_t*>(&header[44]);
  std::cout << "version at offset 0x2C: 0x" << std::hex << version_at_2c << " (" << ((version_at_2c >> 16) & 0xFFFF) << "." << (version_at_2c & 0xFFFF) << ")\n";
  
  // Test: What if chunk_count is at offset 0x28?
  uint16_t chunk_count_at_28 = *reinterpret_cast<uint16_t*>(&header[40]);
  std::cout << "chunk_count at offset 0x28: " << std::dec << chunk_count_at_28 << "\n";
  
  // Test: What if file_size is at offset 0x10?
  uint64_t file_size_at_10 = *reinterpret_cast<uint64_t*>(&header[16]);
  std::cout << "file_size at offset 0x10: " << std::dec << file_size_at_10 << "\n";
  
  // Test: What if oldest_offset is at offset 0x18?
  uint64_t oldest_at_18 = *reinterpret_cast<uint64_t*>(&header[24]);
  std::cout << "oldest_chunk_offset at offset 0x18: " << std::dec << oldest_at_18 << "\n";
  
  // Test chunk header
  uint32_t chunk_hdr_size = *reinterpret_cast<uint32_t*>(&chunk_header[40]);
  std::cout << "\nchunk header_size at offset 0x28: " << std::dec << chunk_hdr_size << "\n";

  // Get actual file size
  file.seekg(0, std::ios::end);
  std::streampos actual_size = file.tellg();
  std::cout << "\nActual file size: " << actual_size << " bytes\n";

  return 0;
}
