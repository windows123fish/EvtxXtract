#pragma once

#include "evtx_structs.h"
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

namespace Evtx {

/**
 * @brief EvtxParser class
 * 
 * Provides streaming parsing functionality for Windows .evtx files.
 * Implements Phase 1: File Reconnaissance - validates file structure,
 * reads header metadata, and traverses chunks.
 */
class EvtxParser {
 public:
  /**
   * @brief Constructor (narrow string path)
   * 
   * @param file_path Path to the .evtx file
   */
  explicit EvtxParser(const std::string& file_path);
  
  /**
   * @brief Constructor (wide string path)
   * 
   * @param file_path Path to the .evtx file
   */
  explicit EvtxParser(const std::wstring& file_path);

  /**
   * @brief Destructor - closes the file if open
   */
  ~EvtxParser();

  /**
   * @brief Open the .evtx file
   * 
   * @return true if file opened successfully, false otherwise
   */
  bool open();

  /**
   * @brief Close the file
   */
  void close();

  /**
   * @brief Read and validate the file header
   * 
   * @return true if header is valid, false otherwise
   */
  bool read_file_header();

  /**
   * @brief Get the file header
   * 
   * @return Const reference to the file header
   */
  const EVT_FILE_HEADER& get_file_header() const noexcept;

  /**
   * @brief Traverse and validate all chunks in the file
   * 
   * Reads each chunk's header and validates its magic number.
   * 
   * @return Number of valid chunks found
   */
  size_t validate_chunks();

  /**
   * @brief Get information about all validated chunks
   * 
   * @return Vector of chunk headers for valid chunks
   */
  const std::vector<EVT_CHUNK_HEADER>& get_valid_chunks() const noexcept;

  /**
   * @brief Get the file path
   * 
   * @return The path to the .evtx file
   */
  const std::string& get_file_path() const noexcept;

  /**
   * @brief Check if the file is open
   * 
   * @return true if file is open, false otherwise
   */
  bool is_open() const noexcept;

  /**
   * @brief Get the last error message
   * 
   * @return Error message string
   */
  const std::string& get_last_error() const noexcept;

 private:
  /**
   * @brief Set an error message
   * 
   * @param error Error message to store
   */
  void set_error(const std::string& error);

  std::string file_path_;
  std::ifstream file_stream_;
  EVT_FILE_HEADER file_header_;
  std::vector<EVT_CHUNK_HEADER> valid_chunks_;
  std::string last_error_;
};

}  // namespace Evtx
