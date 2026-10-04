cmake_minimum_required(VERSION 3.15)
project(jk2x CXX)

option(JK2X_ANALYZE "Run the MSVC code analyzer (/analyze) as part of the build" ON)

# Static runtime: users need no Visual C++ redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(jk2x WIN32 src/jk2x.cpp)
set_target_properties(jk2x PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_compile_definitions(jk2x PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(jk2x PRIVATE shell32)

if(MSVC)
	# Highest warning level, warnings are errors, strict standard conformance;
	# warnings from system headers (<...>) are not ours to fix
	target_compile_options(jk2x PRIVATE /W4 /WX /permissive- /utf-8 /external:anglebrackets /external:W0)
	if(JK2X_ANALYZE)
		target_compile_options(jk2x PRIVATE /analyze /analyze:external-)
	endif()
endif()
