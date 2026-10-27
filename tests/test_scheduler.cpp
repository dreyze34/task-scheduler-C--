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
#include "TaskScheduler.hpp"

namespace {

// Concrete task for testing the scheduler without adding output from run().
class TestTask : public Task {
public:
    using Task::Task;

    int runCount = 0;
    TaskStatus statusDuringRun = TaskStatus::PENDING;

protected:
    void run() override {
        ++runCount;
        statusDuringRun = getStatus();
    }
};

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
    const TestTask task(42, "Compile sources");
    CHECK(task.getId() == 42);
    CHECK(task.getName() == "Compile sources");
    CHECK(task.getStatus() == TaskStatus::PENDING);
    CHECK(task.getDependencies().empty());
    CHECK(task.canExecute());
}

void taskStatusChanges() {
    TestTask task(1, "Task");
    for (const auto status : {TaskStatus::RUNNING, TaskStatus::COMPLETED,
                              TaskStatus::FAILED, TaskStatus::PENDING}) {
        task.setStatus(status);
        CHECK(task.getStatus() == status);
    }
}

void createAndRetrieveTasks() {
    TaskScheduler scheduler;
    auto first = scheduler.createTask<TestTask>("Same name");
    auto second = scheduler.createTask<TestTask>("Same name");
    auto unnamed = scheduler.createTask<TestTask>("");
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
    auto task = scheduler.createTask<TestTask>("Existing");
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
    auto removed = scheduler.createTask<TestTask>("Removed");
    auto retained = scheduler.createTask<TestTask>("Retained");
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
    auto first = scheduler.createTask<TestTask>("First");
    auto second = scheduler.createTask<TestTask>("Second");
    CHECK(scheduler.removeTask(second->getId()));
    auto third = scheduler.createTask<TestTask>("Third");
    CHECK(third->getId() > second->getId());
    CHECK(scheduler.getTask(first->getId()) == first);
}

void schedulersHaveIndependentState() {
    TaskScheduler first;
    TaskScheduler second;
    auto a = first.createTask<TestTask>("A");
    auto b = second.createTask<TestTask>("B");
    CHECK(a->getId() == b->getId());
    CHECK(a != b);
    CHECK(first.removeTask(a->getId()));
    CHECK(second.getTask(b->getId()) == b);
}

void taskExecution() {
    TestTask task(7, "Run");
    CaptureOutput output;
    task.execute();
    CHECK(task.getStatus() == TaskStatus::COMPLETED);
    CHECK(task.runCount == 1);
    CHECK(task.statusDuringRun == TaskStatus::RUNNING);
    CHECK(output.str() == "Running task 7.\nTask 7 successfully finished.\n");
}

void schedulerExecutesSelectedTask() {
    TaskScheduler scheduler;
    auto selected = scheduler.createTask<TestTask>("Selected");
    auto other = scheduler.createTask<TestTask>("Other");
    CaptureOutput output;
    scheduler.executeTask(selected->getId());
    CHECK(selected->getStatus() == TaskStatus::COMPLETED);
    CHECK(other->getStatus() == TaskStatus::PENDING);
    CHECK(selected->runCount == 1);
    CHECK(selected->statusDuringRun == TaskStatus::RUNNING);
    CHECK(other->runCount == 0);
    CHECK(!output.str().empty());
}

void removedTaskIsNotExecuted() {
    TaskScheduler scheduler;
    auto task = scheduler.createTask<TestTask>("Removed");
    CHECK(scheduler.removeTask(task->getId()));
    CaptureOutput output;
    scheduler.executeTask(task->getId());
    CHECK(task->getStatus() == TaskStatus::PENDING);
    CHECK(output.str().empty());
}

void printTask() {
    TestTask task(7, "Build");
    CaptureOutput output;
    task.print();
    CHECK(output.str() == "Task 7: Build\n");
}

void printAllTasks() {
    TaskScheduler scheduler;
    scheduler.createTask<TestTask>("Build");
    scheduler.createTask<TestTask>("Test");
    auto removed = scheduler.createTask<TestTask>("Removed");
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
    TestTask task(1, "Task");
    task.addDependency(nullptr);
    CHECK(task.getDependencies().empty());
    CHECK(task.canExecute());
}

void dependencyStatusControlsReadiness() {
    auto prerequisite = std::make_shared<TestTask>(1, "Prerequisite");
    TestTask dependent(2, "Dependent");
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
    auto first = std::make_shared<TestTask>(1, "First");
    auto second = std::make_shared<TestTask>(2, "Second");
    TestTask dependent(3, "Dependent");
    dependent.addDependency(first);
    dependent.addDependency(second);
    CHECK((dependent.getDependencies() == std::vector<std::shared_ptr<Task>>{first, second}));
    first->setStatus(TaskStatus::COMPLETED);
    CHECK(!dependent.canExecute());
    second->setStatus(TaskStatus::COMPLETED);
    CHECK(dependent.canExecute());
}

void expiredDependencyBlocksExecution() {
    TestTask dependent(2, "Dependent");
    std::weak_ptr<Task> observer;
    {
        auto prerequisite = std::make_shared<TestTask>(1, "Prerequisite");
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
    auto prerequisite = scheduler.createTask<TestTask>("Prerequisite");
    auto dependent = scheduler.createTask<TestTask>("Dependent");
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
    auto task = scheduler.createTask<TestTask>("Single");
    CHECK(!scheduler.hasCycle());
    CHECK(scheduler.getExecutionOrder() == std::vector<int>{task->getId()});
}

void independentTasksGraph() {
    TaskScheduler scheduler;
    scheduler.createTask<TestTask>("A");
    scheduler.createTask<TestTask>("B");
    scheduler.createTask<TestTask>("C");
    CHECK(!scheduler.hasCycle());
    checkExecutionOrder(scheduler);
}

void dependencyChain() {
    TaskScheduler scheduler;
    // Create the dependent first so creation order cannot satisfy the test.
    auto last = scheduler.createTask<TestTask>("Last");
    auto middle = scheduler.createTask<TestTask>("Middle");
    auto first = scheduler.createTask<TestTask>("First");
    last->addDependency(middle);
    middle->addDependency(first);
    CHECK(!scheduler.hasCycle());
    CHECK((scheduler.getExecutionOrder() ==
           std::vector<int>{first->getId(), middle->getId(), last->getId()}));
}

void diamondGraph() {
    TaskScheduler scheduler;
    auto root = scheduler.createTask<TestTask>("Root");
    auto left = scheduler.createTask<TestTask>("Left");
    auto right = scheduler.createTask<TestTask>("Right");
    auto leaf = scheduler.createTask<TestTask>("Leaf");
    left->addDependency(root);
    right->addDependency(root);
    leaf->addDependency(left);
    leaf->addDependency(right);
    CHECK(!scheduler.hasCycle());
    checkExecutionOrder(scheduler);
}

void disconnectedAcyclicGraph() {
    TaskScheduler scheduler;
    auto first = scheduler.createTask<TestTask>("First root");
    auto second = scheduler.createTask<TestTask>("Second root");
    scheduler.createTask<TestTask>("Isolated");
    scheduler.createTask<TestTask>("First leaf")->addDependency(first);
    scheduler.createTask<TestTask>("Second leaf")->addDependency(second);
    CHECK(!scheduler.hasCycle());
    checkExecutionOrder(scheduler);
}

void duplicateDependencies() {
    TaskScheduler scheduler;
    auto prerequisite = scheduler.createTask<TestTask>("Prerequisite");
    auto dependent = scheduler.createTask<TestTask>("Dependent");
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
    auto task = scheduler.createTask<TestTask>("Self");
    task->addDependency(task);
    CHECK(!task->canExecute());
    CHECK(scheduler.hasCycle());
    checkRuntimeError([&] { scheduler.getExecutionOrder(); });
}

void twoTaskCycle() {
    TaskScheduler scheduler;
    auto first = scheduler.createTask<TestTask>("First");
    auto second = scheduler.createTask<TestTask>("Second");
    first->addDependency(second);
    second->addDependency(first);
    CHECK(scheduler.hasCycle());
    checkRuntimeError([&] { scheduler.getExecutionOrder(); });
}

void indirectCycle() {
    TaskScheduler scheduler;
    auto first = scheduler.createTask<TestTask>("First");
    auto second = scheduler.createTask<TestTask>("Second");
    auto third = scheduler.createTask<TestTask>("Third");
    first->addDependency(second);
    second->addDependency(third);
    third->addDependency(first);
    CHECK(scheduler.hasCycle());
    checkRuntimeError([&] { scheduler.getExecutionOrder(); });
}

void disconnectedCycle() {
    TaskScheduler scheduler;
    auto root = scheduler.createTask<TestTask>("Root");
    scheduler.createTask<TestTask>("Leaf")->addDependency(root);
    scheduler.createTask<TestTask>("Isolated");
    auto first = scheduler.createTask<TestTask>("Cycle first");
    auto second = scheduler.createTask<TestTask>("Cycle second");
    first->addDependency(second);
    second->addDependency(first);
    CHECK(scheduler.hasCycle());
    // A partial order containing only the acyclic component must be rejected.
    checkRuntimeError([&] { scheduler.getExecutionOrder(); });
}

void cycleReachableFromAcyclicTask() {
    TaskScheduler scheduler;
    auto entry = scheduler.createTask<TestTask>("Entry");
    auto first = scheduler.createTask<TestTask>("Cycle first");
    auto second = scheduler.createTask<TestTask>("Cycle second");
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
    auto root = scheduler.createTask<TestTask>("Root");
    auto leaf = scheduler.createTask<TestTask>("Leaf");
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
    auto first = scheduler.createTask<TestTask>("First");
    auto second = scheduler.createTask<TestTask>("Second");
    checkExecutionOrder(scheduler);
    first->addDependency(second);
    CHECK((scheduler.getExecutionOrder() == std::vector<int>{second->getId(), first->getId()}));
    second->addDependency(first);
    CHECK(scheduler.hasCycle());
    checkRuntimeError([&] { scheduler.getExecutionOrder(); });
}

void executionOrderExcludesRemovedIndependentTask() {
    TaskScheduler scheduler;
    auto removed = scheduler.createTask<TestTask>("Removed");
    scheduler.createTask<TestTask>("Retained");
    CHECK(scheduler.removeTask(removed->getId()));
    checkExecutionOrder(scheduler);
}

void largerAcyclicGraph() {
    TaskScheduler scheduler;
    std::vector<std::shared_ptr<Task>> tasks;
    for (int index = 0; index < 128; ++index) {
        tasks.push_back(scheduler.createTask<TestTask>("Task " + std::to_string(index)));
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
    auto last = scheduler.createTask<TestTask>("Last");
    auto first = scheduler.createTask<TestTask>("First");
    auto left = scheduler.createTask<TestTask>("Left");
    auto right = scheduler.createTask<TestTask>("Right");
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

std::string taskExecutionOutput(int id) {
    return "Running task " + std::to_string(id) + ".\nTask " +
           std::to_string(id) + " successfully finished.\n";
}

void checkTasksExecutedOnce(const std::vector<std::shared_ptr<Task>>& tasks,
                           const std::string& output) {
    std::size_t expectedOutputSize = 0;
    for (const auto& task : tasks) {
        CHECK(task->getStatus() == TaskStatus::COMPLETED);
        const auto trace = taskExecutionOutput(task->getId());
        const auto position = output.find(trace);
        CHECK(position != std::string::npos);
        CHECK(output.find(trace, position + trace.size()) == std::string::npos);
        expectedOutputSize += trace.size();
        for (const auto& dependency : task->getDependencies()) {
            const auto dependencyPosition = output.find(taskExecutionOutput(dependency->getId()));
            CHECK(dependencyPosition != std::string::npos);
            CHECK(dependencyPosition < position);
        }
    }
    CHECK(output.size() == expectedOutputSize);
}

void executeAllEmptyScheduler() {
    TaskScheduler scheduler;
    CaptureOutput output;
    scheduler.executeAll();
    CHECK(output.str().empty());
    CHECK(scheduler.getTaskIds().empty());
}

void executeAllSingleTask() {
    TaskScheduler scheduler;
    auto task = scheduler.createTask<TestTask>("Single");
    CaptureOutput output;
    scheduler.executeAll();
    checkTasksExecutedOnce({task}, output.str());
}

void executeAllIndependentTasks() {
    TaskScheduler scheduler;
    auto first = scheduler.createTask<TestTask>("First");
    auto second = scheduler.createTask<TestTask>("Second");
    auto third = scheduler.createTask<TestTask>("Third");
    CaptureOutput output;
    scheduler.executeAll();
    checkTasksExecutedOnce({first, second, third}, output.str());
}

void executeAllDependencyChain() {
    TaskScheduler scheduler;
    // The required execution order is the reverse of creation order.
    auto last = scheduler.createTask<TestTask>("Last");
    auto middle = scheduler.createTask<TestTask>("Middle");
    auto first = scheduler.createTask<TestTask>("First");
    last->addDependency(middle);
    middle->addDependency(first);
    CaptureOutput output;
    scheduler.executeAll();
    checkTasksExecutedOnce({first, middle, last}, output.str());
}

void executeAllDiamondAndIndependentTask() {
    TaskScheduler scheduler;
    auto leaf = scheduler.createTask<TestTask>("Leaf");
    auto left = scheduler.createTask<TestTask>("Left");
    auto right = scheduler.createTask<TestTask>("Right");
    auto root = scheduler.createTask<TestTask>("Root");
    auto independent = scheduler.createTask<TestTask>("Independent");
    leaf->addDependency(left);
    leaf->addDependency(right);
    left->addDependency(root);
    right->addDependency(root);
    CaptureOutput output;
    scheduler.executeAll();
    checkTasksExecutedOnce({leaf, left, right, root, independent}, output.str());
}

void executeAllRejectsSelfCycle() {
    TaskScheduler scheduler;
    auto task = scheduler.createTask<TestTask>("Self");
    task->addDependency(task);
    CaptureOutput output;
    checkRuntimeError([&] { scheduler.executeAll(); });
    CHECK(task->getStatus() == TaskStatus::PENDING);
    CHECK(output.str().empty());
}

void executeAllRejectsDisconnectedCycleBeforeExecuting() {
    TaskScheduler scheduler;
    auto independent = scheduler.createTask<TestTask>("Independent");
    auto first = scheduler.createTask<TestTask>("First");
    auto second = scheduler.createTask<TestTask>("Second");
    auto third = scheduler.createTask<TestTask>("Third");
    first->addDependency(second);
    second->addDependency(third);
    third->addDependency(first);
    CaptureOutput output;
    checkRuntimeError([&] { scheduler.executeAll(); });
    for (const auto& task : {independent, first, second, third}) {
        CHECK(task->getStatus() == TaskStatus::PENDING);
    }
    CHECK(output.str().empty());
}

void executeAllSkipsBlockedTasksAndContinues() {
    TaskScheduler scheduler;
    auto blocked = scheduler.createTask<TestTask>("Blocked");
    auto dependent = scheduler.createTask<TestTask>("Dependent on blocked task");
    auto independent = scheduler.createTask<TestTask>("Independent");
    dependent->addDependency(blocked);
    {
        auto expired = std::make_shared<TestTask>(100, "Expired prerequisite");
        blocked->addDependency(expired);
    }
    CHECK(!blocked->canExecute());
    CaptureOutput output;
    scheduler.executeAll();
    CHECK(blocked->getStatus() == TaskStatus::PENDING);
    CHECK(dependent->getStatus() == TaskStatus::PENDING);
    checkTasksExecutedOnce({independent}, output.str());
}

void executeAllExcludesRemovedTask() {
    TaskScheduler scheduler;
    auto removed = scheduler.createTask<TestTask>("Removed");
    auto retained = scheduler.createTask<TestTask>("Retained");
    CHECK(scheduler.removeTask(removed->getId()));
    CaptureOutput output;
    scheduler.executeAll();
    CHECK(removed->getStatus() == TaskStatus::PENDING);
    checkTasksExecutedOnce({retained}, output.str());
}

void executeAllWithDuplicateDependencies() {
    TaskScheduler scheduler;
    auto dependent = scheduler.createTask<TestTask>("Dependent");
    auto prerequisite = scheduler.createTask<TestTask>("Prerequisite");
    dependent->addDependency(prerequisite);
    dependent->addDependency(prerequisite);
    CaptureOutput output;
    scheduler.executeAll();
    checkTasksExecutedOnce({dependent, prerequisite}, output.str());
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
    {"executeAll on empty scheduler", executeAllEmptyScheduler},
    {"executeAll on single task", executeAllSingleTask},
    {"executeAll on independent tasks", executeAllIndependentTasks},
    {"executeAll respects dependency chain", executeAllDependencyChain},
    {"executeAll on diamond and independent task", executeAllDiamondAndIndependentTask},
    {"executeAll rejects self cycle", executeAllRejectsSelfCycle},
    {"executeAll rejects disconnected cycle before executing", executeAllRejectsDisconnectedCycleBeforeExecuting},
    {"executeAll skips blocked tasks and continues", executeAllSkipsBlockedTasksAndContinues},
    {"executeAll excludes removed task", executeAllExcludesRemovedTask},
    {"executeAll with duplicate dependencies", executeAllWithDuplicateDependencies},
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
