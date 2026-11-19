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

    return std::string("Castle Escape")
        + " | Ammo " + std::to_string(state.ammo) + " | " + (state.alarm ? "ALARM" : "Quiet")
        + (paused ? " | PAUSED" : objective.empty() ? "" : " | " + objective);
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

void Application::pauseForFocusLoss() {
    focusPaused_ = true;
    paused_ = true;
    accumulator_ = 0;
    previous_ = platform_.nowSeconds();
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

    if (input.focusLost) {
        pauseForFocusLoss();
    }

    if (input.restartPressed && !input.focusLost) {
        game_.restart();
        accumulator_ = 0;
        previous_ = platform_.nowSeconds();
    }
    if (menu_) {
        const GameState state = game_.snapshot();
        if (input.startPressed && !input.focusLost) {
            menu_ = false;
            accumulator_ = 0;
            previous_ = platform_.nowSeconds();
            platform_.playTone(660, 100);
        }
        result_.title = menu_ ? "Castle Escape | MENU | Enter/Space: start | Esc: quit"
                              : buildTitle(state, false);
        platform_.setTitle(result_.title);
        renderFrame(platform_.renderer(), state);
        if (menu_) drawMenuOverlay(platform_.renderer(), state);
        platform_.renderer().present();
        ++result_.frames;
        if (options_.stopAfterFrames > 0 && result_.frames >= options_.stopAfterFrames) running_ = false;
        if (!options_.deterministic) platform_.delay(16);
        return running_;
    }
    if (input.pausePressed && !input.focusLost) {
        paused_ = !paused_;
        focusPaused_ = false;
        accumulator_ = 0;
        previous_ = platform_.nowSeconds();
        platform_.playTone(paused_ ? 330 : 660, 100);
    }
    if (paused_ && focusPaused_ && (input.startPressed || input.pausePressed) && !input.focusLost) {
        paused_ = false;
        focusPaused_ = false;
        accumulator_ = 0;
        previous_ = platform_.nowSeconds();
        platform_.playTone(660, 100);
    }
    if (paused_) {
        const GameState state = game_.snapshot();
        result_.title = buildTitle(state, true);
        platform_.setTitle(result_.title);
        renderFrame(platform_.renderer(), state);
        drawPauseOverlay(platform_.renderer(), state);
        platform_.renderer().present();
        ++result_.frames;
        if (options_.stopAfterFrames > 0 && result_.frames >= options_.stopAfterFrames) running_ = false;
        if (!options_.deterministic) platform_.delay(16);
        return running_;
    }
    game_.aim(input.horizontal, input.vertical);
    if (input.interactPressed && game_.interact()) {
        platform_.playTone(520, 70);
    }
    if (input.shootPressed && game_.shoot()) {
        platform_.playTone(180, 90);
    }

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
