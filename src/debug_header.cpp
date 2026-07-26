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

  // Read first 64 bytes of file header
  uint8_t header[64];
  file.read(reinterpret_cast<char*>(header), sizeof(header));
  
  std::cout << "=== EVTX File Header - First 64 bytes ===\n";
  print_hex(header, sizeof(header));
  
  // Extract values assuming different layouts
  std::cout << "\n=== Value Extraction Tests ===\n";
  
  // Test Layout 1: version at offset 8 (4 bytes)
  uint32_t version1 = *reinterpret_cast<uint32_t*>(&header[8]);
  uint16_t flags1 = *reinterpret_cast<uint16_t*>(&header[12]);
  uint16_t chunk_count1 = *reinterpret_cast<uint16_t*>(&header[14]);
  uint64_t file_size1 = *reinterpret_cast<uint64_t*>(&header[16]);
  uint64_t oldest_offset1 = *reinterpret_cast<uint64_t*>(&header[24]);
  uint64_t newest_offset1 = *reinterpret_cast<uint64_t*>(&header[32]);
  uint32_t checksum1 = *reinterpret_cast<uint32_t*>(&header[40]);
  
  std::cout << "Layout 1 (MS-EVTX spec):\n";
  std::cout << "  version (0x08): 0x" << std::hex << version1 << " (" << ((version1 >> 16) & 0xFFFF) << "." << (version1 & 0xFFFF) << ")\n";
  std::cout << "  flags (0x0C): 0x" << std::hex << flags1 << "\n";
  std::cout << "  chunk_count (0x0E): " << std::dec << chunk_count1 << "\n";
  std::cout << "  file_size (0x10): " << std::dec << file_size1 << "\n";
  std::cout << "  oldest_offset (0x18): " << std::dec << oldest_offset1 << "\n";
  std::cout << "  newest_offset (0x20): " << std::dec << newest_offset1 << "\n";
  std::cout << "  checksum (0x28): 0x" << std::hex << checksum1 << "\n";
  
  // Test Layout 2: version as two 2-byte fields
  uint16_t major2 = *reinterpret_cast<uint16_t*>(&header[8]);
  uint16_t minor2 = *reinterpret_cast<uint16_t*>(&header[10]);
  uint16_t flags2 = *reinterpret_cast<uint16_t*>(&header[12]);
  uint16_t chunk_count2 = *reinterpret_cast<uint16_t*>(&header[14]);
  uint32_t unknown2 = *reinterpret_cast<uint32_t*>(&header[16]);
  uint64_t file_size2 = *reinterpret_cast<uint64_t*>(&header[20]);
  
  std::cout << "\nLayout 2 (version as two 16-bit):\n";
  std::cout << "  major (0x08): " << std::dec << major2 << "\n";
  std::cout << "  minor (0x0A): " << std::dec << minor2 << "\n";
  std::cout << "  flags (0x0C): 0x" << std::hex << flags2 << "\n";
  std::cout << "  chunk_count (0x0E): " << std::dec << chunk_count2 << "\n";
  std::cout << "  unknown (0x10): 0x" << std::hex << unknown2 << "\n";
  std::cout << "  file_size (0x14): " << std::dec << file_size2 << "\n";

  return 0;
}
