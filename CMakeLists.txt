cmake_minimum_required(VERSION 3.20)
project(main LANGUAGES CXX)

# Include directories
include_directories(include)

# Add client executable
add_executable(client ./src/client.cpp)
target_compile_options(client PUBLIC -fsanitize=address -g)
target_link_options(client PUBLIC -fsanitize=address -g)

# Add gamedemon executable
add_executable(gamedemon ./src/gamedemon.cpp)
target_compile_options(gamedemon PUBLIC -fsanitize=address -g)
target_link_options(gamedemon PUBLIC -fsanitize=address -g)

# Add server executable
add_executable(server ./src/server.cpp)
target_compile_options(server PUBLIC -fsanitize=address -g)
target_link_options(server PUBLIC -fsanitize=address -g)

# Add tests executable
add_executable(tests ./tests/tests.cpp)
target_compile_options(tests PUBLIC -fsanitize=address -g)
target_link_options(tests PUBLIC -fsanitize=address -g)

# Link GTest and pthread for the tests
find_package(GTest REQUIRED)
target_link_libraries(tests GTest::GTest GTest::Main pthread)

# Enable testing
enable_testing()

# Add the tests to CTest
add_test(NAME RunTests COMMAND tests)
