#include "game.hpp"
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main() {
    try {
        Game game;
        const auto initial = game.snapshot();
        game.update(1, 0, 0);
        require(game.snapshot().x == initial.x && game.snapshot().y == initial.y, "Idle moved player");
        for (int i = 0; i < 60; ++i) game.update(1.0 / 60, 1, 0);
        require(std::abs(game.snapshot().x - initial.x - 120) < 1e-8, "Fixed-step speed incorrect");
        Game diagonal;
        diagonal.update(0.5, 1, 1);
        const auto moved = diagonal.snapshot();
        require(std::abs(std::hypot(moved.x - initial.x, moved.y - initial.y) - 60) < 1e-8,
            "Diagonal movement must be normalized");
        game.update(100, -1, -1);
        require(game.snapshot().x == initial.wall && game.snapshot().y == initial.wall, "Top-left collision failed");
        game.update(100, 1, 1);
        auto state = game.snapshot();
        require(state.x == state.width - state.wall - state.size && state.y == state.height - state.wall - state.size,
            "Bottom-right collision failed");
        std::mt19937 random(0xCA57);
        std::uniform_int_distribution<int> direction(-1, 1);
        std::uniform_real_distribution<double> duration(0, 2);
        for (int i = 0; i < 10000; ++i) {
            game.update(duration(random), direction(random), direction(random));
            state = game.snapshot();
            require(std::isfinite(state.x) && std::isfinite(state.y) &&
                state.x >= state.wall && state.y >= state.wall &&
                state.x + state.size <= state.width - state.wall &&
                state.y + state.size <= state.height - state.wall, "Movement property violated room bounds");
        }
        bool rejected = false;
        try { game.update(-1, 0, 0); } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "Negative time was accepted");
        std::cout << "PASS: idle, fixed-step speed, diagonal speed, four walls, invalid input, 10000 seeded movement properties\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
