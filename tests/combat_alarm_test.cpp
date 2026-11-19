#include "game.hpp"
#include <iostream>

namespace {
void ticks(Game& game, int count, int x = 0, int y = 0) {
    for (int i = 0; i < count; ++i) game.update(1.0 / 60, x, y);
}
void guardRoom(Game& game) {
    for (int i = 0; i < 2; ++i) {
        game.update(5, 1, 0);
        game.interact();
        ticks(game, 1, 1);
    }
    ticks(game, 45, 1);
    ticks(game, 46, 0, -1);
    for (int i = 0; i < 360 && game.snapshot().guards.front()[3] == 0; ++i) ticks(game, 1);
}
}

int main() {
    Game game;
    for (int i = 0; i < 6; ++i) if (!game.shoot()) return 1;
    if (game.shoot() || game.snapshot().ammo != 0 || game.snapshot().alarm) return 2;
    game.restart();
    guardRoom(game);
    if (game.snapshot().guards.front()[3] != 3) return 3;
    ticks(game, 179);
    if (game.snapshot().alarm || game.snapshot().guards.front()[3] != 1) return 4;
    ticks(game, 1);
    if (game.snapshot().alarmSeconds != 10 || game.snapshot().guards.front()[3] != 0) return 5;
    ticks(game, 599);
    if (game.snapshot().lost || game.snapshot().alarmSeconds != 1) return 6;
    ticks(game, 1);
    if (!game.snapshot().lost) return 7;
    game.restart();
    guardRoom(game);
    game.aim(1, 0);
    if (!game.shoot() || !game.shoot() || game.snapshot().guards.front()[2] != 2) return 8;
    ticks(game, 30);
    if (game.snapshot().guards.front()[2] != 0 || game.snapshot().alarm) return 9;
    ticks(game, 180);
    if (game.snapshot().alarm || game.snapshot().lost) return 10;
    std::cout << "PASS: timed projectile impacts, warning cancellation and exact alarm deadline\n";
}
