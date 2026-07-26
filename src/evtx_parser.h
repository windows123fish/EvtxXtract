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

  // Phase 2: Event Record Parsing
  
  /**
   * @brief Parse all event records from the file
   * 
   * @param max_records Maximum number of records to parse (0 = all)
   * @return Vector of parsed event records
   */
  std::vector<EventRecord> parse_all_events(size_t max_records = 0);

  /**
   * @brief Parse event records from a specific chunk
   * 
   * @param chunk_index Index of the chunk to parse
   * @param max_records Maximum number of records to parse (0 = all)
   * @return Vector of parsed event records
   */
  std::vector<EventRecord> parse_chunk_events(size_t chunk_index, size_t max_records = 0);

  /**
   * @brief Get total number of events in the file
   * 
   * @return Total event count
   */
  uint64_t get_total_event_count() const noexcept;

 private:
  /**
   * @brief Set an error message
   * 
   * @param error Error message to store
   */
  void set_error(const std::string& error);

  /**
   * @brief Parse a single event record from current file position
   * 
   * @param chunk_offset Offset of the chunk containing this record
   * @param record Event record to populate
   * @return true if parsing succeeded
   */
  bool parse_event_record(uint64_t chunk_offset, EventRecord& record);

  /**
   * @brief Parse binary XML data
   * 
   * @param data Binary XML data
   * @param size Size of the data
   * @param record Event record to populate with parsed data
   */
  void parse_binary_xml(const uint8_t* data, size_t size, EventRecord& record);

  /**
   * @brief Parse a BXml token
   * 
   * @param data Binary data pointer (will be advanced)
   * @param end End of data
   * @param record Event record to populate
   * @param current_element Current XML element name
   */
  void parse_bxml_token(const uint8_t*& data, const uint8_t* end, EventRecord& record, std::string& current_element);

  /**
   * @brief Read a variable-length string from binary XML
   * 
   * @param data Binary data pointer (will be advanced)
   * @param end End of data
   * @return The parsed string
   */
  std::string read_bxml_string(const uint8_t*& data, const uint8_t* end);

  /**
   * @brief Read a variable-length integer from binary XML
   * 
   * @param data Binary data pointer (will be advanced)
   * @return The parsed integer
   */
  uint64_t read_vlq(const uint8_t*& data);

  std::string file_path_;
  std::ifstream file_stream_;
  EVT_FILE_HEADER file_header_;
  std::vector<EVT_CHUNK_HEADER> valid_chunks_;
  std::string last_error_;
};

}  // namespace Evtx
