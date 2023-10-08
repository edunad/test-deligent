cmake_minimum_required(VERSION 3.16.3 FATAL_ERROR)

# SETUP CPM -----
include("cmake/cpm.cmake")
CPMUsePackageLock(package-lock.cmake)
# ---------------

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
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")

# Apple needs this flag to compile mixed objective/c++
if(APPLE AND NOT XCODE)
    set(CMAKE_CXX_FLAGS "-ObjC++")
endif()

# Linux and Wayland support
if(NOT WIN32)
    set(EXTRA_LIBS pthread GL X11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")

    if(USE_WAYLAND)
        list(APPEND EXTRA_LIBS wayland-egl)
        set(GLFW_BUILD_WAYLAND ON CACHE INTERNAL "")
    endif()
else()
    # Ignore warnings about missing pdb
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4099")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4099")

    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-") # Breaks deligent
endif()

set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_SKIP_BUILD_RPATH TRUE)
# ---------------------------------


## LIBS -----
add_subdirectory(DiligentCore)

CPMAddPackage(
    NAME
        glfw
    GITHUB_REPOSITORY
        glfw/glfw
    GIT_TAG
        3.3.8
    OPTIONS
        "GLFW_BUILD_DOCS OFF"
        "GLFW_BUILD_EXAMPLES OFF"
        "GLFW_BUILD_TESTS OFF"
        "GLFW_INSTALL OFF"
        "GLFW_USE_OSMESA OFF"
        "GLFW_VULKAN_STATIC OFF"
        "USE_MSVC_RUNTIME_LIBRARY_DLL OFF")
## ----------

## ASSETS ----
set(SHADERS
    assets/cube.vsh
    assets/cube.psh
)

if(${CMAKE_VERSION} VERSION_LESS "3.26.0")
    add_custom_target(
        copy_resources ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/assets
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets
        COMMENT "Copying resources into binary directory")
else()
    add_custom_target(
        copy_resources ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${PROJECT_SOURCE_DIR}/assets
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets
        COMMENT "Copying resources into binary directory")
endif()

## ------

# Grab source files
file(GLOB_RECURSE TEST_SOURCES "include/*.hpp" "src/*.cpp")

# Project setup
set(output_target test)
add_executable(${output_target} ${TEST_SOURCES} ${SHADERS})
add_dependencies(${output_target} copy_resources)
target_include_directories(${output_target} PRIVATE "include" "./DiligentCore")
target_compile_features(${output_target} PRIVATE cxx_std_${CMAKE_CXX_STANDARD})
target_compile_options(${output_target} PRIVATE -DUNICODE -DENGINE_DLL)
target_compile_definitions(${output_target} PRIVATE NOMINMAX)
target_link_libraries(${output_target} PRIVATE
    Diligent-GraphicsTools
    #Diligent-ShaderTools
    glfw
    ${EXTRA_LIBS}
)

if(WIN32)
    if(D3D11_SUPPORTED)
        target_link_libraries(${output_target} PRIVATE Diligent-GraphicsEngineD3D11-shared)
    endif()

    if(D3D12_SUPPORTED)
        target_link_libraries(${output_target} PRIVATE Diligent-GraphicsEngineD3D12-shared)
    endif()
endif()

if(GL_SUPPORTED)
    target_link_libraries(${output_target} PRIVATE Diligent-GraphicsEngineOpenGL-shared)
endif()

if(VULKAN_SUPPORTED)
    target_link_libraries(${output_target} PRIVATE Diligent-GraphicsEngineVk-shared)
endif()

if(NOT WIN32)
    set_target_properties(${output_target} PROPERTIES LINK_FLAGS -Wl,-rpath,'\$ORIGIN')
else()
    copy_required_dlls(${output_target})
    add_compile_options("/EHsc")
endif()


