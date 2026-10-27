#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Task.hpp"

namespace {

// Unlike assert(), these checks also run when NDEBUG is defined (Release builds).
#define CHECK(expression)                                                     \
    do {                                                                      \
        if (!(expression)) {                                                  \
            throw std::runtime_error(std::string(__FILE__) + ":" +             \
                                     std::to_string(__LINE__) +                \
                                     ": check failed: " #expression);          \
        }                                                                     \
    } while (false)

class CaptureOutput {
public:
    CaptureOutput() : previous(std::cout.rdbuf(buffer.rdbuf())) {}
    ~CaptureOutput() { std::cout.rdbuf(previous); }

    CaptureOutput(const CaptureOutput&) = delete;
    CaptureOutput& operator=(const CaptureOutput&) = delete;

    std::string str() const { return buffer.str(); }

private:
    std::ostringstream buffer;
    std::streambuf* previous;
};

template <typename Function>
void checkRuntimeError(Function function) {
    bool thrown = false;
    try {
        function();
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    CHECK(thrown);
}

// unordered_map iteration order is unspecified: check membership, uniqueness,
// and every dependency constraint instead of one arbitrary topological order.
void checkExecutionOrder(const TaskScheduler& scheduler) {
    const auto order = scheduler.getExecutionOrder();
    const auto ids = scheduler.getTaskIds();
    CHECK(order.size() == ids.size());

    std::unordered_map<int, std::size_t> positions;
    for (std::size_t index = 0; index < order.size(); ++index) {
        CHECK(scheduler.getTask(order[index]) != nullptr);
        CHECK(positions.emplace(order[index], index).second);
    }
    for (const int id : ids) {
        CHECK(positions.contains(id));
        for (const auto& dependency : scheduler.getTask(id)->getDependencies()) {
            CHECK(positions.contains(dependency->getId()));
            CHECK(positions.at(dependency->getId()) < positions.at(id));
        }
    }
}

void emptyScheduler() {
    const TaskScheduler scheduler;
    CHECK(scheduler.getTaskIds().empty());
    CHECK(scheduler.getTask(1) == nullptr);
    CHECK(!scheduler.hasCycle());
    CHECK(scheduler.getExecutionOrder().empty());
    CaptureOutput output;
    scheduler.printAllTasks();
    CHECK(output.str().empty());
}

void taskInitialState() {
    const Task task(42, "Compile sources");
    CHECK(task.getId() == 42);
    CHECK(task.getName() == "Compile sources");
    CHECK(task.getStatus() == TaskStatus::PENDING);
    CHECK(task.getDependencies().empty());
    CHECK(task.canExecute());
}

void taskStatusChanges() {
    Task task(1, "Task");
    for (const auto status : {TaskStatus::RUNNING, TaskStatus::COMPLETED,
                              TaskStatus::FAILED, TaskStatus::PENDING}) {
        task.setStatus(status);
        CHECK(task.getStatus() == status);
    }
}

void createAndRetrieveTasks() {
    TaskScheduler scheduler;
    auto first = scheduler.createTask("Same name");
    auto second = scheduler.createTask("Same name");
    auto unnamed = scheduler.createTask("");
    CHECK(first != nullptr);
    CHECK(second != nullptr);
    CHECK(unnamed != nullptr);
    CHECK(first->getId() == 1);
    CHECK(second->getId() == 2);
    CHECK(unnamed->getId() == 3);
    CHECK(first != second);
    CHECK(first->getName() == "Same name");
    CHECK(second->getName() == "Same name");
    CHECK(unnamed->getName().empty());
    CHECK(first->getStatus() == TaskStatus::PENDING);
    const auto& readOnlyScheduler = scheduler;
    CHECK(readOnlyScheduler.getTask(first->getId()) == first);
    CHECK(readOnlyScheduler.getTask(second->getId()) == second);
    auto ids = scheduler.getTaskIds();
    std::sort(ids.begin(), ids.end());
    CHECK((ids == std::vector<int>{1, 2, 3}));
}

void missingTaskOperations() {
    TaskScheduler scheduler;
    auto task = scheduler.createTask("Existing");
    CaptureOutput output;
    for (const int id : {-1, 0, 999}) {
        CHECK(scheduler.getTask(id) == nullptr);
        CHECK(!scheduler.removeTask(id));
        scheduler.executeTask(id);
    }
    CHECK(output.str().empty());
    CHECK(scheduler.getTaskIds() == std::vector<int>{task->getId()});
    CHECK(task->getStatus() == TaskStatus::PENDING);
}

void removeTaskAndKeepOtherTasks() {
    TaskScheduler scheduler;
    auto removed = scheduler.createTask("Removed");
    auto retained = scheduler.createTask("Retained");
    CHECK(scheduler.removeTask(removed->getId()));
    CHECK(!scheduler.removeTask(removed->getId()));
    CHECK(scheduler.getTask(removed->getId()) == nullptr);
    CHECK(scheduler.getTask(retained->getId()) == retained);
    CHECK(scheduler.getTaskIds() == std::vector<int>{retained->getId()});
    CHECK(removed->getName() == "Removed"); // External shared ownership survives.
    CHECK(scheduler.removeTask(retained->getId()));
    CHECK(scheduler.getTaskIds().empty());
}

void idsAreNotReused() {
    TaskScheduler scheduler;
    auto first = scheduler.createTask("First");
    auto second = scheduler.createTask("Second");
    CHECK(scheduler.removeTask(second->getId()));
    auto third = scheduler.createTask("Third");
    CHECK(third->getId() > second->getId());
    CHECK(scheduler.getTask(first->getId()) == first);
}

void schedulersHaveIndependentState() {
    TaskScheduler first;
    TaskScheduler second;
    auto a = first.createTask("A");
    auto b = second.createTask("B");
    CHECK(a->getId() == b->getId());
    CHECK(a != b);
    CHECK(first.removeTask(a->getId()));
    CHECK(second.getTask(b->getId()) == b);
}

void taskExecution() {
    Task task(7, "Run");
    CaptureOutput output;
    task.execute();
    CHECK(task.getStatus() == TaskStatus::COMPLETED);
    CHECK(output.str() == "Running task 7.\nTask 7 successfully finished.\n");
}

void schedulerExecutesSelectedTask() {
    TaskScheduler scheduler;
    auto selected = scheduler.createTask("Selected");
    auto other = scheduler.createTask("Other");
    CaptureOutput output;
    scheduler.executeTask(selected->getId());
    CHECK(selected->getStatus() == TaskStatus::COMPLETED);
    CHECK(other->getStatus() == TaskStatus::PENDING);
    CHECK(!output.str().empty());
}

void removedTaskIsNotExecuted() {
    TaskScheduler scheduler;
    auto task = scheduler.createTask("Removed");
    CHECK(scheduler.removeTask(task->getId()));
    CaptureOutput output;
    scheduler.executeTask(task->getId());
    CHECK(task->getStatus() == TaskStatus::PENDING);
    CHECK(output.str().empty());
}

void printTask() {
    Task task(7, "Build");
    CaptureOutput output;
    task.print();
    CHECK(output.str() == "Task 7: Build\n");
}

void printAllTasks() {
    TaskScheduler scheduler;
    scheduler.createTask("Build");
    scheduler.createTask("Test");
    auto removed = scheduler.createTask("Removed");
    CHECK(scheduler.removeTask(removed->getId()));
    CaptureOutput output;
    scheduler.printAllTasks();
    std::istringstream lines(output.str());
    std::vector<std::string> actual;
    for (std::string line; std::getline(lines, line);) {
        actual.push_back(line);
    }
    std::sort(actual.begin(), actual.end());
    CHECK((actual == std::vector<std::string>{"Task 1: Build", "Task 2: Test"}));
}

void nullDependencyIsIgnored() {
    Task task(1, "Task");
    task.addDependency(nullptr);
    CHECK(task.getDependencies().empty());
    CHECK(task.canExecute());
}

void dependencyStatusControlsReadiness() {
    auto prerequisite = std::make_shared<Task>(1, "Prerequisite");
    Task dependent(2, "Dependent");
    dependent.addDependency(prerequisite);
    CHECK(dependent.getDependencies() == std::vector<std::shared_ptr<Task>>{prerequisite});
    for (const auto status : {TaskStatus::PENDING, TaskStatus::RUNNING, TaskStatus::FAILED}) {
        prerequisite->setStatus(status);
        CHECK(!dependent.canExecute());
    }
    prerequisite->setStatus(TaskStatus::COMPLETED);
    CHECK(dependent.canExecute());
    prerequisite->setStatus(TaskStatus::FAILED);
    CHECK(!dependent.canExecute());
}

void allDependenciesMustComplete() {
    auto first = std::make_shared<Task>(1, "First");
    auto second = std::make_shared<Task>(2, "Second");
    Task dependent(3, "Dependent");
    dependent.addDependency(first);
    dependent.addDependency(second);
    CHECK((dependent.getDependencies() == std::vector<std::shared_ptr<Task>>{first, second}));
    first->setStatus(TaskStatus::COMPLETED);
    CHECK(!dependent.canExecute());
    second->setStatus(TaskStatus::COMPLETED);
    CHECK(dependent.canExecute());
}

void expiredDependencyBlocksExecution() {
    Task dependent(2, "Dependent");
    std::weak_ptr<Task> observer;
    {
        auto prerequisite = std::make_shared<Task>(1, "Prerequisite");
        observer = prerequisite;
        prerequisite->setStatus(TaskStatus::COMPLETED);
        dependent.addDependency(prerequisite);
        CHECK(dependent.canExecute());
    }
    CHECK(observer.expired());
    CHECK(dependent.getDependencies().empty());
    CHECK(!dependent.canExecute());
}

void removingTaskReleasesSchedulerOwnership() {
    TaskScheduler scheduler;
    auto prerequisite = scheduler.createTask("Prerequisite");
    auto dependent = scheduler.createTask("Dependent");
    dependent->addDependency(prerequisite);
    const int id = prerequisite->getId();
    std::weak_ptr<Task> observer = prerequisite;
    prerequisite.reset();
    CHECK(!observer.expired());
    CHECK(scheduler.removeTask(id));
    CHECK(observer.expired());
    CHECK(dependent->getDependencies().empty());
    CHECK(!dependent->canExecute());
}

void singleTaskGraph() {
    TaskScheduler scheduler;
    auto task = scheduler.createTask("Single");
    CHECK(!scheduler.hasCycle());
    CHECK(scheduler.getExecutionOrder() == std::vector<int>{task->getId()});
}

void independentTasksGraph() {
    TaskScheduler scheduler;
    scheduler.createTask("A");
    scheduler.createTask("B");
    scheduler.createTask("C");
    CHECK(!scheduler.hasCycle());
    checkExecutionOrder(scheduler);
}

void dependencyChain() {
    TaskScheduler scheduler;
    // Create the dependent first so creation order cannot satisfy the test.
    auto last = scheduler.createTask("Last");
    auto middle = scheduler.createTask("Middle");
    auto first = scheduler.createTask("First");
    last->addDependency(middle);
    middle->addDependency(first);
    CHECK(!scheduler.hasCycle());
    CHECK((scheduler.getExecutionOrder() ==
           std::vector<int>{first->getId(), middle->getId(), last->getId()}));
}

void diamondGraph() {
    TaskScheduler scheduler;
    auto root = scheduler.createTask("Root");
    auto left = scheduler.createTask("Left");
    auto right = scheduler.createTask("Right");
    auto leaf = scheduler.createTask("Leaf");
    left->addDependency(root);
    right->addDependency(root);
    leaf->addDependency(left);
    leaf->addDependency(right);
    CHECK(!scheduler.hasCycle());
    checkExecutionOrder(scheduler);
}

void disconnectedAcyclicGraph() {
    TaskScheduler scheduler;
    auto first = scheduler.createTask("First root");
    auto second = scheduler.createTask("Second root");
    scheduler.createTask("Isolated");
    scheduler.createTask("First leaf")->addDependency(first);
    scheduler.createTask("Second leaf")->addDependency(second);
    CHECK(!scheduler.hasCycle());
    checkExecutionOrder(scheduler);
}

void duplicateDependencies() {
    TaskScheduler scheduler;
    auto prerequisite = scheduler.createTask("Prerequisite");
    auto dependent = scheduler.createTask("Dependent");
    dependent->addDependency(prerequisite);
    dependent->addDependency(prerequisite);
    CHECK(!dependent->canExecute());
    CHECK(!scheduler.hasCycle());
    checkExecutionOrder(scheduler);
    prerequisite->setStatus(TaskStatus::COMPLETED);
    CHECK(dependent->canExecute());
}

void selfCycle() {
    TaskScheduler scheduler;
    auto task = scheduler.createTask("Self");
    task->addDependency(task);
    CHECK(!task->canExecute());
    CHECK(scheduler.hasCycle());
    checkRuntimeError([&] { scheduler.getExecutionOrder(); });
}

void twoTaskCycle() {
    TaskScheduler scheduler;
    auto first = scheduler.createTask("First");
    auto second = scheduler.createTask("Second");
    first->addDependency(second);
    second->addDependency(first);
    CHECK(scheduler.hasCycle());
    checkRuntimeError([&] { scheduler.getExecutionOrder(); });
}

void indirectCycle() {
    TaskScheduler scheduler;
    auto first = scheduler.createTask("First");
    auto second = scheduler.createTask("Second");
    auto third = scheduler.createTask("Third");
    first->addDependency(second);
    second->addDependency(third);
    third->addDependency(first);
    CHECK(scheduler.hasCycle());
    checkRuntimeError([&] { scheduler.getExecutionOrder(); });
}

void disconnectedCycle() {
    TaskScheduler scheduler;
    auto root = scheduler.createTask("Root");
    scheduler.createTask("Leaf")->addDependency(root);
    scheduler.createTask("Isolated");
    auto first = scheduler.createTask("Cycle first");
    auto second = scheduler.createTask("Cycle second");
    first->addDependency(second);
    second->addDependency(first);
    CHECK(scheduler.hasCycle());
    // A partial order containing only the acyclic component must be rejected.
    checkRuntimeError([&] { scheduler.getExecutionOrder(); });
}

void cycleReachableFromAcyclicTask() {
    TaskScheduler scheduler;
    auto entry = scheduler.createTask("Entry");
    auto first = scheduler.createTask("Cycle first");
    auto second = scheduler.createTask("Cycle second");
    entry->addDependency(first);
    first->addDependency(second);
    second->addDependency(first);
    std::unordered_set<int> visited;
    CHECK(scheduler.hasTaskCycle(entry, visited));
    CHECK(scheduler.hasCycle());
    checkRuntimeError([&] { scheduler.getExecutionOrder(); });
}

void cycleTraversalTracksVisitedTasks() {
    TaskScheduler scheduler;
    auto root = scheduler.createTask("Root");
    auto leaf = scheduler.createTask("Leaf");
    leaf->addDependency(root);
    std::unordered_set<int> visited;
    CHECK(!scheduler.hasTaskCycle(leaf, visited));
    CHECK(visited.size() == 2);
    CHECK(visited.contains(root->getId()));
    CHECK(visited.contains(leaf->getId()));
    CHECK(!scheduler.hasTaskCycle(leaf, visited));
}

void executionOrderReflectsGraphChanges() {
    TaskScheduler scheduler;
    auto first = scheduler.createTask("First");
    auto second = scheduler.createTask("Second");
    checkExecutionOrder(scheduler);
    first->addDependency(second);
    CHECK((scheduler.getExecutionOrder() == std::vector<int>{second->getId(), first->getId()}));
    second->addDependency(first);
    CHECK(scheduler.hasCycle());
    checkRuntimeError([&] { scheduler.getExecutionOrder(); });
}

void executionOrderExcludesRemovedIndependentTask() {
    TaskScheduler scheduler;
    auto removed = scheduler.createTask("Removed");
    scheduler.createTask("Retained");
    CHECK(scheduler.removeTask(removed->getId()));
    checkExecutionOrder(scheduler);
}

void largerAcyclicGraph() {
    TaskScheduler scheduler;
    std::vector<std::shared_ptr<Task>> tasks;
    for (int index = 0; index < 128; ++index) {
        tasks.push_back(scheduler.createTask("Task " + std::to_string(index)));
    }
    for (std::size_t index = 1; index < tasks.size(); ++index) {
        tasks[index - 1]->addDependency(tasks[index]);
        if (index + 3 < tasks.size()) {
            tasks[index - 1]->addDependency(tasks[index + 3]);
        }
    }
    CHECK(!scheduler.hasCycle());
    checkExecutionOrder(scheduler);
    checkExecutionOrder(scheduler);
    for (const auto& task : tasks) {
        CHECK(task->getStatus() == TaskStatus::PENDING);
    }
}

void executeTasksInDependencyOrder() {
    TaskScheduler scheduler;
    auto last = scheduler.createTask("Last");
    auto first = scheduler.createTask("First");
    auto left = scheduler.createTask("Left");
    auto right = scheduler.createTask("Right");
    left->addDependency(first);
    right->addDependency(first);
    last->addDependency(left);
    last->addDependency(right);
    CaptureOutput output;
    for (const int id : scheduler.getExecutionOrder()) {
        auto task = scheduler.getTask(id);
        CHECK(task->canExecute());
        scheduler.executeTask(id);
        CHECK(task->getStatus() == TaskStatus::COMPLETED);
    }
    for (const int id : scheduler.getTaskIds()) {
        CHECK(scheduler.getTask(id)->getStatus() == TaskStatus::COMPLETED);
    }
    CHECK(!scheduler.hasCycle());
    checkExecutionOrder(scheduler);
}

struct TestCase {
    const char* name;
    void (*run)();
};

const TestCase tests[] = {
    {"empty scheduler", emptyScheduler},
    {"task initial state", taskInitialState},
    {"task status changes", taskStatusChanges},
    {"create and retrieve tasks", createAndRetrieveTasks},
    {"missing task operations", missingTaskOperations},
    {"remove task and keep other tasks", removeTaskAndKeepOtherTasks},
    {"IDs are not reused", idsAreNotReused},
    {"schedulers have independent state", schedulersHaveIndependentState},
    {"task execution", taskExecution},
    {"scheduler executes selected task", schedulerExecutesSelectedTask},
    {"removed task is not executed", removedTaskIsNotExecuted},
    {"print task", printTask},
    {"print all tasks", printAllTasks},
    {"null dependency is ignored", nullDependencyIsIgnored},
    {"dependency status controls readiness", dependencyStatusControlsReadiness},
    {"all dependencies must complete", allDependenciesMustComplete},
    {"expired dependency blocks execution", expiredDependencyBlocksExecution},
    {"removing task releases scheduler ownership", removingTaskReleasesSchedulerOwnership},
    {"single task graph", singleTaskGraph},
    {"independent tasks graph", independentTasksGraph},
    {"dependency chain", dependencyChain},
    {"diamond graph", diamondGraph},
    {"disconnected acyclic graph", disconnectedAcyclicGraph},
    {"duplicate dependencies", duplicateDependencies},
    {"self cycle", selfCycle},
    {"two task cycle", twoTaskCycle},
    {"indirect cycle", indirectCycle},
    {"disconnected cycle", disconnectedCycle},
    {"cycle reachable from acyclic task", cycleReachableFromAcyclicTask},
    {"cycle traversal tracks visited tasks", cycleTraversalTracksVisitedTasks},
    {"execution order reflects graph changes", executionOrderReflectsGraphChanges},
    {"execution order excludes removed independent task", executionOrderExcludesRemovedIndependentTask},
    {"larger acyclic graph", largerAcyclicGraph},
    {"execute tasks in dependency order", executeTasksInDependencyOrder},
};

} // namespace

int main() {
    int failed = 0;
    int total = 0;
    for (const auto& test : tests) {
        ++total;
        try {
            test.run();
            std::cout << "[PASS] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failed;
            std::cerr << "[FAIL] " << test.name << ": " << error.what() << '\n';
        } catch (...) {
            ++failed;
            std::cerr << "[FAIL] " << test.name << ": unknown exception\n";
        }
    }
    std::cout << total - failed << '/' << total << " tests passed\n";
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
