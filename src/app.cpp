#include "app.hpp"

#include "platform.hpp"
#include "render.hpp"

#include <algorithm>
#include <string>

namespace {
std::string buildTitle(const GameState& state, bool paused) {
    std::string objective;
    if (state.won) objective = "VICTORY | R: restart";
    else if (state.lost) objective = "CAPTURED | R: restart";
    else if (state.hasKey) objective = "Key: yes | Exit unlocked";
    else objective = "Key: no | Find the key";

    return "Castle Escape | Room " + std::to_string(state.room)
        + " | Ammo " + std::to_string(state.ammo) + " | " + (state.alarm ? "ALARM" : "Quiet")
        + " | " + (paused ? "PAUSED" : objective)
        + " | WASD / Arrows: move | E: door | Space: shoot | P: pause | R: restart | Esc: quit";
}
} // namespace

Application::Application(Game& game, Platform& platform, AppOptions options)
    : game_(game), platform_(platform), options_(options), menu_(options.showMenu) {}

AppResult Application::run() {
    while (step()) {
    }
    result_.final = game_.snapshot();
    return result_;
}

bool Application::step() {
    if (!running_) return false;

    if (!initialized_) {
        result_.initial = game_.snapshot();
        previous_ = platform_.nowSeconds();
        initialized_ = true;
    }

    const InputFrame input = platform_.pollInput();
    if (input.quitRequested) {
        running_ = false;
        return false;
    }

    if (input.restartPressed) {
        game_.restart();
        accumulator_ = 0;
        previous_ = platform_.nowSeconds();
    }
    if (menu_) {
        const GameState state = game_.snapshot();
        if (input.startPressed) menu_ = false;
        result_.title = menu_ ? "Castle Escape | MENU | Enter/Space: start | Esc: quit"
                              : buildTitle(state, false);
        platform_.setTitle(result_.title);
        renderFrame(platform_.renderer(), state);
        if (menu_) drawMenuOverlay(platform_.renderer(), state);
        platform_.renderer().present();
        ++result_.frames;
        if (options_.stopAfterFrames > 0 && result_.frames >= options_.stopAfterFrames) running_ = false;
        return running_;
    }
    if (input.pausePressed) paused_ = !paused_;
    if (paused_) {
        const GameState state = game_.snapshot();
        result_.title = buildTitle(state, true);
        platform_.setTitle(result_.title);
        renderFrame(platform_.renderer(), state);
        drawPauseOverlay(platform_.renderer(), state);
        platform_.renderer().present();
        ++result_.frames;
        if (options_.stopAfterFrames > 0 && result_.frames >= options_.stopAfterFrames) running_ = false;
        return running_;
    }
    if (input.interactPressed) game_.interact();
    if (input.shootPressed) game_.shoot();

    if (options_.deterministic) {
        accumulator_ += options_.fixedStep;
        previous_ += options_.fixedStep;
    } else {
        const double now = platform_.nowSeconds();
        accumulator_ += std::min(std::max(0.0, now - previous_), options_.maxFrameSeconds);
        previous_ = now;
    }

    while (accumulator_ >= options_.fixedStep) {
        game_.update(options_.fixedStep, input.horizontal, input.vertical);
        accumulator_ -= options_.fixedStep;
    }

    const GameState state = game_.snapshot();
    result_.title = buildTitle(state, false);
    platform_.setTitle(result_.title);
    renderFrame(platform_.renderer(), state);
    platform_.renderer().present();
    ++result_.frames;

    if (options_.stopAfterFrames > 0 && result_.frames >= options_.stopAfterFrames) {
        running_ = false;
    }
    if (!options_.deterministic) platform_.delay(1);

    return running_;
}
