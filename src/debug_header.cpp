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
    for