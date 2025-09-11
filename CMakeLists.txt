cmake_minimum_required(VERSION 3.14)
project(tp3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# GoogleTest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# FTXUI
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG v5.0.0  # Use a stable version
)
FetchContent_MakeAvailable(ftxui)

# Set FTXUI include directory
set(FTXUI_INCLUDE_DIR ${ftxui_SOURCE_DIR}/include)

enable_testing()

# Find threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Test executable
add_executable(
  tp_tests
  tests/tests.cpp
  hangman.cpp
)
target_link_libraries(
  tp_tests
  GTest::gtest_main
  Threads::Threads
)
target_include_directories(
  tp_tests PRIVATE ${CMAKE_SOURCE_DIR}
)

include(GoogleTest)
gtest_discover_tests(tp_tests)

# Main executable
add_executable(
  main
  main.cpp
  hangman.cpp
)

# Include the directory containing the header files
target_include_directories(
  main PRIVATE ${CMAKE_SOURCE_DIR}
  PRIVATE ${FTXUI_INCLUDE_DIR}
)

# Link libraries to main
target_link_libraries(main
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
  PRIVATE Threads::Threads
)

# Server executable
add_executable(
  server
  server.cpp
  hangman.cpp
  threadpool.cpp
)
target_include_directories(
  server PRIVATE ${CMAKE_SOURCE_DIR}
)
target_link_libraries(server
  PRIVATE Threads::Threads
)

# Client executable
add_executable(
  client
  client.cpp
  hangman.cpp
)
target_include_directories(
  client PRIVATE ${CMAKE_SOURCE_DIR}
  PRIVATE ${FTXUI_INCLUDE_DIR}
)
target_link_libraries(client
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
  PRIVATE Threads::Threads
)


