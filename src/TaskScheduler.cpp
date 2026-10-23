#include <Task.hpp>

std::shared_ptr<Task> TaskScheduler::createTask(const std::string& name) {
    const int id = this->nextId;

    this->tasks[id] = std::make_shared<Task>(id, name);
    this->nextId++;

    return this->tasks.at(id);
}

std::shared_ptr<Task> TaskScheduler::getTask(int id) {
    auto it = this->tasks.find(id);

    if (it != this->tasks.end()) {
        return it->second;
    }
    return nullptr;
}

bool TaskScheduler::removeTask(int id) {
    size_t count = this->tasks.erase(id);
    if (count > 0) {
        return true;
    }
    return false;
}

void TaskScheduler::executeTask(int id) {
    auto task_ptr = this->getTask(id);
    if (task_ptr != nullptr) {
        task_ptr->execute();
    }
}

std::vector<int> TaskScheduler::getTaskIds() const {
    std::vector<int> ids;
    for (const auto& [key, value] : this->tasks) {
        ids.push_back(key);
    }
    return ids;
}

void TaskScheduler::printAllTasks() const {
    for (const auto& [key, value] : this->tasks) {
        value.get()->print();
    }
}