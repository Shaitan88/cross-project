#include "game.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void walk(Game& game, int x, int y) {
    for (int step = 0; step < 400; ++step) {
        const auto state = game.snapshot();
        if (state.lost) throw std::runtime_error("Caught walking in mission " +
            std::to_string(state.level) + ", room " + std::to_string(state.room));
        const int dx = state.x < x - 1 ? 1 : state.x > x + 1 ? -1 : 0;
        const int dy = state.y < y - 1 ? 1 : state.y > y + 1 ? -1 : 0;
        if (dx == 0 && dy == 0) return;
        game.update(1.0 / 60, dx, dx == 0 ? dy : 0);
    }
    const auto state = game.snapshot();
    throw std::runtime_error("Waypoint " + std::to_string(x) + "," + std::to_string(y)
        + " unreachable in mission " + std::to_string(state.level) + ", room " + std::to_string(state.room));
}

void cross(Game& game, char side, int room) {
    const int x = side == 'L' ? 24 : side == 'R' ? 600 : 312;
    const int y = side == 'U' ? 24 : side == 'D' ? 360 : 192;
    for (int step = 0; step < 800 && game.snapshot().room != room; ++step) {
        const auto s = game.snapshot();
        if (s.lost) throw std::runtime_error("Caught in mission " + std::to_string(s.level)
            + ", room " + std::to_string(s.room) + " before door " + side);
        const bool horizontal = side == 'L' || side == 'R';
        const int dx = s.x < x - 1 ? 1 : s.x > x + 1 ? -1 : 0;
        const int dy = s.y < y - 1 ? 1 : s.y > y + 1 ? -1 : 0;
        if (!dx && !dy) {
            game.interact();
            game.update(1.0 / 60, side == 'L' ? -1 : side == 'R' ? 1 : 0,
                side == 'U' ? -1 : side == 'D' ? 1 : 0);
        } else if (horizontal) {
            game.update(1.0 / 60, dy == 0 ? dx : 0, dy);
        } else {
            game.update(1.0 / 60, dx, dx == 0 ? dy : 0);
        }
    }
    require(game.snapshot().room == room, "Door did not lead to expected room");
    require(game.snapshot().objective.empty() == (room != 1), "Mission hint leaked outside first room");
}

void finish(Game& game) {
    walk(game, game.snapshot().level == 3 ? 190 : 530, 192);
    require(game.interact(), "Exit interaction failed");
    game.update(1.0 / 60, 0, 0);
}
void powerBranch(Game& game, bool left) {
    const int first = left ? 2 : 4;
    const int last = left ? 3 : 5;
    cross(game, left ? 'L' : 'R', first);
    // Go above the cover before aligning with the far wall.
    walk(game, left ? 598 : 26, 40);
    walk(game, left ? 28 : 596, 40);
    cross(game, left ? 'L' : 'R', last);
    walk(game, left ? 598 : 26, left ? 310 : 42);
    walk(game, 552, left ? 310 : 42);
    require(game.interact(), "Switch interaction failed");
    walk(game, 552, left ? 310 : 42);
    walk(game, left ? 598 : 26, left ? 310 : 42);
    cross(game, left ? 'R' : 'L', first);
    walk(game, left ? 26 : 598, 40);
    walk(game, left ? 596 : 28, 40);
    cross(game, left ? 'R' : 'L', 1);
}

void campaign(Game& game, bool alternative) {
    require(game.snapshot().level == 1 && !game.snapshot().objective.empty(), "Campaign did not start correctly");
    cross(game, 'R', 2);
    if (alternative) {
        cross(game, 'D', 4);
        walk(game, 220, 26);
        walk(game, 220, 320);
        walk(game, 590, 320);
        cross(game, 'R', 3);
    } else {
        cross(game, 'R', 3);
    }
    walk(game, 552, 192);
    require(game.interact() && game.snapshot().hasKey, "Key was not collected");
    cross(game, 'R', 5);
    require(game.snapshot().ammo == 6, "Escape used ammunition");
    finish(game);
    require(game.snapshot().level == 2, "Sabotage did not unlock");
    require(game.snapshot().doors.back()[4] == 2, "Powered exit was already open");
    powerBranch(game, alternative);
    require(game.snapshot().doors.back()[4] == 2, "One switch unlocked the powered exit");
    powerBranch(game, !alternative);
    cross(game, 'U', 6);
    finish(game);
    require(game.snapshot().level == 3, "Breakout did not unlock");
    if (alternative) {
        cross(game, 'D', 3);
        walk(game, 410, 26);
        cross(game, 'R', 2);
    } else {
        cross(game, 'R', 2);
    }
    // The control panels are below the patrol lane.
    walk(game, static_cast<int>(game.snapshot().x), 294);
    walk(game, 552, 294);
    require(game.interact(), "First gate panel failed");
    if (alternative) {
        walk(game, 312, 294);
        cross(game, 'U', 5);
        walk(game, 26, 330);
        walk(game, 598, 330);
        cross(game, 'R', 4);
    } else {
        walk(game, 590, 294);
        cross(game, 'R', 4);
    }
    walk(game, alternative ? 312 : 26, 294);
    walk(game, 552, 294);
    require(game.interact(), "Second gate panel failed");
    if (alternative) {
        walk(game, 570, 294);
        walk(game, 570, 30);
        cross(game, 'U', 5);
        walk(game, 598, 330);
        cross(game, 'D', 6);
    } else {
        walk(game, 590, 294);
        cross(game, 'R', 6);
    }
    finish(game);
    require(game.snapshot().won, "Campaign did not finish");
}
}

int main() {
    try {
        Game game;
        campaign(game, false);
        game.restart();
        campaign(game, true);
        std::cout << "PASS: two 60 Hz campaign routes and both switch orders\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
