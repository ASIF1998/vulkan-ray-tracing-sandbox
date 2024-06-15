cmake_minimum_required(VERSION 3.27)

if (NOT WIN32)
    message(FATAL_ERROR "This project works only Windows.")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    set(GLSLANG_LIBS ${GLSLANG_LIBS} ${Vulkan_SDK_LIBS}/${library})
endforeach()

######################################################################
#   assimp
######################################################################
set(ASSIMP_ROOT ${CMAKE_SOURCE_DIR}/third_party/assimp)

set(ASSIMP_INCLUDE_DIRECTORY ${ASSIMP_ROOT}/include)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ASSIMP_LIBS_NAME
        assimp-vc143-mtd.lib
    )
else()
    set(ASSIMP_LIBS_NAME
        assimp-vc143-mt.lib
    )
endif()

foreach(library ${ASSIMP_LIBS_NAME})
    set(ASSIMP_LIBS ${ASSIMP_LIBS} ${ASSIMP_ROOT}/${CMAKE_BUILD_TYPE}/${library})
endforeach()

######################################################################
#   stb
######################################################################
set(STB_INCLUDE_DIRECTORY ${CMAKE_SOURCE_DIR}/third_party/)

######################################################################
#   SDL2
######################################################################
set(SDL2_LIBS 
    ${Vulkan_SDK_LIBS}/SDL2.lib
    ${Vulkan_SDK_LIBS}/SDL2main.lib
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
    ${ASSIMP_INCLUDE_DIRECTORY}
    ${STB_INCLUDE_DIRECTORY}
)
