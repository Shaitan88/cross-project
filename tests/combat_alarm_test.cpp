#include "game.hpp"

#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void aimAtGuard(Game& game) {
    game.update(1, 0, -1);
    game.update(1.5, 1, 0);
}

void ammunitionAndReset() {
    Game game;
    require(game.snapshot().ammo == 6 && !game.snapshot().alarm, "Initial ammo/alarm incorrect");
    for (int i = 0; i < 6; ++i) game.shoot();
    require(game.snapshot().ammo == 0 && game.snapshot().alarm, "Shots did not exhaust six rounds");
    aimAtGuard(game);
    game.shoot();
    require(game.snapshot().ammo == 0 && game.snapshot().guard.health == 2,
        "Empty weapon fired or ammunition became negative");
    Game fresh;
    require(fresh.snapshot().ammo == 6 && !fresh.snapshot().alarm && fresh.snapshot().guard.health == 2,
        "New game did not reset combat state");
    std::cout << "PASS: six shots consume six rounds; empty weapon cannot damage; new game resets combat\n";
}

void hitsAndDeath() {
    Game game;
    aimAtGuard(game);
    game.shoot();
    require(game.snapshot().guard.health == 1 && !game.snapshot().guard.dead && game.snapshot().ammo == 5,
        "First aligned shot did not wound guard");
    game.shoot();
    const auto corpse = game.snapshot().guard;
    require(corpse.health == 0 && corpse.dead && !corpse.detected, "Second hit did not kill guard");
    game.update(3, 0, 0);
    require(game.snapshot().guard.x == corpse.x && !game.snapshot().detected,
        "Dead guard moved or detected player");
    game.shoot();
    require(game.snapshot().guard.health == 0, "Corpse health became negative");
    game.update(1, 0, 1);
    game.update(5, 1, 0);
    game.interact();
    game.update(0.1, 1, 0);
    require(game.snapshot().room == 2 && game.snapshot().guard.health == 2 && game.snapshot().alarm,
        "Room change lost alarm or damaged other guard");
    game.update(0.1, -1, 0);
    require(game.snapshot().room == 1 && game.snapshot().guard.dead, "Room change resurrected guard");
    std::cout << "PASS: two-hit death; corpse stationary and cannot detect; room-local damage persists\n";
}

void missesAndCover() {
    Game misaligned;
    misaligned.shoot();
    require(misaligned.snapshot().guard.health == 2 && misaligned.snapshot().alarm,
        "Misaligned shot hit or did not raise alarm");
    Game far;
    far.update(1, 0, -1);
    far.update(0, 1, 0);
    far.shoot();
    require(far.snapshot().guard.health == 2, "Shot exceeded range");
    Game behind;
    aimAtGuard(behind);
    behind.update(0, -1, 0);
    behind.shoot();
    require(behind.snapshot().guard.health == 2, "Shot hit behind player");
    Game cover;
    cover.update(5, 1, 0);
    cover.interact();
    cover.update(0.1, 1, 0);
    cover.update(1.4, 1, 0);
    const auto state = cover.snapshot();
    require(state.guard.x - state.x < 320 && state.y == state.guard.y, "Cover setup is not in range/on aim");
    cover.shoot();
    require(cover.snapshot().guard.health == 2, "Bullet passed through cover");
    std::cout << "PASS: off-axis, out-of-range, backwards and cover-blocked shots miss\n";
}

void detectionAlarm() {
    Game game;
    game.update(1, 0, -1);
    require(!game.snapshot().alarm, "Idle patrol raised alarm without detection");
    game.update(3, 1, 0);
    require(game.snapshot().detected && game.snapshot().alarm, "Detection did not raise alarm");
    game.update(2, 0, 0);
    require(!game.snapshot().detected && game.snapshot().alarm, "Losing sight cleared latched alarm");
    std::cout << "PASS: quiet patrol, detection raises alarm, losing sight preserves alarm\n";
}
}

int main() {
    try {
        ammunitionAndReset();
        hitsAndDeath();
        missesAndCover();
        detectionAlarm();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
