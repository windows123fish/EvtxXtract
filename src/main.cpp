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
  std::cerr << "用法: " << program_name << " [evtx_file_path]\n\n";
  std::cerr << "EvtxXtract - 高性能Windows .evtx文件流式解析器\n\n";
  std::cerr << "阶段1: 文件侦察\n";
  std::cerr << "  - 验证文件头魔术数 ('ElfFile')\n";
  std::cerr << "  - 遍历并验证所有块 ('ElfChnk')\n";
  std::cerr << "  - 显示基本文件元数据\n\n";
  std::cerr << "参数:\n";
  std::cerr << "  <evtx_file_path>  要分析的.evtx文件路径（可选）\n";
  std::cerr << "  如果未提供路径，自动扫描Windows事件日志目录\n";
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
    std::cerr << "扫描目录时出错: " << e.what() << "\n";
  }
  
  std::sort(files.begin(), files.end());
  return files;
}

bool export_event_log(const std::string& log_name, const std::string& output_path) {
  // 如果输出文件已存在，先删除它
  try {
    fs::remove(output_path);
  } catch (...) {
    // 删除失败不影响后续操作
  }
  
  std::string command = "wevtutil epl " + log_name + " \"" + output_path + "\"";
  int result = std::system(command.c_str());
  return result == 0;
}

void analyze_file(const std::string& file_path) {
  std::cout << "\nEvtxXtract v1.0.0 - 阶段1: 文件侦察\n";
  std::cout << "=========================================\n";
  std::cout << "正在分析文件: " << file_path << "\n\n";

  // 获取实际文件大小
  uint64_t actual_file_size = 0;
  try {
    actual_file_size = fs::file_size(file_path);
  } catch (...) {
    actual_file_size = 0;
  }

  try {
    Evtx::EvtxParser parser(file_path);

    std::cout << "[1/4] 打开文件... ";
    if (!parser.open()) {
      std::cerr << "失败\n";
      std::cerr << "错误: " << parser.get_last_error() << "\n";
      return;
    }
    std::cout << "成功\n";

    std::cout << "[2/4] 读取文件头... ";
    if (!parser.read_file_header()) {
      std::cerr << "失败\n";
      std::cerr << "错误: " << parser.get_last_error() << "\n";
      return;
    }
    std::cout << "成功\n";

    const auto& file_header = parser.get_file_header();
    std::cout << "\n--- 文件头详情 ---\n";
    std::cout << file_header.to_string() << "\n";

    std::cout << "\n[3/4] 验证块... ";
    const size_t valid_chunk_count = parser.validate_chunks();
    std::cout << "成功\n";

    const auto& chunks = parser.get_valid_chunks();
    std::cout << "\n--- 块分析 ---\n";
    std::cout << "找到的块总数: " << valid_chunk_count << "\n";

    if (!chunks.empty()) {
      std::cout << "\n第一个块:\n";
      std::cout << chunks[0].to_string() << "\n";

      if (chunks.size() > 1) {
        std::cout << "\n最后一个块:\n";
        std::cout << chunks.back().to_string() << "\n";
      }
    }

    std::cout << "\n[4/4] 分析完成\n";
    std::cout << "========================\n";
    std::cout << "文件: " << file_path << "\n";
    std::cout << "状态: " << (file_header.validate_magic() ? "有效" : "无效") << "\n";
    
    // 使用文件头版本或显示未知
    uint16_t major = file_header.get_major_version();
    uint16_t minor = file_header.get_minor_version();
    std::cout << "版本: " << (major != 0 || minor != 0 ? std::to_string(major) + "." + std::to_string(minor) : "未知（文件头未填充）") << "\n";
    
    std::cout << "脏标记: " << (file_header.is_dirty() ? "已设置（文件可能已损坏）" : "未设置") << "\n";
    
    // 使用实际文件大小
    std::cout << "文件大小: " << actual_file_size << " 字节\n";
    
    // 显示块信息
    std::cout << "已验证块数: " << valid_chunk_count << "\n";
    
    // 如果文件头中的块数与实际不一致，显示警告
    if (file_header.chunk_count != valid_chunk_count) {
      std::cout << "警告: 文件头中记录的块数(" << file_header.chunk_count << ")与实际验证的块数(" << valid_chunk_count << ")不一致\n";
    }

    parser.close();

  } catch (const std::exception& e) {
    std::cerr << "\n意外错误: " << e.what() << "\n";
  } catch (...) {
    std::cerr << "\n发生未知错误\n";
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

  // 未提供路径 - 自动扫描Windows事件日志目录
  std::cout << "EvtxXtract v1.0.0 - 自动日志扫描器\n";
  std::cout << "=============================================\n\n";

  const std::string default_log_dir = "C:\\Windows\\System32\\winevt\\Logs\\";
  std::cout << "正在扫描默认事件日志目录: " << default_log_dir << "\n\n";

  std::vector<std::string> evtx_files = find_evtx_files(default_log_dir);

  // 如果在默认目录未找到文件，尝试使用wevtutil导出
  if (evtx_files.empty()) {
    std::cout << "在默认目录中未找到.evtx文件。\n";
    std::cout << "正在尝试使用wevtutil导出事件日志...\n\n";
    
    const std::string temp_dir = fs::temp_directory_path().string();
    const std::vector<std::string> log_names = {"System", "Application", "Security"};
    
    for (const auto& log_name : log_names) {
      std::string output_path = temp_dir + "\\" + log_name + "_export.evtx";
      std::cout << "正在导出 " << log_name << " 日志... ";
      
      if (export_event_log(log_name, output_path)) {
        std::cout << "成功\n";
        evtx_files.push_back(output_path);
      } else {
        std::cout << "失败\n";
        if (log_name == "Security") {
          std::cout << "      提示: Security日志需要管理员权限才能访问。\n";
        }
      }
    }
    
    if (evtx_files.empty()) {
      std::cerr << "\n无法访问事件日志。\n";
      std::cerr << "请指定一个文件路径，或将.evtx文件复制到可访问的位置。\n";
      std::cerr << "\n用法: " << argv[0] << " <evtx_file_path>\n";
      return 1;
    }
    
    std::cout << "\n";
  }

  std::cout << "找到 " << evtx_files.size() << " 个 .evtx 文件:\n";
  std::cout << "-----------------------------------------------------\n";
  
  for (size_t i = 0; i < evtx_files.size(); ++i) {
    std::cout << "[" << (i + 1) << "] " << fs::path(evtx_files[i]).filename().string();
    
    try {
      uint64_t file_size = fs::file_size(evtx_files[i]);
      std::cout << " (" << file_size << " 字节)";
    } catch (...) {
      std::cout << " (大小不可用)";
    }
    
    std::cout << "\n";
  }

  std::cout << "\n请输入要分析的文件编号（或输入 'all' 分析所有文件）: ";
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
        std::cerr << "无效的选择: " << input << "\n";
        return 1;
      }
    } catch (const std::exception&) {
      std::cerr << "无效的输入: " << input << "\n";
      return 1;
    }
  }

  return 0;
}
