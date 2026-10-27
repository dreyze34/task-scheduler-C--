# Tests du scheduler

La suite utilise uniquement la bibliothèque standard C++20 et CTest, sans
dépendance externe.

Depuis la racine du projet :

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Pour afficher le résultat de chacun des 44 cas :

```sh
./build/scheduler_tests
```

Les vérifications restent actives en Release et le programme renvoie un code
d'échec dès qu'au moins un cas échoue. CTest exécute les 44 cas comme une suite.

La couverture comprend :

- création, recherche, suppression et identifiants des tâches ;
- états, exécution et affichage ;
- dépendances nulles, multiples, dupliquées ou expirées et conditions d'exécution ;
- durée de vie des tâches avec `shared_ptr` et `weak_ptr` ;
- détection de cycles directs, indirects et dans une composante isolée ;
- ordre topologique sur des chaînes, diamants, composantes indépendantes et un
  graphe de 128 tâches ;
- rejet d'un ordre incomplet en présence d'un cycle ;
- exécution complète des tâches dans l'ordre de leurs dépendances ;
- `executeAll()` : scheduler vide, tâche unique, tâches indépendantes, chaîne,
  diamant, dépendances dupliquées et exclusion des tâches supprimées ;
- rejet des cycles par `executeAll()` avant toute exécution, même pour une
  composante indépendante ;
- tâches bloquées par une dépendance expirée, propagation du blocage et
  poursuite des tâches indépendantes avec `executeAll()`.

Les tests ne supposent aucun ordre entre tâches indépendantes, puisque le
scheduler les stocke dans une `unordered_map`.

Ils suivent l'API actuelle : `canExecute()` vérifie les prérequis,
`getExecutionOrder()` calcule l'ordre, puis l'appelant utilise `executeTask()`.
Cette dernière méthode ne vérifie pas elle-même les dépendances. Une dépendance
expirée est absente de `getDependencies()` mais bloque `canExecute()`.
`executeAll()` orchestre ces opérations : il rejette les cycles, respecte l'ordre
des dépendances et n'exécute que les tâches pour lesquelles `canExecute()` vaut
`true`. Les tests vérifient les états finaux et les traces d'exécution pour
s'assurer que chaque tâche attendue s'exécute exactement une fois, après ses
prérequis.

Pour compiler uniquement l'application, configurer avec `-DBUILD_TESTING=OFF`.
