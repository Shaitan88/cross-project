#include "render.hpp"
#include "press_start_2p_8px.hpp"

#include <array>
#include <cmath>
#include <string_view>

namespace {
const std::array<std::uint8_t, 8>* glyphFor(char32_t character) {
    for (std::size_t index = 0; index < press_start_2p::codepoints.size(); ++index) {
        if (press_start_2p::codepoints[index] == character) {
            return &press_start_2p::glyph_rows[index];
        }
    }
    return nullptr;
}

// World key: ring, shaft and two teeth inside the 16x16 footprint reported by
// the Lua key rectangle (keyX/keyY are its top-left corner).
void drawWorldKey(Renderer& renderer, double keyX, double keyY) {
    const int left = static_cast<int>(std::lround(keyX));
    const int top = static_cast<int>(std::lround(keyY));
    renderer.fillRect(left, top + 4, 6, 2, {236, 197, 88});
    renderer.fillRect(left, top + 9, 6, 2, {236, 197, 88});
    renderer.fillRect(left, top + 4, 2, 7, {236, 197, 88});
    renderer.fillRect(left + 4, top + 4, 2, 7, {236, 197, 88});
    renderer.fillRect(left + 6, top + 7, 9, 2, {236, 197, 88});
    renderer.fillRect(left + 10, top + 9, 2, 4, {255, 228, 132});
    renderer.fillRect(left + 14, top + 9, 2, 3, {255, 228, 132});
}

// Inventory marker next to the ammo pips.
void drawHudKey(Renderer& renderer, int left, int top) {
    renderer.fillRect(left, top + 2, 6, 2, {236, 197, 88});
    renderer.fillRect(left, top + 6, 6, 2, {236, 197, 88});
    renderer.fillRect(left, top + 2, 2, 6, {236, 197, 88});
    renderer.fillRect(left + 4, top + 2, 2, 6, {236, 197, 88});
    renderer.fillRect(left + 6, top + 4, 7, 2, {236, 197, 88});
    renderer.fillRect(left + 9, top + 6, 2, 3, {255, 228, 132});
    renderer.fillRect(left + 12, top + 6, 2, 2, {255, 228, 132});
}

} // namespace

int pixelTextWidth(std::u32string_view text, int scale) {
    if (text.empty()) return 0;
    return (static_cast<int>(text.size()) * 8) * scale;
}

void drawPixelText(Renderer& renderer, std::u32string_view text, int left, int top,
    int scale, Color color) {
    if (scale <= 0) return;
    for (std::size_t index = 0; index < text.size(); ++index) {
        const auto* glyph = glyphFor(text[index]);
        if (!glyph) continue;
        const int glyphLeft = left + static_cast<int>(index) * 8 * scale;
        for (int row = 0; row < 8; ++row) {
            const std::uint8_t pixels = (*glyph)[row];
            int column = 0;
            while (column < 8) {
                while (column < 8 && (pixels & (1u << (7 - column))) == 0) ++column;
                const int runStart = column;
                while (column < 8 && (pixels & (1u << (7 - column))) != 0) ++column;
                if (runStart < column) {
                    renderer.fillRect(glyphLeft + runStart * scale, top + row * scale,
                        (column - runStart) * scale, scale, color);
                }
            }
        }
    }
}

void drawResultOverlay(Renderer& renderer, const GameState& state) {
    const bool victory = state.won;
    const int width = static_cast<int>(state.width);
    const int height = static_cast<int>(state.height);
    const int panelWidth = victory ? 380 : 470;
    const int panelHeight = 140;
    const int panelLeft = (width - panelWidth) / 2;
    const int panelTop = (height - panelHeight) / 2;

    renderer.setBlend(true);
    renderer.fillRect(0, 0, width, height,
        victory ? Color{38, 122, 70, 132} : Color{150, 30, 34, 142});
    renderer.fillRect(panelLeft + 6, panelTop + 6, panelWidth, panelHeight, {0, 0, 0, 120});
    renderer.fillRect(panelLeft, panelTop, panelWidth, panelHeight, {18, 22, 20, 224});

    const Color border = victory ? Color{226, 194, 92} : Color{210, 82, 62};
    constexpr int borderWidth = 4;
    renderer.fillRect(panelLeft, panelTop, panelWidth, borderWidth, border);
    renderer.fillRect(panelLeft, panelTop + panelHeight - borderWidth, panelWidth,
        borderWidth, border);
    renderer.fillRect(panelLeft, panelTop, borderWidth, panelHeight, border);
    renderer.fillRect(panelLeft + panelWidth - borderWidth, panelTop, borderWidth,
        panelHeight, border);

    renderer.setBlend(false);
    // Keep result labels within the current built-in glyph set. Font expansion
    // is handled separately; these words render completely with existing glyphs.
    const std::u32string_view text = victory ? U"VICTORY" : U"CAPTURED";
    const int scale = 6;
    const int textWidth = pixelTextWidth(text, scale);
    const int textHeight = 8 * scale;
    const int left = panelLeft + (panelWidth - textWidth) / 2;
    const int top = panelTop + (panelHeight - textHeight) / 2;
    drawPixelText(renderer, text, left + 4, top + 4, scale, {10, 10, 10});
    drawPixelText(renderer, text, left, top, scale,
        victory ? Color{255, 246, 204} : Color{255, 222, 196});
}

void drawPauseOverlay(Renderer& renderer, const GameState& state) {
    const int width = static_cast<int>(state.width);
    const int height = static_cast<int>(state.height);
    renderer.setBlend(true);
    renderer.fillRect(0, 0, width, height, {4, 8, 12, 150});
    renderer.setBlend(false);
    const int panelWidth = 360;
    const int panelHeight = 120;
    const int left = (width - panelWidth) / 2;
    const int top = (height - panelHeight) / 2;
    renderer.fillRect(left, top, panelWidth, panelHeight, {18, 22, 20});
    renderer.fillRect(left, top, panelWidth, 4, {226, 194, 92});
    renderer.fillRect(left, top + panelHeight - 4, panelWidth, 4, {226, 194, 92});
    const std::u32string_view text = U"PAUSED";
    const int scale = 6;
    const int textWidth = pixelTextWidth(text, scale);
    drawPixelText(renderer, text, left + (panelWidth - textWidth) / 2, top + 28,
        scale, {255, 246, 204});
    const std::u32string_view hint = U"P TO RESUME";
    drawPixelText(renderer, hint, left + (panelWidth - pixelTextWidth(hint, 2)) / 2,
        top + 88, 2, {194, 180, 143});
}

void drawMenuOverlay(Renderer& renderer, const GameState& state) {
    const int width = static_cast<int>(state.width);
    const int height = static_cast<int>(state.height);
    renderer.setBlend(true);
    renderer.fillRect(0, 0, width, height, {4, 8, 12, 190});
    renderer.setBlend(false);
    const int panelWidth = 500;
    const int panelHeight = 230;
    const int left = (width - panelWidth) / 2;
    const int top = (height - panelHeight) / 2;
    renderer.fillRect(left, top, panelWidth, panelHeight, {18, 22, 20});
    renderer.fillRect(left, top, panelWidth, 4, {226, 194, 92});
    renderer.fillRect(left, top + panelHeight - 4, panelWidth, 4, {226, 194, 92});
    renderer.fillRect(left, top, 4, panelHeight, {226, 194, 92});
    renderer.fillRect(left + panelWidth - 4, top, 4, panelHeight, {226, 194, 92});
    renderer.fillRect(left + 62, top + 96, panelWidth - 124, 48, {46, 142, 68});
    renderer.fillRect(left + 62, top + 96, panelWidth - 124, 3, {108, 205, 113});
    renderer.fillRect(left + 62, top + 141, panelWidth - 124, 3, {22, 86, 42});

    const std::u32string_view title = U"CASTLE ESCAPE";
    const int titleScale = 4;
    drawPixelText(renderer, title, left + (panelWidth - pixelTextWidth(title, titleScale)) / 2,
        top + 24, titleScale, {255, 246, 204});
    const std::u32string_view action = U"START GAME";
    const int actionScale = 4;
    drawPixelText(renderer, action, left + (panelWidth - pixelTextWidth(action, actionScale)) / 2,
        top + 108, actionScale, {245, 255, 224});
    const std::u32string_view hint = U"ENTER / SPACE";
    const int hintScale = 2;
    drawPixelText(renderer, hint, left + (panelWidth - pixelTextWidth(hint, hintScale)) / 2,
        top + 172, hintScale, {194, 180, 143});
}

void renderFrame(Renderer& renderer, const GameState& state) {
    renderer.setBlend(false);
    renderer.fillRect(0, 0, static_cast<int>(state.width), static_cast<int>(state.height),
        {static_cast<std::uint8_t>(state.alarm ? 160 : 76),
            static_cast<std::uint8_t>(state.alarm ? 42 : 68),
            static_cast<std::uint8_t>(state.alarm ? 35 : 56)});
    renderer.fillRect(static_cast<int>(state.wall), static_cast<int>(state.wall),
        static_cast<int>(state.width - 2 * state.wall),
        static_cast<int>(state.height - 2 * state.wall), {23, 28, 26});

    const auto drawRect = [&](const auto& rect, Color color) {
        renderer.fillRect(static_cast<int>(rect[0]), static_cast<int>(rect[1]),
            static_cast<int>(rect[2]), static_cast<int>(rect[3]), color);
    };
    for (const auto& door : state.doors) {
        if (door[4] != 2) {
            drawRect(door, door[4] == 1 ? Color{55, 104, 76} : Color{166, 126, 52});
            continue;
        }
        const int x = static_cast<int>(door[0]), y = static_cast<int>(door[1]);
        const int w = static_cast<int>(door[2]), h = static_cast<int>(door[3]);
        drawRect(door, {37, 30, 28});
        renderer.fillRect(x + 2, y + 2, w - 4, h - 4, {107, 65, 49});
        renderer.fillRect(x + 4, y + 4, w - 8, h - 8, {70, 44, 37});
        for (int offset = 7; offset < w - 5; offset += 5) {
            renderer.fillRect(x + offset, y + 5, 2, h - 10, {88, 53, 42});
        }
        const int cx = x + w / 2, cy = y + h / 2;
        renderer.fillRect(x + 2, cy - 2, w - 4, 4, {43, 46, 48});
        renderer.fillRect(cx - 4, cy - 7, 8, 8, {221, 176, 91});
        renderer.fillRect(cx - 2, cy - 5, 4, 5, {70, 44, 37});
        renderer.fillRect(cx - 6, cy - 1, 12, 9, {221, 176, 91});
        renderer.fillRect(cx - 1, cy + 2, 2, 3, {70, 44, 37});
    }
    for (const auto& cover : state.covers) drawRect(cover, {91, 102, 94});
    for (const auto& object : state.objects) {
        if (object[4] == 1) drawWorldKey(renderer, object[0], object[1]);
        else drawRect(object, object[4] == 2 ? Color{102, 225, 177} :
            object[4] == 3 ? Color{255, 159, 64} : Color{93, 171, 224});
    }
    for (const auto& guard : state.guards) {
        const auto x = static_cast<int>(guard[0]);
        const auto y = static_cast<int>(guard[1]);
        renderer.fillRect(x, y, 16, 16, guard[2] <= 0 ? Color{90, 90, 65} :
            guard[4] > 0 ? Color{237, 142, 66} : Color{215, 75, 65});
        if (guard[3] > 0) {
            const char32_t digit = U'0' + static_cast<int>(guard[3]);
            drawPixelText(renderer, std::u32string_view(&digit, 1), x, y - 18, 2, {255, 218, 102});
        }
    }

    const int playerX = static_cast<int>(state.x);
    const int playerY = static_cast<int>(state.y);
    const int playerSize = static_cast<int>(state.size);
    renderer.fillRect(playerX, playerY, playerSize, playerSize, {221, 194, 113});
    renderer.drawLine(playerX + 8, playerY + 8,
        static_cast<int>(state.x + 8 + state.aimX * 24),
        static_cast<int>(state.y + 8 + state.aimY * 24), {221, 194, 113});

    const double aimLength = std::hypot(state.aimX, state.aimY);
    const double aimX = aimLength > 1e-9 ? state.aimX / aimLength : 1.0;
    const double aimY = aimLength > 1e-9 ? state.aimY / aimLength : 0.0;
    const double centerX = state.x + state.size / 2.0;
    const double centerY = state.y + state.size / 2.0;

    // Muzzle flash: three short primitives laid along the aim direction.
    if (state.flash > 0) {
        renderer.drawLine(centerX + aimX * 6, centerY + aimY * 6,
            centerX + aimX * 24, centerY + aimY * 24, {255, 248, 186});
        renderer.drawLine(centerX + aimX * 15 - aimY * 6, centerY + aimY * 15 + aimX * 6,
            centerX + aimX * 15 + aimY * 6, centerY + aimY * 15 - aimX * 6, {255, 255, 232});
        renderer.drawLine(centerX + aimX * 8 + aimY * 3, centerY + aimY * 8 - aimX * 3,
            centerX + aimX * 20 + aimY * 3, centerY + aimY * 20 - aimX * 3, {255, 184, 48});
    }

    // Flying bullet with a short normalized trail.
    if (state.bulletActive) {
        const double bulletLength = std::hypot(state.bulletDX, state.bulletDY);
        const double dirX = bulletLength > 1e-9 ? state.bulletDX / bulletLength : 1.0;
        const double dirY = bulletLength > 1e-9 ? state.bulletDY / bulletLength : 0.0;
        renderer.drawLine(state.bulletX - dirX * 18, state.bulletY - dirY * 18,
            state.bulletX - dirX * 3, state.bulletY - dirY * 3, {255, 164, 44});
        renderer.drawLine(state.bulletX - dirX * 9, state.bulletY - dirY * 9,
            state.bulletX - dirX * 2, state.bulletY - dirY * 2, {255, 228, 120});
        renderer.fillRect(static_cast<int>(std::lround(state.bulletX)) - 2,
            static_cast<int>(std::lround(state.bulletY)) - 2, 4, 4, {255, 255, 214});
    }

    for (int i = 0; i < state.ammo; ++i) {
        renderer.fillRect(24 + i * 12, 7, 6, 10, {221, 194, 113});
    }
    if (state.hasKey) drawHudKey(renderer, 112, 6);

    if (state.alarmSeconds > 0) {
        const auto seconds = std::to_string(state.alarmSeconds);
        drawPixelText(renderer, std::u32string(seconds.begin(), seconds.end()), 300, 5, 2, {255, 77, 69});
    }
    drawPixelText(renderer, std::u32string(state.objective.begin(), state.objective.end()),
        24, 30, 1, {255, 246, 204});
    if (state.won || state.lost) drawResultOverlay(renderer, state);
}
