#include <iostream>
#include <numeric>

#include "TaskCompute.hpp"


ComputeTask::ComputeTask(int id, const std::vector<double>& values)
    : Task(id, "ComputeTask"), values(values) {};

void ComputeTask::run() {
    double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    std::cout << "[ComputeTask] " << mean << std::endl;
}