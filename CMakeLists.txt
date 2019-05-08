cmake_minimum_required(VERSION 3.5)
project(main)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
    common.h
    glad.c
    main.cpp
    ShaderProgram.h
    ShaderProgram.cpp
    camera.h
    mesh.h
    model.h)

set(ADDITIONAL_INCLUDE_DIRS
        dependencies/include/GLAD
        dependencies/include/SOIL
        dependencies/include/freetype/include
        dependencies/include)

set(ADDITIONAL_LIBRARY_DIRS
        dependencies/lib)   

set(ADDITIONAL_RUNTIME_LIBRARY_DIRS
        dependencies/bin)

set(IRRKLANG_HOME "${CMAKE_CURRENT_SOURCE_DIR}/irrKlang-64bit-1.6.0")
set(IRRKLANG_INCLUDE_DIR "${IRRKLANG_HOME}/include")
set(IRRKLANG_LIBRARIES "${IRRKLANG_HOME}/lib/Winx64-visualStudio/irrKlang.lib")

include_directories(${IRRKLANG_INCLUDE_DIR})

set (CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG}")

if(WIN32)
  set(ADDITIONAL_INCLUDE_DIRS 
        ${ADDITIONAL_INCLUDE_DIRS}
        dependencies/include)
  link_directories(${ADDITIONAL_LIBRARY_DIRS})
else()
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")
  find_package(glfw3 REQUIRED)
endif()

include_directories(${ADDITIONAL_INCLUDE_DIRS})

find_package(OpenGL REQUIRED)

add_executable(main ${SOURCE_FILES})

target_include_directories(main PRIVATE ${OPENGL_INCLUDE_DIR})
add_custom_command(TARGET main POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/shaders" "${PROJECT_BINARY_DIR}")

#ASSIMP
find_package(ASSIMP REQUIRED)
if(ASSIMP_FOUND)
    include_directories(${ASSIMP_INCLUDE_DIR})
endif()

#FREETYPE
find_package(Freetype REQUIRED)
if(FREETYPE)
    include_directories(${FREETYPE_INCLUDE_DIR})
endif()

if(WIN32)
  add_custom_command(TARGET main POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/dependencies/bin" $<TARGET_FILE_DIR:main>)
  #set(CMAKE_MSVCIDE_RUN_PATH ${ADDITIONAL_RUNTIME_LIBRARY_DIRS})
  target_compile_options(main PRIVATE)
  target_link_libraries(main LINK_PUBLIC ${OPENGL_gl_LIBRARY} glfw3dll)
else()
  target_compile_options(main PRIVATE -Wnarrowing)
  target_link_libraries(main LINK_PUBLIC ${OPENGL_gl_LIBRARY} glfw rt dl SOIL)
  target_link_libraries(main LINK_PUBLIC ${ASSIMP_LIBRARIES})
  target_link_libraries(main LINK_PUBLIC ${FREETYPE_LIBRARIES})
  target_link_libraries(main LINK_PUBLIC ${IRRKLANG_LIBRARIES})
  target_link_libraries(main LINK_PUBLIC "${IRRKLANG_HOME}/bin/linux-gcc-64/libIrrKlang.so")

endif()
