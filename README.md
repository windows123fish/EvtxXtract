# EvtxXtract
Windows 的 .evtx文件采用了一种专有的二进制 XML 格式来存储日志，以提高存储效率。现有的开源工具（如 python-evtx）基于 Python，解析速度慢且内存占用高。本项目旨在用 Modern C++ (C++20) 实现一个高性能、流式解析器
