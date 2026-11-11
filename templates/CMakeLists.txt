# Builds both templates and the self-test as 32-bit binaries. MIT, see LICENSE-MIT.
#
#   cmake -S . -B build-cmake -A Win32          (Visual Studio generator)
#   cmake --build build-cmake --config Release
#   build-cmake\Release\selftest.exe build-cmake\Release\TemplatePlugin.dll build-cmake\Release\dinput8.dll
#
# or, from an x86 developer prompt: cmake -S . -B build-cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake_minimum_required(VERSION 3.20)
project(tm_mod_templates CXX)

if(NOT MSVC)
  message(FATAL_ERROR "The templates use MSVC extensions (__try/__except); build with MSVC.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "The classic games are 32-bit: configure with -A Win32 or from an x86 developer prompt.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # static CRT: no redistributable needed

set(COMMON_SOURCES
  common/log.cpp
  common/gamebuild.cpp
  common/hook.cpp
  common/frame_counter.cpp)

function(template_options target)
  target_include_directories(${target} PRIVATE common)
  target_compile_definitions(${target} PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_compile_options(${target} PRIVATE /W4 /WX /EHsc /utf-8 /GS)
  target_link_options(${target} PRIVATE /SAFESEH /DYNAMICBASE /NXCOMPAT)
endfunction()

# TMLoader plugin: exports _DllMain@12 through __declspec(dllexport)
add_library(TemplatePlugin SHARED tmloader-plugin/plugin.cpp common/mod.cpp ${COMMON_SOURCES})
template_options(TemplatePlugin)

# dinput8 proxy: exports from the .def file, output name dinput8.dll
add_library(dinput8_proxy SHARED dinput8-proxy/proxy.cpp dinput8-proxy/dinput8.def common/mod.cpp ${COMMON_SOURCES})
set_target_properties(dinput8_proxy PROPERTIES OUTPUT_NAME dinput8)
template_options(dinput8_proxy)

# self-test, no game needed
add_executable(selftest tests/selftest.cpp ${COMMON_SOURCES})
template_options(selftest)
target_link_libraries(selftest PRIVATE d3d9 dxguid)
