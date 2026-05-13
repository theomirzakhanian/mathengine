cmake_minimum_required(VERSION 3.20)
project(MathEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# M_PI / M_E come from <cmath> on POSIX but MSVC requires this define.
add_compile_definitions(_USE_MATH_DEFINES)

if(APPLE)
    add_compile_definitions(GL_SILENCE_DEPRECATION)
endif()

if(WIN32)
    # Stop <windows.h> from defining min/max macros that clash with std::min/std::max.
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

include(cmake/Dependencies.cmake)

add_subdirectory(engine)
add_subdirectory(app)
add_subdirectory(tests)
