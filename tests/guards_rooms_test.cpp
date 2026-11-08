#include "game.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void enterSecondRoom(Game& game) {
    game.update(5, 1, 0);
    game.interact();
    game.update(0.1, 1, 0);
}

void doorsAndRooms() {
    Game game;
    game.interact();
    require(!game.snapshot().doorOpen, "Remote interaction opened door");
    game.update(5, 1, 0);
    auto state = game.snapshot();
    require(state.room == 1 && state.x == 600 && !state.doorOpen, "Closed door did not block exit");
    game.interact();
    require(game.snapshot().doorOpen, "Nearby interaction did not open door");
    game.update(0, 1, 0);
    require(game.snapshot().room == 1, "Zero time caused room transition");
    game.update(0.1, 1, 0);
    state = game.snapshot();
    require(state.room == 2 && state.x == 24 && state.y == 200, "Open door did not enter room two");
    game.interact();
    game.update(0.1, -1, 0);
    require(game.snapshot().room == 2 && !game.snapshot().doorOpen, "Closed return door did not block exit");
    game.interact();
    game.update(0.1, -1, 0);
    require(game.snapshot().room == 1 && game.snapshot().x == 600, "Return transition failed");
    game.update(1, 0, -1);
    game.update(1, 1, 0);
    require(game.snapshot().room == 1, "Exited outside doorway");
    std::cout << "PASS: closed/open door, interaction proximity, bidirectional room transition, doorway bounds\n";
}

void patrolAndDetection() {
    Game game;
    const auto initial = game.snapshot().guard;
    game.update(1, 0, 0);
    require(game.snapshot().guard.x == initial.x + 40, "Guard patrol did not advance");
    game.update(2, 0, 0);
    require(game.snapshot().guard.x == 480 && game.snapshot().guard.direction == -1,
        "Guard did not reverse at patrol endpoint");
    Game detection;
    detection.update(1, 0, -1);
    detection.update(3, 1, 0);
    require(detection.snapshot().detected && detection.snapshot().guard.detected,
        "Guard failed to detect visible player ahead");
    detection.update(2, 0, 0);
    require(!detection.snapshot().detected, "Guard detected player behind its facing direction");
    detection.update(100, -1, 0);
    require(!detection.snapshot().detected, "Guard detected player beyond sight range");
    Game stepped;
    Game single;
    for (int i = 0; i < 600; ++i) stepped.update(1.0 / 60, 0, 0);
    single.update(10, 0, 0);
    require(std::abs(stepped.snapshot().guard.x - single.snapshot().guard.x) < 1e-8,
        "Patrol depends on update step size");
    std::cout << "PASS: patrol advance/reversal, step-size independence, detection, facing and range\n";
}

void coverOcclusion() {
    Game game;
    enterSecondRoom(game);
    game.update(4.0 / 3, 1, 0);
    auto state = game.snapshot();
    require(state.guard.direction == -1 && state.guard.x - state.x < 400,
        "Occlusion setup must face player within range");
    require(!state.detected, "Guard saw through cover");
    game.update(2, 1, 0);
    require(game.snapshot().x == 284, "Player tunneled through cover");
    game.update(1, 0, -1);
    game.update(76.0 / 120, 1, 0);
    game.update(1, 0, 1);
    for (int i = 0; i < 60 && !game.snapshot().detected; ++i) game.update(0.1, 0, 0);
    require(game.snapshot().detected, "Guard failed to detect player after leaving cover");
    std::cout << "PASS: cover occludes sight, blocks movement, exposed player detected\n";
}
}

int main() {
    try {
        doorsAndRooms();
        patrolAndDetection();
        coverOcclusion();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
