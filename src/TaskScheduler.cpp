#include <TaskScheduler.hpp>

#include <queue>
#include <stack>
#include <stdexcept>


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

void TaskScheduler::executeAll() {
    if (hasCycle()) {
        throw std::runtime_error("Cycle detected in task dependencies");
    }
    std::vector<int> executionOrder = getExecutionOrder();
    for (auto& taskId : executionOrder) {
        auto task = getTask(taskId);
        if (task->canExecute()) {
            task->execute();
        }
    }
}

bool TaskScheduler::hasTaskCycle(const std::shared_ptr<Task> &task, std::unordered_set<int> &visited) const
{
    std::unordered_set<int> inPath;
    std::stack<std::pair<std::shared_ptr<Task>, bool>> stack;
    stack.push({task, false});
    while (!stack.empty())
    {
        auto [curr, exiting] = std::move(stack.top());
        auto curr_id = curr->getId();
        stack.pop();
        if (!exiting)
        {
            if (inPath.contains(curr_id))
            {
                return true;
            }
            if (visited.contains(curr_id))
            {
                continue;
            }
            inPath.insert(curr_id);
            visited.insert(curr_id);
            stack.push({curr, true});
            for (const auto &dep : curr->getDependencies())
            {
                stack.push({dep, false});
            }
        }
        else {
            inPath.erase(curr_id);
        }
    }
    return false;
}

bool TaskScheduler::hasCycle() const
{
    std::unordered_set<int> visited;
    for (const auto &[key, value] : this->tasks)
    {
        if (!visited.contains(key) && hasTaskCycle(value, visited))
        {
            return true;
        }
    }
    return false;
}