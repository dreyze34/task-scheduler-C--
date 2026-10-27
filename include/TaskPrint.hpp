#include "Task.hpp"

class PrintTask : public Task {
private: 
    std::string message;

protected:
    void run() override;

public:
    PrintTask(int id, const std::string& message);
};