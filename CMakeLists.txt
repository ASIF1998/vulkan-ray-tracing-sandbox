cmake_minimum_required(VERSION 3.27)

if (NOT WIN32)
    message(FATAL_ERROR "This project works only Windows.")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

project(vulkan-ray-tracing-sandbox)

set(VULKAN_RAY_TRACING_SANDBOX_PROJECT_DIR ${CMAKE_SOURCE_DIR})

configure_file(include/base/configuration.hpp.in ${CMAKE_SOURCE_DIR}/include/base/configuration.hpp)

file(GLOB_RECURSE VULKAN_RAY_TRACING_SANDBOX_SOURCES CONFIGURE_DEPENDS src/*.cpp)

include_directories(include/)

add_executable(vulkan-ray-tracing-sandbox ${VULKAN_RAY_TRACING_SANDBOX_SOURCES} main.cpp)

if (MSVC)
    target_compile_options(vulkan-ray-tracing-sandbox PRIVATE /W3 /WX)
endif()

######################################################################
#   vulkan
######################################################################
find_package(Vulkan REQUIRED)

set(Vulkan_SDK_LIBS ${Vulkan_INCLUDE_DIRS}/../Lib)

######################################################################
#   glslang
######################################################################
set(GLSLANG_INCLUDE_DIRECTORY ${CMAKE_SOURCE_DIR}/third_party/glslang/include)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(GLSLANG_LIBS_NAME 
        GenericCodeGend.lib
        glslang-default-resource-limitsd.lib
        glslangd.lib
        MachineIndependentd.lib
        OSDependentd.lib
        SPIRV-Tools-optd.lib
        SPIRV-Toolsd.lib
        SPIRVd.lib
        SPVRemapperd.lib
    )
else()
    set(GLSLANG_LIBS_NAME 
        GenericCodeGen.lib
        glslang-default-resource-limits.lib
        glslang.lib
        MachineIndependent.lib
        OSDependent.lib
        SPIRV-Tools-opt.lib
        SPIRV-Tools.lib
        SPIRV.lib
        SPVRemapper.lib
    )
endif()

foreach(library ${GLSLANG_LIBS_NAME})
    set(GLSLANG_LIBS ${GLSLANG_LIBS} ${CMAKE_SOURCE_DIR}/third_party/glslang/${CMAKE_BUILD_TYPE}/${library})
endforeach()

######################################################################
#   assimp
######################################################################
set(ASSIMP_ROOT ${CMAKE_SOURCE_DIR}/third_party/assimp)

set(ASSIMP_INCLUDE_DIRECTORY ${ASSIMP_ROOT}/include)

set(ASSIMP_LIBS
    ${ASSIMP_ROOT}/assimp-vc143-mt.lib
)

set(ASSIMP_BINS
    assimp-vc143-mt.dll
    assimp-vc143-mt.exp
    assimp-vc143-mt.lib
    assimp-vc143-mt.pdb
)

foreach(bin_name ${ASSIMP_BINS})
    configure_file(${ASSIMP_ROOT}/${bin_name} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/${bin_name} COPYONLY)
endforeach()

######################################################################
#   stb
######################################################################
set(STB_INCLUDE_DIRECTORY ${CMAKE_SOURCE_DIR}/third_party/)

######################################################################
#   SDL2
######################################################################
set(SDL2_INCLUDE_DIRECTORY ${CMAKE_SOURCE_DIR}/third_party/SDL2/include)

set(SDL2_LIBS 
    ${CMAKE_SOURCE_DIR}/third_party/SDL2/SDL2.lib
    ${CMAKE_SOURCE_DIR}/third_party/SDL2/SDL2main.lib
)

######################################################################
#   glm
######################################################################
set(GLM_INCLUDE_DIRECTORY ${CMAKE_SOURCE_DIR}/third_party)

######################################################################
#   add libs
######################################################################
target_link_libraries(vulkan-ray-tracing-sandbox PUBLIC
    ${Vulkan_LIBRARIES}
    ${SDL2_LIBS}
    ${GLSLANG_LIBS}
    ${ASSIMP_LIBS}
)
    
target_include_directories(vulkan-ray-tracing-sandbox PUBLIC 
    ${Vulkan_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRECTORY}
    ${GLSLANG_INCLUDE_DIRECTORY}
    ${ASSIMP_INCLUDE_DIRECTORY}
    ${STB_INCLUDE_DIRECTORY}
    ${GLM_INCLUDE_DIRECTORY}
)
