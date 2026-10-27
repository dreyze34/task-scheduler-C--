
#include <iostream>
#include <memory>
#include <string>
#include "Task.hpp"

void testCycle(const std::string& name,
               const TaskScheduler& scheduler,
               bool expected) {
    bool result = scheduler.hasCycle();

    std::cout << name
              << " | Expected: " << std::boolalpha << expected
              << " | Got: " << result
              << " | " << (result == expected ? "PASS" : "FAIL")
              << '\n';
}

int main() {
    // TEST 1: Graphe sans cycle
    {
        TaskScheduler scheduler;

        auto A = scheduler.createTask("A");
        auto B = scheduler.createTask("B");
        auto C = scheduler.createTask("C");

        B->addDependency(A);
        C->addDependency(B);

        // C -> B -> A
        testCycle("Test 1 - No cycle", scheduler, false);
    }

    // TEST 2: Cycle simple
    {
        TaskScheduler scheduler;

        auto A = scheduler.createTask("A");
        auto B = scheduler.createTask("B");

        A->addDependency(B);
        B->addDependency(A);

        // A -> B -> A
        testCycle("Test 2 - Simple cycle", scheduler, true);
    }

    // TEST 3: Cycle indirect
    {
        TaskScheduler scheduler;

        auto A = scheduler.createTask("A");
        auto B = scheduler.createTask("B");
        auto C = scheduler.createTask("C");

        A->addDependency(B);
        B->addDependency(C);
        C->addDependency(A);

        // A -> B -> C -> A
        testCycle("Test 3 - Indirect cycle", scheduler, true);
    }

    // TEST 4: Cycle dans une composante isolée
    {
        TaskScheduler scheduler;

        auto A = scheduler.createTask("A");
        auto B = scheduler.createTask("B");
        auto C = scheduler.createTask("C");
        auto D = scheduler.createTask("D");
        auto E = scheduler.createTask("E");

        B->addDependency(A);
        C->addDependency(B);

        D->addDependency(E);
        E->addDependency(D);

        // C -> B -> A (sans cycle)
        // D -> E -> D (cycle)
        testCycle("Test 4 - Disconnected cycle", scheduler, true);
    }

    // TEST 5: Auto-dépendance
    {
        TaskScheduler scheduler;

        auto A = scheduler.createTask("A");

        A->addDependency(A);

        // A -> A
        testCycle("Test 5 - Self cycle", scheduler, true);
    }

    // TEST 6: Graphe en diamant sans cycle
    {
        TaskScheduler scheduler;

        auto A = scheduler.createTask("A");
        auto B = scheduler.createTask("B");
        auto C = scheduler.createTask("C");
        auto D = scheduler.createTask("D");

        B->addDependency(A);
        C->addDependency(A);
        D->addDependency(B);
        D->addDependency(C);

        // D -> B -> A
        // D -> C -> A
        testCycle("Test 6 - Diamond graph", scheduler, false);
    }

    return 0;
}
