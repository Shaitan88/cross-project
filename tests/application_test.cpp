#include "app.hpp"
#include "platform.hpp"
#include "render.hpp"

#include <deque>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct FakeRenderer final : Renderer {
    int fillCount = 0;
    int lineCount = 0;
    int blendCount = 0;
    int presentCount = 0;

    void fillRect(int, int, int, int, Color) override { ++fillCount; }
    void drawLine(double, double, double, double, Color) override { ++lineCount; }
    void setBlend(bool) override { ++blendCount; }
    void present() override { ++presentCount; }
};

class FakePlatform final : public Platform {
public:
    FakePlatform(std::deque<InputFrame> frames, FakeRenderer& renderer)
        : frames_(std::move(frames)), renderer_(renderer) {}

    Renderer& renderer() override { return renderer_; }

    InputFrame pollInput() override {
        if (frames_.empty()) {
            InputFrame quit;
            quit.quitRequested = true;
            return quit;
        }
        InputFrame frame = frames_.front();
        frames_.pop_front();
        return frame;
    }

    double nowSeconds() override {
        now_ += 0.001;
        return now_;
    }

    void setTitle(const std::string& title) override { title_ = title; }
    void delay(int) override { ++delayCount; }

    const std::string& title() const { return title_; }
    int delayCount = 0;

private:
    std::deque<InputFrame> frames_;
    FakeRenderer& renderer_;
    double now_ = 0;
    std::string title_;
};

void actionFramesRestartAndQuit() {
    FakeRenderer renderer;
    std::deque<InputFrame> frames;
    InputFrame shoot;
    shoot.horizontal = 1;
    shoot.shootPressed = true;
    frames.push_back(shoot);
    InputFrame interact;
    interact.interactPressed = true;
    frames.push_back(interact);
    InputFrame restart;
    restart.restartPressed = true;
    frames.push_back(restart);
    InputFrame quit;
    quit.quitRequested = true;
    frames.push_back(quit);

    FakePlatform platform(std::move(frames), renderer);
    Game game;
    AppOptions options;
    options.deterministic = true;
    Application application(game, platform, options);
    const AppResult result = application.run();

    require(result.frames == 3, "Application did not run exactly three frames");
    require(result.final.ammo == 6, "Restart did not refill ammo");
    require(!result.final.alarm, "Restart did not clear the alarm");
    require(result.final.room == 1, "Restart did not return to room one");
    require(renderer.presentCount == 3, "Each frame was not presented exactly once");
    require(platform.title().find("Ammo 6 | Quiet") != std::string::npos,
        "Restarted title lacks ammo and quiet state");
    std::cout << "PASS: input edges dispatch actions, restart resets state and loop frames render\n";
}

void fixedStepMovementRendersEveryFrame() {
    constexpr int frameCount = 6;
    FakeRenderer renderer;
    std::deque<InputFrame> frames;
    for (int i = 0; i < frameCount; ++i) {
        InputFrame move;
        move.horizontal = 1;
        frames.push_back(move);
    }
    InputFrame quit;
    quit.quitRequested = true;
    frames.push_back(quit);

    FakePlatform platform(std::move(frames), renderer);
    Game game;
    AppOptions options;
    options.deterministic = true;
    Application application(game, platform, options);
    const AppResult result = application.run();

    require(result.frames == frameCount, "Deterministic loop did not render every frame");
    require(renderer.presentCount == frameCount, "Present count does not match frame count");
    require(result.final.x > result.initial.x, "Player did not advance under fixed-step movement");
    require(result.final.x == result.initial.x + frameCount * 2.0,
        "Fixed-step movement advanced by an unexpected distance");
    std::cout << "PASS: deterministic fixed-step loop advances the player and renders each frame\n";
}

void cyrillicOverlayCoverage() {
    require(pixelTextWidth(U"ПОБЕДА", 7) == 245, "Victory text width is wrong");
    require(pixelTextWidth(U"ПОРАЖЕНИЕ", 6) == 318, "Defeat text width is wrong");

    FakeRenderer renderer;
    GameState won{};
    won.width = 640;
    won.height = 400;
    won.won = true;
    drawResultOverlay(renderer, won);
    const int victoryFills = renderer.fillCount;
    require(victoryFills > 100, "Victory overlay did not draw a substantial panel and glyphs");

    GameState lost{};
    lost.width = 640;
    lost.height = 400;
    lost.lost = true;
    drawResultOverlay(renderer, lost);
    require(renderer.fillCount - victoryFills > 100,
        "Defeat overlay did not draw a substantial panel and glyphs");
    std::cout << "PASS: Russian victory and defeat text render from internal glyphs\n";
}
} // namespace

int main() {
    try {
        actionFramesRestartAndQuit();
        fixedStepMovementRendersEveryFrame();
        cyrillicOverlayCoverage();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
