#pragma once

#include "Task.hpp"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <queue>
#include <string>
#include <vector>
#include <utility>

enum class TaskStatus {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED
};

class Task {
private:
    int id;
    std::string name;
    TaskStatus status;
    std::vector<std::weak_ptr<Task>> dependencies;

public:
    Task(int id, const std::string& name);
    ~Task();

    int getId() const;
    const std::string& getName() const;
    TaskStatus getStatus() const;

    void setStatus(const TaskStatus newStatus);
    void execute();
    void print();

    void addDependency(const std::shared_ptr<Task>& task);
    bool canExecute() const;
    std::vector<std::shared_ptr<Task>> getDependencies() const;
};

class TaskScheduler {
private:
    std::unordered_map<int, std::shared_ptr<Task>> tasks;
    int nextId = 1;

public:
    std::shared_ptr<Task> createTask(const std::string& name);
    std::shared_ptr<Task> getTask(int id) const;
    bool removeTask(int id);
    void executeTask(int id);
    void printAllTasks() const;
    std::vector<int> getTaskIds() const;
    bool hasTaskCycle(const std::shared_ptr<Task>& task, std::unordered_set<int>& visited) const;
    bool hasCycle() const;
    std::vector<int> getExecutionOrder() const;
};