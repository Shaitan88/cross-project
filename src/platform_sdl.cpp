#define SDL_MAIN_HANDLED
#include "platform_sdl.hpp"

#include <SDL.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace {
void check(int result) {
    if (result < 0) throw std::runtime_error(SDL_GetError());
}
} // namespace

SdlPlatform::SdlPlatform(bool smoke) : smoke_(smoke) {
    SDL_SetMainReady();
#if defined(__EMSCRIPTEN__)
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
#endif
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        throw std::runtime_error(SDL_GetError());
    }
    try {
        window_ = SDL_CreateWindow("Castle Escape", SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED, 960, 600, SDL_WINDOW_RESIZABLE);
        if (!window_) throw std::runtime_error(SDL_GetError());
        renderer_ = SDL_CreateRenderer(window_, -1,
            smoke_ ? SDL_RENDERER_SOFTWARE : SDL_RENDERER_ACCELERATED);
        if (!renderer_) renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
        if (!renderer_) throw std::runtime_error(SDL_GetError());
        check(SDL_RenderSetLogicalSize(renderer_, 640, 400));
    } catch (...) {
        if (renderer_) SDL_DestroyRenderer(renderer_);
        if (window_) SDL_DestroyWindow(window_);
        SDL_Quit();
        throw;
    }
}

SdlPlatform::~SdlPlatform() {
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
}

Renderer& SdlPlatform::renderer() {
    return *this;
}

InputFrame SdlPlatform::pollInput() {
    if (smoke_ && firstPoll_) {
        firstPoll_ = false;
        SDL_Event shot{};
        shot.type = SDL_KEYDOWN;
        shot.key.keysym.sym = SDLK_SPACE;
        if (SDL_PushEvent(&shot) != 1) throw std::runtime_error("Could not enqueue smoke shot");
        shot.key.repeat = 1;
        if (SDL_PushEvent(&shot) != 1) throw std::runtime_error("Could not enqueue repeat key");
    }

    InputFrame input;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            input.quitRequested = true;
        } else if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_ESCAPE) input.quitRequested = true;
            if (!event.key.repeat) {
                if (event.key.keysym.sym == SDLK_e) input.interactPressed = true;
                if (event.key.keysym.sym == SDLK_SPACE) input.shootPressed = true;
                if (event.key.keysym.sym == SDLK_r) input.restartPressed = true;
                if (event.key.keysym.sym == SDLK_p) input.pausePressed = true;
            }
        }
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    input.horizontal = (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT])
        - (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]);
    input.vertical = (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])
        - (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]);
    if (smoke_) {
        input.horizontal = 1;
        input.vertical = 0;
    }
    return input;
}

double SdlPlatform::nowSeconds() {
    const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());
    return static_cast<double>(SDL_GetPerformanceCounter()) / frequency;
}

void SdlPlatform::setTitle(const std::string& title) {
#if defined(__EMSCRIPTEN__)
    EM_ASM({
        var el = document.getElementById('hud');
        if (el) el.textContent = UTF8ToString($0);
    }, title.c_str());
#else
    SDL_SetWindowTitle(window_, title.c_str());
#endif
}

void SdlPlatform::delay(int milliseconds) {
#if defined(__EMSCRIPTEN__)
    (void)milliseconds;
#else
    if (milliseconds > 0) SDL_Delay(static_cast<Uint32>(milliseconds));
#endif
}

void SdlPlatform::fillRect(int x, int y, int w, int h, Color color) {
    check(SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a));
    SDL_Rect rect{x, y, w, h};
    check(SDL_RenderFillRect(renderer_, &rect));
}

void SdlPlatform::drawLine(double x1, double y1, double x2, double y2, Color color) {
    check(SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a));
    check(SDL_RenderDrawLine(renderer_, static_cast<int>(std::lround(x1)),
        static_cast<int>(std::lround(y1)), static_cast<int>(std::lround(x2)),
        static_cast<int>(std::lround(y2))));
}

void SdlPlatform::setBlend(bool enabled) {
    check(SDL_SetRenderDrawBlendMode(renderer_,
        enabled ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE));
}

void SdlPlatform::present() {
    SDL_RenderPresent(renderer_);
}
