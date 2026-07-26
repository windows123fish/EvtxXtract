#include "evtx_parser.h"
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cstdio>

namespace fs = std::filesystem;

/**
 * @brief EvtxXtract - High-performance streaming parser for Windows .evtx files
 * 
 * Phase 1: File Reconnaissance (MVP)
 * - Open and validate .evtx files
 * - Read and verify file header magic ("ElfFile")
 * - Traverse and count valid chunks
 * - Display essential metadata
 * 
 * Usage: EvtxXtract [evtx_file_path]
 * If no path is provided, automatically scans C:\Windows\System32\winevt\Logs\
 */

void print_usage(const std::string& program_name) {
  std::cerr << "Usage: " << program_name << " [evtx_file_path]\n\n";
  std::cerr << "EvtxXtract - High-performance streaming parser for Windows .evtx files\n\n";
  std::cerr << "Phase 1: File Reconnaissance\n";
  std::cerr << "  - Validates file header magic number ('ElfFile')\n";
  std::cerr << "  - Traverses and validates all chunks ('ElfChnk')\n";
  std::cerr << "  - Displays essential file metadata\n\n";
  std::cerr << "Arguments:\n";
  std::cerr << "  <evtx_file_path>  Path to the .evtx file to analyze (optional)\n";
  std::cerr << "  If no path is provided, automatically scans Windows Event Log directory\n";
}

std::vector<std::string> find_evtx_files(const std::string& directory) {
  std::vector<std::string> files;
  
  try {
    if (fs::exists(directory) && fs::is_directory(directory)) {
      for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".evtx") {
          files.push_back(entry.path().string());
        }
      }
    }
  } catch (const fs::filesystem_error& e) {
    std::cerr << "Error scanning directory: " << e.what() << "\n";
  }
  
  std::sort(files.begin(), files.end());
  return files;
}

bool export_event_log(const std::string& log_name, const std::string& output_path) {
  std::string command = "wevtutil epl " + log_name + " \"" + output_path + "\"";
  int result = std::system(command.c_str());
  return result == 0;
}

void analyze_file(const std::string& file_path) {
  std::cout << "\nEvtxXtract v1.0.0 - Phase 1: File Reconnaissance\n";
  std::cout << "=================================================\n";
  std::cout << "Analyzing file: " << file_path << "\n\n";

  try {
    Evtx::EvtxParser parser(file_path);

    std::cout << "[1/4] Opening file... ";
    if (!parser.open()) {
      std::cerr << "FAILED\n";
      std::cerr << "Error: " << parser.get_last_error() << "\n";
      std::cerr << "Note: You may need to run this program as Administrator to access system logs.\n";
      return;
    }
    std::cout << "OK\n";

    std::cout << "[2/4] Reading file header... ";
    if (!parser.read_file_header()) {
      std::cerr << "FAILED\n";
      std::cerr << "Error: " << parser.get_last_error() << "\n";
      return;
    }
    std::cout << "OK\n";

    const auto& file_header = parser.get_file_header();
    std::cout << "\n--- File Header Details ---\n";
    std::cout << file_header.to_string() << "\n";

    std::cout << "\n[3/4] Validating chunks... ";
    const size_t valid_chunk_count = parser.validate_chunks();
    std::cout << "OK\n";

    const auto& chunks = parser.get_valid_chunks();
    std::cout << "\n--- Chunk Analysis ---\n";
    std::cout << "Total chunks found: " << valid_chunk_count << "\n";

    if (!chunks.empty()) {
      std::cout << "\nFirst chunk:\n";
      std::cout << chunks[0].to_string() << "\n";

      if (chunks.size() > 1) {
        std::cout << "\nLast chunk:\n";
        std::cout << chunks.back().to_string() << "\n";
      }
    }

    std::cout << "\n[4/4] Analysis Complete\n";
    std::cout << "========================\n";
    std::cout << "File: " << file_path << "\n";
    std::cout << "Status: " << (file_header.validate_magic() ? "VALID" : "INVALID") << "\n";
    std::cout << "Version: " << file_header.get_major_version() << "." << file_header.get_minor_version() << "\n";
    std::cout << "Dirty flag: " << (file_header.is_dirty() ? "SET (file may be corrupted)" : "NOT SET") << "\n";
    std::cout << "File size: " << file_header.file_size << " bytes\n";
    std::cout << "Expected chunks: " << file_header.chunk_count << "\n";
    std::cout << "Validated chunks: " << valid_chunk_count << "\n";

    parser.close();

  } catch (const std::exception& e) {
    std::cerr << "\nUnexpected error: " << e.what() << "\n";
  } catch (...) {
    std::cerr << "\nUnknown error occurred\n";
  }
}

int main(int argc, char* argv[]) {
  if (argc > 2) {
    print_usage(argv[0]);
    return 1;
  }

  // If a file path is provided, analyze it directly
  if (argc == 2) {
    analyze_file(argv[1]);
    return 0;
  }

  // No path provided - automatically scan Windows Event Log directory
  std::cout << "EvtxXtract v1.0.0 - Automatic Log Scanner\n";
  std::cout << "===========================================\n\n";

  const std::string default_log_dir = "C:\\Windows\\System32\\winevt\\Logs\\";
  std::cout << "Scanning default Event Log directory: " << default_log_dir << "\n\n";

  std::vector<std::string> evtx_files = find_evtx_files(default_log_dir);

  // If no files found in default directory, try to export using wevtutil
  if (evtx_files.empty()) {
    std::cout << "No .evtx files found in the default directory.\n";
    std::cout << "Attempting to export event logs using wevtutil...\n\n";
    
    const std::string temp_dir = fs::temp_directory_path().string();
    const std::vector<std::string> log_names = {"System", "Application", "Security"};
    
    for (const auto& log_name : log_names) {
      std::string output_path = temp_dir + "\\" + log_name + "_export.evtx";
      std::cout << "Exporting " << log_name << " log... ";
      
      if (export_event_log(log_name, output_path)) {
        std::cout << "OK\n";
        evtx_files.push_back(output_path);
      } else {
        std::cout << "FAILED (may need Administrator privileges)\n";
      }
    }
    
    if (evtx_files.empty()) {
      std::cerr << "\nFailed to access event logs.\n";
      std::cerr << "Please run this program as Administrator or specify a file path.\n";
      std::cerr << "\nUsage: " << argv[0] << " <evtx_file_path>\n";
      return 1;
    }
    
    std::cout << "\n";
  }

  std::cout << "Found " << evtx_files.size() << " .evtx file(s):\n";
  std::cout << "-----------------------------------------------------\n";
  
  for (size_t i = 0; i < evtx_files.size(); ++i) {
    std::cout << "[" << (i + 1) << "] " << fs::path(evtx_files[i]).filename().string();
    
    try {
      uint64_t file_size = fs::file_size(evtx_files[i]);
      std::cout << " (" << file_size << " bytes)";
    } catch (...) {
      std::cout << " (size unavailable)";
    }
    
    std::cout << "\n";
  }

  std::cout << "\nEnter the number of the file to analyze (or 'all' for all files): ";
  std::string input;
  std::getline(std::cin, input);

  if (input == "all" || input == "ALL" || input == "All") {
    for (const auto& file : evtx_files) {
      analyze_file(file);
      std::cout << "\n" << std::string(60, '-') << "\n\n";
    }
  } else {
    try {
      size_t index = std::stoull(input) - 1;
      if (index < evtx_files.size()) {
        analyze_file(evtx_files[index]);
      } else {
        std::cerr << "Invalid selection: " << input << "\n";
        return 1;
      }
    } catch (const std::exception&) {
      std::cerr << "Invalid input: " << input << "\n";
      return 1;
    }
  }

  return 0;
}
