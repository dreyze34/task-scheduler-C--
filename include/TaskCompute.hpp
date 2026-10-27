#include "Task.hpp"

class ComputeTask : public Task {
private: 
    std::vector<double> values;

protected:
    void run() override;

public:
    ComputeTask(int id, const std::vector<double>& values);
};