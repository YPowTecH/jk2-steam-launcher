cmake_minimum_required(VERSION 3.15)
project(jk2x CXX)

# Static runtime: users need no Visual C++ redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(jk2x WIN32 src/jk2x.cpp)
set_target_properties(jk2x PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_compile_definitions(jk2x PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(jk2x PRIVATE shell32)
