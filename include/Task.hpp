#pragma once

#include <memory>
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
protected:
    virtual void run() = 0;

private:
    int id;
    std::string name;
    TaskStatus status;
    std::vector<std::weak_ptr<Task>> dependencies;

public:
    Task(int id, const std::string& name);
    virtual ~Task();

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