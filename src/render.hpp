#pragma once

#include "game.hpp"
#include "platform.hpp"

#include <string_view>

void renderFrame(Renderer& renderer, const GameState& state);
void drawPauseOverlay(Renderer& renderer, const GameState& state);
void drawResultOverlay(Renderer& renderer, const GameState& state);
int pixelTextWidth(std::u32string_view text, int scale);
void drawPixelText(Renderer& renderer, std::u32string_view text, int left, int top,
    int scale, Color color);
