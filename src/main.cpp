#include <cassert>
#include <iostream>
#include <memory>

#include <Task.hpp>

int main()
{
    TaskScheduler scheduler;

    std::shared_ptr<Task> dependency = scheduler.createTask("Dependance");
    std::shared_ptr<Task> task = scheduler.createTask("Tache principale");

    assert(dependency != nullptr);
    assert(task != nullptr);
    assert(dependency->getId() == 1);
    assert(task->getId() == 2);
    assert(dependency->getStatus() == TaskStatus::PENDING);
    assert(task->getStatus() == TaskStatus::PENDING);

    std::cout << "Taches creees avec succes.\n";

    task->addDependency(dependency);

    std::vector<std::shared_ptr<Task>> dependencies =
        task->getDependencies();

    assert(dependencies.size() == 1);
    assert(dependencies.front() == dependency);
    assert(!task->canExecute());

    std::cout << "Dependance ajoutee avec succes.\n";

    scheduler.executeTask(dependency->getId());
    assert(dependency->getStatus() == TaskStatus::COMPLETED);
    assert(task->canExecute());

    std::cout << "Execution et verification de la dependance reussies.\n";

    std::shared_ptr<Task> foundTask = scheduler.getTask(task->getId());
    assert(foundTask == task);

    std::cout << "Recherche de la tache reussie.\n";

    const int dependencyId = dependency->getId();
    assert(scheduler.removeTask(dependencyId));
    assert(scheduler.getTask(dependencyId) == nullptr);
    assert(dependency != nullptr);
    assert(!task->getDependencies().empty());

    dependencies.clear();
    dependency.reset();
    assert(task->getDependencies().empty());
    assert(!task->canExecute());

    std::cout << "Expiration de la weak_ptr verifiee.\n";

    const std::vector<int> taskIds = scheduler.getTaskIds();
    assert(taskIds.size() == 1);
    assert(taskIds.front() == task->getId());

    assert(scheduler.removeTask(task->getId()));
    assert(scheduler.getTask(task->getId()) == nullptr);
    assert(task != nullptr);

    std::cout << "Tous les tests sont passes.\n";

    return 0;
}