#include "app.hpp"
#include "game.hpp"
#include "platform_sdl.hpp"

#include <emscripten.h>

namespace {
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

int main() {
    static WebAppState state;
    emscripten_set_main_loop_arg(frame, &state, 0, true);
    return 0;
}
