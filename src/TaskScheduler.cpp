#include <Task.hpp>

std::shared_ptr<Task> TaskScheduler::createTask(const std::string& name) {
    const int id = this->nextId;

    this->tasks[id] = std::make_shared<Task>(id, name);
    this->nextId++;

    return this->tasks.at(id);
}

std::shared_ptr<Task> TaskScheduler::getTask(int id) const {
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

std::vector<int> TaskScheduler::getExecutionOrder() const {
    std::unordered_map<int, int> inDegree;
    std::unordered_map<int, std::vector<int>> reverseDep;
    for (const auto& [key, value] : tasks) {
        auto dep = value->getDependencies();
        inDegree[key] += dep.size();
        for (const auto& d : dep) {
            reverseDep[d->getId()].push_back(key);
        }
    }

    std::queue<int> queue;
    for (const auto& [key, val] : inDegree) {
        if (val == 0) {
            queue.push(key);
        }
    }

    std::vector<int> res;
    while (!queue.empty()) {
        int curr_id = queue.front();
        queue.pop();
        res.push_back(curr_id);
        for (const auto& d : reverseDep[curr_id]) {
            inDegree[d]--;
            if (inDegree[d] == 0) {
                queue.push(d);
            }
        }
    }

    if (res.size() != tasks.size()) {
        throw std::runtime_error("Cycle detected in task dependencies");
    }

    return res;
}