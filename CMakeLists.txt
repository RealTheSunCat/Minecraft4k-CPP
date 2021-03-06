cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

project(Minecraft4k C CXX)

#include(CMake/Utils.cmake)
#include(CMake/GlobalSettingsInclude.cmake OPTIONAL)
#set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include_directories(include)

set(PROJECT_NAME Minecraft4k)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "Constants.h"
    "Shader.h"
    "TextureGenerator.h"
    "Util.h"
    "World.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Resource_Files
    "res/raytrace.comp"
    "res/screen.frag"
    "res/screen.vert"
)
source_group("Resource Files" FILES ${Resource_Files})

set(Source_Files
    "glad.c"
    "Minecraft4k.cpp"
    "Shader.cpp"
    "TextureGenerator.cpp"
    "Util.cpp"
    "World.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Header_Files}
    ${Resource_Files}
    ${Source_Files}
)


add_executable(${PROJECT_NAME} ${ALL_FILES})
set(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

#set(ROOT_NAMESPACE Minecraft4k)

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:
            /Oi-;
            /Gy
        >
        /permissive-;
        /O1;
        /sdl;
        /W3;
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        /Os;
        ${DEFAULT_CXX_EXCEPTION_HANDLING}
    )
    
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /INCREMENTAL
        >
        $<$<CONFIG:Release>:
            /INCREMENTAL:NO
        >
        /DEBUG;
        /SUBSYSTEM:CONSOLE;
        /OPT:REF;
        /OPT:ICF
    )
endif()

################################################################################
# Dependencies
################################################################################
if(UNIX)
set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "glfw"
    "dl"
)
else()
set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "glfw"
)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")


link_directories(${PROJECT_NAME} PRIVATE
    "lib"
)

