cmake_minimum_required(VERSION 3.14)
project(sqlite_test)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set (SRCS
	src/Main.cpp
	src/sqlite_db.cpp)

add_compile_options(-Wall)

add_executable(sqlite_test ${SRCS})
target_link_libraries(sqlite_test PRIVATE sqlite3)
set_property(TARGET sqlite_test PROPERTY C_STANDARD 17)
set_property(TARGET sqlite_test PROPERTY CXX_STANDARD 20)
