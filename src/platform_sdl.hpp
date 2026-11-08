#pragma once

#include "platform.hpp"

#include <string>

struct SDL_Window;
struct SDL_Renderer;

class SdlPlatform final : public Platform, public Renderer {
public:
    explicit SdlPlatform(bool smoke);
    ~SdlPlatform() override;

    Renderer& renderer() override;
    InputFrame pollInput() override;
    double nowSeconds() override;
    void setTitle(const std::string& title) override;
    void delay(int milliseconds) override;

    void fillRect(int x, int y, int w, int h, Color color) override;
    void drawLine(double x1, double y1, double x2, double y2, Color color) override;
    void setBlend(bool enabled) override;
    void present() override;

private:
    bool smoke_;
    bool firstPoll_ = true;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
};
