#include "app.hpp"
#include "platform_sdl.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    const bool smoke = argc == 2 && std::string(argv[1]) == "--smoke-test";
    if (argc > 1 && !smoke) {
        std::cerr << "Usage: castle_escape [--smoke-test]\n";
        return 2;
    }

    try {
        SdlPlatform platform(smoke);
        Game game;
        AppOptions options;
        options.deterministic = smoke;
        options.showMenu = !smoke;
        options.stopAfterFrames = smoke ? 120 : 0;
        Application application(game, platform, options);
        const AppResult result = application.run();

        if (smoke) {
            if (result.frames != 120 || result.final.x <= result.initial.x)
                throw std::runtime_error("Smoke test did not advance the game");
            if (result.final.ammo != 5 || result.final.alarm)
                throw std::runtime_error("Space/repeat handling failed");
            if (result.title.find("Ammo 5 | Quiet") == std::string::npos)
                throw std::runtime_error("Combat state missing from window title");
            std::cout << "PASS: SDL window, 120 fixed updates and rendered frames; player x="
                << result.final.x
                << "; Space fires once, repeat ignored, Ammo 5 and Quiet in title\n";
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Castle Escape: " << error.what() << '\n';
        return 1;
    }
}
