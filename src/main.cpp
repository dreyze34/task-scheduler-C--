#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "TaskScheduler.hpp"

namespace {

void check(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

} // namespace

int main() {
    try {
        // TEST 1 : La factory fournit les identifiants et transmet les arguments.
        {
            std::cout << "\n=== TEST 1: Factory et polymorphisme ===\n";
            TaskScheduler scheduler;
            const std::string message = "Hello World";
            const std::vector<double> values{1.0, 2.0, 3.0, 4.0};

            std::shared_ptr<PrintTask> printTask = scheduler.createTask<PrintTask>(message);
            std::shared_ptr<ComputeTask> computeTask = scheduler.createTask<ComputeTask>(values);

            check(printTask->getId() == 1, "Le premier identifiant doit etre 1");
            check(computeTask->getId() == 2, "Le deuxieme identifiant doit etre 2");
            check(scheduler.getTask(printTask->getId()) == printTask,
                  "La factory doit enregistrer la tache d'affichage");

            scheduler.executeTask(printTask->getId());
            // Le pointeur de base conserve le comportement de ComputeTask::run().
            std::shared_ptr<Task> baseTask = scheduler.getTask(computeTask->getId());
            check(baseTask == computeTask, "La factory doit enregistrer la tache de calcul");
            baseTask->execute(); // Moyenne attendue : 2.5.

            check(printTask->getStatus() == TaskStatus::COMPLETED,
                  "La tache d'affichage doit etre terminee");
            check(computeTask->getStatus() == TaskStatus::COMPLETED,
                  "La tache de calcul doit etre terminee");
        }

        // TEST 2 : Creation avec des temporaires et dependances en diamant.
        {
            std::cout << "\n=== TEST 2: executeAll et dependances ===\n";
            TaskScheduler scheduler;
            // L'ordre de creation ne correspond pas a l'ordre d'execution.
            auto last = scheduler.createTask<PrintTask>("Fin des deux branches");
            auto compute = scheduler.createTask<ComputeTask>(
                std::vector<double>{10.0, 20.0, 30.0});
            auto branch = scheduler.createTask<PrintTask>("Seconde branche");
            auto first = scheduler.createTask<PrintTask>("Debut du traitement");

            compute->addDependency(first);
            branch->addDependency(first);
            last->addDependency(compute);
            last->addDependency(branch);

            check(first->canExecute(), "La racine doit pouvoir s'executer");
            check(!compute->canExecute() && !branch->canExecute() && !last->canExecute(),
                  "Les autres taches doivent attendre leurs prerequis");
            check(!scheduler.hasCycle(), "Le diamant ne doit pas contenir de cycle");

            scheduler.printAllTasks();
            std::cout << "Ordre d'execution :";
            const auto order = scheduler.getExecutionOrder();
            check(order.size() == 4, "L'ordre doit contenir les quatre taches");
            check(order.front() == first->getId() && order.back() == last->getId(),
                  "La racine doit preceder les branches et la tache finale");
            for (const int id : order) {
                std::cout << ' ' << id;
            }
            std::cout << '\n';
            scheduler.executeAll(); // Moyenne attendue : 20.

            for (const int id : scheduler.getTaskIds()) {
                check(scheduler.getTask(id)->getStatus() == TaskStatus::COMPLETED,
                      "Toutes les taches du diamant doivent etre terminees");
            }
        }

        // TEST 3 : Suppression et conservation du pointeur externe.
        {
            std::cout << "\n=== TEST 3: Suppression et identifiants ===\n";
            TaskScheduler scheduler;
            auto removed = scheduler.createTask<PrintTask>("Cette tache ne doit pas s'executer");
            check(scheduler.removeTask(removed->getId()), "La suppression doit reussir");
            check(!scheduler.removeTask(removed->getId()), "La seconde suppression doit echouer");
            check(scheduler.getTask(removed->getId()) == nullptr,
                  "La tache supprimee ne doit plus etre dans le scheduler");

            auto retained = scheduler.createTask<PrintTask>("Cette tache doit s'executer");
            check(retained->getId() > removed->getId(), "Un identifiant ne doit pas etre reutilise");
            scheduler.executeTask(removed->getId());
            scheduler.executeAll();
            check(removed->getStatus() == TaskStatus::PENDING,
                  "La tache supprimee ne doit pas etre executee");
            check(retained->getStatus() == TaskStatus::COMPLETED,
                  "La tache conservee doit etre executee");
        }

        // TEST 4 : Un cycle empeche aussi l'execution des taches independantes.
        {
            std::cout << "\n=== TEST 4: Rejet d'un cycle ===\n";
            TaskScheduler scheduler;
            scheduler.createTask<PrintTask>("Tache hors du cycle");
            auto first = scheduler.createTask<PrintTask>("Premiere tache du cycle");
            auto second = scheduler.createTask<ComputeTask>(std::vector<double>{2.0, 4.0});
            first->addDependency(second);
            second->addDependency(first);
            check(scheduler.hasCycle(), "Le cycle doit etre detecte");

            bool rejected = false;
            try {
                scheduler.executeAll();
            } catch (const std::runtime_error& error) {
                rejected = true;
                std::cout << "Exception attendue : " << error.what() << '\n';
            }
            check(rejected, "executeAll doit rejeter le cycle");
            for (const int id : scheduler.getTaskIds()) {
                check(scheduler.getTask(id)->getStatus() == TaskStatus::PENDING,
                      "Aucune tache ne doit etre executee en presence du cycle");
            }
        }

        // TEST 5 : executeAll sur un scheduler vide.
        {
            std::cout << "\n=== TEST 5: Scheduler vide ===\n";
            TaskScheduler scheduler;
            scheduler.executeAll();
            check(scheduler.getTaskIds().empty(), "Le scheduler doit rester vide");
            check(scheduler.getExecutionOrder().empty(), "L'ordre d'execution doit etre vide");
        }

        // CAS LIMITE : observation du comportement actuel sur un vecteur vide.
        {
            std::cout << "\n=== CAS LIMITE: Vecteur vide ===\n";
            std::cout << "La moyenne d'un vecteur vide n'est pas definie.\n";
            TaskScheduler scheduler;
            auto task = scheduler.createTask<ComputeTask>(std::vector<double>{});
            scheduler.executeAll();
            std::cout << "Status obtenu : " << static_cast<int>(task->getStatus()) << '\n';
        }

        std::cout << "\nToutes les verifications ont reussi.\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Echec : " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
