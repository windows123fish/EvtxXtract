#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdint>

void print_hex_dump(const uint8_t* data, size_t size) {
  for (size_t i = 0; i < size; i += 16) {
    std::cout << std::hex << std::setw(8) << std::setfill('0') << i << "  ";
    
    // Print hex values
    for (size_t j = 0; j < 16 && i + j < size; ++j) {
      std::cout << std::hex << std::setw(2) << std::setfill('0') 
                << static_cast<int>(data[i + j]) << " ";
      if (j == 7) std::cout << " ";
    }
    
    // Fill remaining space
    for (size_t j = size - i; j < 16; ++j) {
      std::cout << "   ";
      if (j == 8) std::cout << " ";
    }
    
    // Print ASCII representation
    std::cout << " | ";
    for (size_t j = 0; j < 16 && i + j < size; ++j) {
      char c = static_cast<char>(data[i + j]);
      if (c >= 0x20 && c <= 0x7E) {
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
    std::cerr << "Failed to open file: " << argv[1] << "\n";
    return 1;
  }

  // Read first 128 bytes for analysis
  uint8_t buffer[128];
  file.read(reinterpret_cast<char*>(buffer), sizeof(buffer));
  
  std::cout << "=== First 128 bytes of file ===\n";
  print_hex_dump(buffer, sizeof(buffer));
  
  std::cout << "\n=== Field interpretation (Little-endian) ===\n";
  
  // Magic (0x0000, 8 bytes)
  std::cout << "[0x0000-0x0007] Magic: ";
  for (size_t i = 0; i < 8; ++i) {
    std::cout << buffer[i];
  }
  std::cout << "\n";
  
  // Version (0x0008, 4 bytes)
  uint32_t version = *reinterpret_cast<uint32_t*>(&buffer[0x08]);
  uint16_t major = (version >> 16) & 0xFFFF;
  uint16_t minor = version & 0xFFFF;
  std::cout << "[0x0008-0x000B] Version: 0x" << std::hex << version << " (" << major << "." << minor << ")\n";
  
  // Flags (0x000C, 2 bytes)
  uint16_t flags = *reinterpret_cast<uint16_t*>(&buffer[0x0C]);
  std::cout << "[0x000C-0x000D] Flags: 0x" << std::hex << flags << "\n";
  
  // Chunk Count (0x000E, 2 bytes)
  uint16_t chunk_count = *reinterpret_cast<uint16_t*>(&buffer[0x0E]);
  std::cout << "[0x000E-0x000F] Chunk Count: 0x" << std::hex << chunk_count << " (" << std::dec << chunk_count << ")\n";
  
  // File Size (0x0010, 8 bytes)
  uint64_t file_size = *reinterpret_cast<uint64_t*>(&buffer[0x10]);
  std::cout << "[0x0010-0x0017] File Size: 0x" << std::hex << file_size << " (" << std::dec << file_size << " bytes)\n";
  
  // Oldest Chunk Offset (0x0018, 8 bytes)
  uint64_t oldest_offset = *reinterpret_cast<uint64_t*>(&buffer[0x18]);
  std::cout << "[0x0018-0x001F] Oldest Chunk Offset: 0x" << std::hex << oldest_offset << "\n";
  
