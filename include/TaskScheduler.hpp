#include <unordered_map>
#include <unordered_set>

#include "Task.hpp"
#include "TaskCompute.hpp"
#include "TaskPrint.hpp"

class TaskScheduler {
private:
    std::unordered_map<int, std::shared_ptr<Task>> tasks;
    int nextId = 1;

public:
    template<typename T, typename... Args>
    std::shared_ptr<T> createTask(Args&&... args) {
        int id = nextId++;

        auto task = std::make_shared<T>(
            id,
            std::forward<Args>(args)...
        );

        tasks[id] = task;

        return task;
    }

    std::shared_ptr<Task> getTask(int id) const;
    bool removeTask(int id);
    void executeTask(int id);
    void printAllTasks() const;
    std::vector<int> getTaskIds() const;

    bool hasTaskCycle(const std::shared_ptr<Task>& task, std::unordered_set<int>& visited) const;
    bool hasCycle() const;

    std::vector<int> getExecutionOrder() const;
    void executeAll();
};