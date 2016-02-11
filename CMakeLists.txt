cmake_minimum_required(VERSION 2.8.4)

project(CfsLib)

if(${MINGW})
  set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/out") 
endif()

include_directories(${PROJECT_SOURCE_DIR}/h/${PROJECT_NAME})

file(GLOB SRCS "src/*.c")

if(${CMAKE_COMPILER_IS_GNUCC})
    set(CMAKE_C_FLAGS "-Wall -O2 -std=gnu99 ")
endif()

if(${UNIX})
  list(APPEND DEPENDENCIES rt)
elseif(${MINGW})
  #list(APPEND DEPENDENCIES winmm imagehlp)
endif()

add_executable(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME} ${DEPENDENCIES})

if(${ENABLE_TEST})
    add_subdirectory(test)
endif()

if(${UNIX})
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/h/${PROJECT_NAME} DESTINATION include)
elseif(${MINGW})
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${INSTALL_DIR})
endif()

