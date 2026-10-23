cmake_minimum_required(VERSION 3.16)
project(TaskScheduler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(task_scheduler
    src/main.cpp
    src/Task.cpp
    src/TaskScheduler.cpp
)

target_include_directories(task_scheduler PRIVATE include)

target_compile_options(task_scheduler PRIVATE
    -Wall -Wextra -Wpedantic
)