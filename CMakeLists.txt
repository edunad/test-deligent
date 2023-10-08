cmake_minimum_required(VERSION 3.16.3 FATAL_ERROR)

# Versioning
project("TEST" DESCRIPTION "TEST" LANGUAGES C CXX)
# ---

# Fix cmake paths
list(PREPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(PREPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
list(PREPEND CONAN_GENERATORS_FOLDER ${CMAKE_BINARY_DIR})
# ---

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSPDLOG_FMT_EXTERNAL -DNOMINMAX")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Apple needs this flag to compile mixed objective/c++
if(APPLE AND NOT XCODE)
    set(CMAKE_CXX_FLAGS "-ObjC++")
endif()

# Linux and Wayland support
if(NOT WIN32)
    #set(RAWRBOX_EXTRA_LIBS pthread GL X11)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")
#
    #if(RAWRBOX_USE_WAYLAND)
    #    list(APPEND RAWRBOX_EXTRA_LIBS wayland-egl)
#
    #    set(GLFW_BUILD_WAYLAND ON CACHE INTERNAL "")
    #    set(BGFX_USE_WAYLAND ON CACHE INTERNAL "")
    #endif()
else()
    # Ignore warnings about missing pdb
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4099")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4099")

    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
endif()

set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_SKIP_BUILD_RPATH TRUE)
# ---------------------------------


## LIBS -----
add_subdirectory(DiligentCore)
## ----------

# Grab source files
file(GLOB_RECURSE TEST_SOURCES "include/*.hpp" "src/*.cpp")

# Project setup
set(output_target test)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_HOME_DIRECTORY}bin/${CMAKE_BUILD_TYPE})

add_executable(${output_target} ${TEST_SOURCES})
target_include_directories(${output_target} PRIVATE "include")
target_compile_features(${output_target} PRIVATE cxx_std_${CMAKE_CXX_STANDARD})
target_compile_options(${output_target} PRIVATE -DUNICODE -DENGINE_DLL)
target_link_libraries(${output_target} PRIVATE
    Diligent-GraphicsEngineD3D11-shared
    Diligent-GraphicsEngineOpenGL-shared
    Diligent-GraphicsEngineD3D12-shared
    Diligent-GraphicsEngineVk-shared
)
target_compile_definitions(${output_target} PRIVATE NOMINMAX)
copy_required_dlls(${output_target})

if(NOT WIN32)
    set_target_properties(${output_target} PROPERTIES LINK_FLAGS -Wl,-rpath,'\$ORIGIN')
else()
    add_compile_options("/EHsc")
endif()
