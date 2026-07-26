cmake_minimum_required(VERSION 3.25)

project(EvtxXtract
        VERSION 1.0.0
        DESCRIPTION "High-performance streaming parser for Windows .evtx files"
        LANGUAGES CXX)

# Require C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_RE