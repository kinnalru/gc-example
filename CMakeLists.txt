set(TARGET gc-example)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_BINARY_DIR})

add_definitions(-std=c++0x)

set(HEADERS
	gc.h
)


set(SOURCES
    main.cpp
)

set(DEPS
)



add_executable(${TARGET} WIN32 ${HEADERS} ${SOURCES})
target_link_libraries(${TARGET} ${DEPS})
