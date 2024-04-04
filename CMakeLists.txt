# Almost all CMake files should start with this
# You should always specify a range with the newest
# and oldest tested versions of CMake. This will ensure
# you pick up the best policies.
cmake_minimum_required(VERSION 3.1...3.29)

# This is your project statement. You should always list languages;
# Listing the version is nice here since it sets lots of useful variables
project(
  publish_subscribe
  VERSION 1.0
  LANGUAGES CXX)

# If you set any CMAKE_ variables, that can go here.
# (But usually don't do this, except maybe for C++ standard)
SET(CMAKE_CXX_FLAGS  "-O3 -fsanitize=address -std=c++17")
find_library(LIBRARY_NAME fmt)

# Check if the library is found
if (LIBRARY_NAME)
    add_executable(pub_sub_main main.cpp callbacks.h)

    # Make sure you link your targets with this command. It can also link libraries and
    # even flags, so linking a target that does not exist will not give a configure-time error.
    target_link_libraries(pub_sub_main PRIVATE ${LIBRARY_NAME})
else()
    message(FATAL_ERROR "Library not found")
endif()
