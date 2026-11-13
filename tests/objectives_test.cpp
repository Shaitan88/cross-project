#include "game.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void enterSecondRoom(Game& game) {
    game.update(4.5, 1, 0);
    game.interact();
    game.update(0.1, 1, 0);
}

void lockedExit() {
    Game game;
    game.update(4.5, -1, 0);
    const auto atExit = game.snapshot();
    require(atExit.x == 24 && atExit.y == 200, "Failed to reach locked exit");
    game.interact();
    require(!game.snapshot().won, "Locked exit granted victory");
    require(!game.snapshot().hasKey, "Locked exit granted a key");
    std::cout << "PASS: locked exit does not win without key\n";
}

void keyAndVictory() {
    Game game;
    enterSecondRoom(game);
    require(game.snapshot().room == 2 && game.snapshot().x == 24, "Did not enter room two");

    // Climb above the room-two cover (cover spans y=120..280).
    game.update(1.0, 0, -1);
    require(game.snapshot().y <= 104, "Did not clear cover vertically");

    // Slide right past the cover to the key's column.
    game.update(4.5, 1, 0);
    const auto atKeyColumn = game.snapshot();
    require(atKeyColumn.x >= 552 && atKeyColumn.x <= 568, "Key column not reached");

    // Drop back to the key's row and collect it.
    game.update(1.0, 0, 1);
    require(game.snapshot().y == 200, "Key row not reached");
    game.interact();
    require(game.snapshot().hasKey, "Key was not collected");

    // Return above the cover, then back to the room-two doorway.
    game.update(1.0, 0, -1);
    game.update(4.5, -1, 0);
    require(game.snapshot().x == 24, "Did not return to the room-two doorway");
    game.update(1.0, 0, 1);
    require(game.snapshot().doorOpen, "Return door should remain open");
    game.update(0.1, -1, 0);
    require(game.snapshot().room == 1, "Did not return to room one");

    // Cross room one to the exit at the left wall.
    game.update(5, -1, 0);
    require(game.snapshot().x == 24 && game.snapshot().y == 200, "Did not reach the exit");
    game.interact();
    require(game.snapshot().room == 3 && !game.snapshot().won,
        "First level exit did not advance to level two");

    // Level two: route above the room-three cover, cross to room four,
    // collect its key, then return through the door to the level exit.
    game.update(1.0, 0, -1);
    game.update(5, 1, 0);
    game.update(1.0, 0, 1);
    game.interact();
    game.update(0.1, 1, 0);
    game.update(1.2, 0, -1);
    game.update(4.5, 1, 0);
    game.update(0.33, 0, 1);
    game.interact();
    game.update(0.33, 0, -1);
    game.update(4.5, -1, 0);
    game.update(1.2, 0, 1);
    game.update(0.1, -1, 0);
    game.update(1.0, 0, -1);
    game.update(4.5, -1, 0);
    game.update(1.0, 0, 1);
    game.update(5, -1, 0);
    game.interact();
    require(game.snapshot().room == 5 && !game.snapshot().won,
        "Second level exit did not advance to level three");

    // Level three: the key is below the central cover.
    game.update(1.0, 0, 1);
    game.update(5, 1, 0);
    game.update(1.0, 0, -1);
    game.interact();
    game.update(0.1, 1, 0);
    game.update(0.73, 0, 1);
    game.update(4.5, 1, 0);
    game.interact();
    game.update(0.73, 0, -1);
    game.update(4.5, -1, 0);
    game.update(1.0, 0, 1);
    game.update(4.5, -1, 0);
    game.update(1.0, 0, -1);
    game.update(5, -1, 0);
    game.interact();
    const auto finished = game.snapshot();
    require(finished.won, "Final level key did not unlock victory");
    require(!finished.lost, "Victory also marked the player lost");
    std::cout << "PASS: keys unlock three sequential levels and final victory\n";
}

void defeatAndRestart() {
    Game game;
    game.update(1.0, 0, -1);
    require(game.snapshot().y == 80, "Did not reach guard row");
    bool caught = false;
    for (int i = 0; i < 1200 && !caught; ++i) {
        game.update(1.0 / 60, 1, 0);
        caught = game.snapshot().lost;
    }
    require(caught, "Living guard never caught the player");
    require(game.snapshot().alarm, "Defeat did not raise the alarm");

    game.restart();
    const auto fresh = game.snapshot();
    require(fresh.x == 64 && fresh.y == 200, "Restart did not reset position");
    require(fresh.ammo == 6, "Restart did not reset ammo");
    require(fresh.room == 1, "Restart did not reset room");
    require(!fresh.hasKey, "Restart kept the key");
    require(!fresh.won, "Restart kept victory");
    require(!fresh.lost, "Restart kept defeat");
    require(!fresh.alarm, "Restart kept alarm");
    require(fresh.flash == 0, "Restart kept shot flash");
    require(!fresh.bulletActive, "Restart kept bullet alive");
    std::cout << "PASS: guard contact loses, alarm latches, restart fully resets\n";
}

void shotEffects() {
    Game game;
    game.shoot();
    auto state = game.snapshot();
    require(state.flash > 0, "Shot did not flash");
    require(state.bulletActive, "Shot did not spawn bullet");
    require(state.ammo == 5, "Shot did not consume one round");
    require(state.alarm, "Shot did not raise alarm");
    game.update(0.05, 0, 0);
    require(game.snapshot().bulletActive, "Bullet expired too early");
    game.update(1.0, 0, 0);
    state = game.snapshot();
    require(!state.bulletActive, "Bullet never expired");
    require(state.flash == 0, "Shot flash never faded");
    std::cout << "PASS: shot flash, bullet lifetime and ammo consumption\n";
}

void keyMetadata() {
    Game game;
    const auto state = game.snapshot();
    require(std::abs(state.keyX - 552) < 1e-9, "Key x metadata incorrect");
    require(std::abs(state.keyY - 192) < 1e-9, "Key y metadata incorrect");
    require(!state.hasKey, "Key reported as collected at start");
    std::cout << "PASS: key metadata reports room-two location\n";
}
}

int main() {
    try {
        lockedExit();
        keyAndVictory();
        defeatAndRestart();
        shotEffects();
        keyMetadata();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
