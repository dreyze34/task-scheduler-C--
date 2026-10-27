#include <iostream>

#include "TaskPrint.hpp"


PrintTask::PrintTask(int id, const std::string& message)
    : Task(id, "PrintTask"), message(message) {}

void PrintTask::run() {
    std::cout << "[PrintTask] " << message << std::endl;
}