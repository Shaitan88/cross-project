#pragma once

#include "game.hpp"

#include <string>

class Platform;

struct AppOptions {
    double fixedStep = 1.0 / 60.0;
    double maxFrameSeconds = 0.25;
    bool deterministic = false;
    bool showMenu = false;
    int stopAfterFrames = 0;
};

struct AppResult {
    int frames = 0;
    GameState initial{};
    GameState final{};
    std::string title;
};

class Application {
public:
    Application(Game& game, Platform& platform, AppOptions options = {});
    bool step();
    AppResult run();
    void pauseForFocusLoss();

private:
    Game& game_;
    Platform& platform_;
    AppOptions options_;
    bool initialized_ = false;
    bool running_ = true;
    bool paused_ = false;
    bool menu_ = false;
    double accumulator_ = 0;
    double previous_ = 0;
    AppResult result_{};
};
