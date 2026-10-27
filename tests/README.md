# Tests du scheduler

La suite utilise uniquement la bibliothèque standard C++20 et CTest, sans
dépendance externe.

Depuis la racine du projet :

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Pour afficher le résultat de chacun des 34 cas :

```sh
./build/scheduler_tests
```

Les vérifications restent actives en Release et le programme renvoie un code
d'échec dès qu'au moins un cas échoue. CTest exécute les 34 cas comme une suite.

La couverture comprend :

- création, recherche, suppression et identifiants des tâches ;
- états, exécution et affichage ;
- dépendances nulles, multiples, dupliquées ou expirées et conditions d'exécution ;
- durée de vie des tâches avec `shared_ptr` et `weak_ptr` ;
- détection de cycles directs, indirects et dans une composante isolée ;
- ordre topologique sur des chaînes, diamants, composantes indépendantes et un
  graphe de 128 tâches ;
- rejet d'un ordre incomplet en présence d'un cycle ;
- exécution complète des tâches dans l'ordre de leurs dépendances.

Les tests ne supposent aucun ordre entre tâches indépendantes, puisque le
scheduler les stocke dans une `unordered_map`.

Ils suivent l'API actuelle : `canExecute()` vérifie les prérequis,
`getExecutionOrder()` calcule l'ordre, puis l'appelant utilise `executeTask()`.
Cette dernière méthode ne vérifie pas elle-même les dépendances. Une dépendance
expirée est absente de `getDependencies()` mais bloque `canExecute()`.

Pour compiler uniquement l'application, configurer avec `-DBUILD_TESTING=OFF`.
