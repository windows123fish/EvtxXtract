#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdint>

void print_hex_dump(const uint8_t* data, size_t size) {
  for (size_t i = 0; i < size; i += 16) {
    std::cout << std::hex << std::setw(8) << std::setfill('0') << i << "  ";
    
    // Print hex values
    for (size_t j =