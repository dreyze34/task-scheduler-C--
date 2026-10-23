#include <string>
#include <iostream>
#include <algorithm>

#include <Task.hpp>

Task::Task(int id, const std::string &name)
    : id(id), name(name), status(TaskStatus::PENDING) {};

Task::~Task() {};

int Task::getId() const
{
    return this->id;
}

const std::string &Task::getName() const
{
    return this->name;
}

TaskStatus Task::getStatus() const
{
    return this->status;
}

void Task::setStatus(const TaskStatus newStatus)
{
    this->status = std::move(newStatus);
}

void Task::execute()
{
    std::cout << "Running " << "task " << std::to_string(this->getId()) << "." << std::endl;
    this->setStatus(TaskStatus::RUNNING);
    std::cout << "Task " << std::to_string(this->getId()) << " successfully finished." << std::endl;
    this->setStatus(TaskStatus::COMPLETED);
}

void Task::print() {
    std::cout << "Task " << std::to_string(this->id) << ": " << this->name << std::endl;
}

void Task::addDependency(const std::shared_ptr<Task>& task) {
    if (task == nullptr) {
        return;
    }
    this->dependencies.push_back(task);
}

bool Task::canExecute() const {
    return std::all_of(
        this->dependencies.begin(),
        this->dependencies.end(),
        [](const std::weak_ptr<Task>& weakDependency) {
            const std::shared_ptr<Task> dependency = weakDependency.lock();

            return dependency != nullptr
                && dependency->getStatus() == TaskStatus::COMPLETED;
        }
    );
}

std::vector<std::shared_ptr<Task>> Task::getDependencies() const {
    std::vector<std::shared_ptr<Task>> dependencies;
    for (const auto& weakDependency : this->dependencies) {
        std::shared_ptr<Task> dependency = weakDependency.lock();
        if (dependency != nullptr) {
            dependencies.push_back(dependency);
        }
    }
    return dependencies;
}