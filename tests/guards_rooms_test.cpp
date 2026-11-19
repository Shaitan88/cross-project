#include "game.hpp"
#include <iostream>

int main() {
    Game game;
    game.update(5, 1, 0);
    if (game.snapshot().room != 1 || game.snapshot().doors.front()[4] != 0) return 1;
    if (!game.interact()) return 2;
    game.update(1.0 / 60, 1, 0);
    if (game.snapshot().room != 2 || !game.snapshot().objective.empty()) return 3;
    game.update(5, 1, 0);
    if (!game.interact()) return 4;
    game.update(1.0 / 60, 1, 0);
    if (game.snapshot().room != 3 || game.snapshot().guards.size() != 1) return 5;
    if (game.snapshot().doors.back()[4] != 2) return 6;
    game.restart();
    if (game.snapshot().room != 1 || game.snapshot().doors.front()[4] != 0) return 7;
    std::cout << "PASS: shared door state, locked route and restart\n";
}
