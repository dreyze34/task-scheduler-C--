cmake_minimum_required(VERSION 3.16)
project(TaskScheduler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(task_scheduler
    src/main.cpp
    src/Task.cpp
    src/TaskPrint.cpp
    src/TaskCompute.cpp
    src/TaskScheduler.cpp
)

target_include_directories(task_scheduler PRIVATE include)

target_compile_options(task_scheduler PRIVATE
    -Wall -Wextra -Wpedantic
)

include(CTest)

if(BUILD_TESTING)
    add_executable(scheduler_tests
        tests/test_scheduler.cpp
        src/Task.cpp
        src/TaskScheduler.cpp
    )
    target_include_directories(scheduler_tests PRIVATE include)
    target_compile_options(scheduler_tests PRIVATE -Wall -Wextra -Wpedantic)

    add_test(NAME scheduler_tests COMMAND scheduler_tests)
endif()
