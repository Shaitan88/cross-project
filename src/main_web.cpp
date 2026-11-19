#include "app.hpp"
#include "game.hpp"
#include "platform_sdl.hpp"

#include <emscripten.h>

extern "C" EMSCRIPTEN_KEEPALIVE void castle_focus_lost();

namespace {
struct WebAppState;
WebAppState* activeState = nullptr;
// Member order matters: the platform and application hold references into the
// game, so it must be constructed first. The whole state lives for the
// lifetime of the page.
struct WebAppState {
    Game game;
    SdlPlatform platform;
    Application application;

    WebAppState() : game(), platform(false), application(game, platform, menuOptions()) {}

    static AppOptions menuOptions() {
        AppOptions options;
        options.showMenu = true;
        return options;
    }
};

void frame(void* arg) {
    auto* state = static_cast<WebAppState*>(arg);
    if (state->application.step()) return;

    emscripten_cancel_main_loop();
    EM_ASM({
        var el = document.getElementById('status');
        if (el) {
            el.textContent = "Игра остановлена. Обновите страницу, чтобы начать заново.";
        }
    });
}
} // namespace

extern "C" EMSCRIPTEN_KEEPALIVE void castle_focus_lost() {
    if (activeState) activeState->application.pauseForFocusLoss();
}

int main() {
    static WebAppState state;
    activeState = &state;
    emscripten_set_main_loop_arg(frame, &state, 0, true);
    return 0;
}
