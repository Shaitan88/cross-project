#pragma once

#include <cstdint>
#include <string>

struct Color {
    std::uint8_t r, g, b, a = 255;
};

class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void fillRect(int x, int y, int w, int h, Color color) = 0;
    virtual void drawLine(double x1, double y1, double x2, double y2, Color color) = 0;
    virtual void setBlend(bool enabled) = 0;
    virtual void present() = 0;
};

struct InputFrame {
    int horizontal = 0;
    int vertical = 0;
    bool interactPressed = false;
    bool shootPressed = false;
    bool restartPressed = false;
    bool pausePressed = false;
    bool startPressed = false;
    bool quitRequested = false;
    bool focusLost = false;
};

class Platform {
public:
    virtual ~Platform() = default;
    virtual Renderer& renderer() = 0;
    virtual InputFrame pollInput() = 0;
    virtual double nowSeconds() = 0;
    virtual void setTitle(const std::string& title) = 0;
    virtual void playTone(int frequencyHz, int durationMs) = 0;
    virtual void delay(int milliseconds) = 0;
};
