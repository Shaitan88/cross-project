#include "render.hpp"

#include <array>
#include <cmath>
#include <string_view>

namespace {
using Glyph = std::array<const char*, 7>;

const Glyph* glyphFor(char32_t character) {
    static const Glyph p{{"#####", "#...#", "#...#", "#...#", "#...#", "#...#", "#...#"}};
    static const Glyph o{{".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}};
    static const Glyph b{{"#####", "#....", "#....", "####.", "#...#", "#...#", "####."}};
    static const Glyph e{{"#####", "#....", "#....", "####.", "#....", "#....", "#####"}};
    static const Glyph d{{"..##.", ".#.#.", ".#.#.", ".#.#.", ".#.#.", "#####", "#...#"}};
    static const Glyph a{{".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}};
    static const Glyph r{{"####.", "#...#", "#...#", "####.", "#....", "#....", "#...."}};
    static const Glyph zh{{"#.#.#", "#.#.#", "#.#.#", "#####", "#.#.#", "#.#.#", "#.#.#"}};
    static const Glyph n{{"#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}};
    static const Glyph i{{"#...#", "#...#", "#..##", "#.#.#", "##..#", "#...#", "#...#"}};
    static const Glyph u{{"#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.."}};
    static const Glyph z{{"#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"}};

    switch (character) {
    case U'П': return &p;
    case U'О': return &o;
    case U'Б': return &b;
    case U'Е': return &e;
    case U'Д': return &d;
    case U'А': return &a;
    case U'Р': return &r;
    case U'Ж': return &zh;
    case U'Н': return &n;
    case U'И': return &i;
    case U'У': return &u;
    case U'З': return &z;
    default: return nullptr;
    }
}

// Wide lock-and-handle detail for the room-1 exit. The normal room door never
// gets this treatment, so the exit stays visually distinct from both doorways.
void drawExit(Renderer& renderer, bool unlocked) {
    if (unlocked) {
        renderer.fillRect(0, 176, 24, 64, {46, 142, 68});          // green door
        renderer.fillRect(0, 176, 24, 2, {226, 194, 92});           // gold frame
        renderer.fillRect(0, 238, 24, 2, {226, 194, 92});
        renderer.fillRect(0, 176, 2, 64, {226, 194, 92});
        renderer.fillRect(22, 176, 2, 64, {226, 194, 92});
        renderer.fillRect(5, 205, 14, 4, {240, 208, 104});          // bright lock bar
        renderer.fillRect(15, 199, 5, 11, {255, 228, 132});         // gold knob
    } else {
        renderer.fillRect(0, 176, 24, 64, {108, 40, 34});           // dark red/brown
        renderer.fillRect(0, 176, 24, 2, {62, 23, 20});             // dark frame
        renderer.fillRect(0, 238, 24, 2, {62, 23, 20});
        renderer.fillRect(0, 176, 2, 64, {62, 23, 20});
        renderer.fillRect(22, 176, 2, 64, {62, 23, 20});
        renderer.fillRect(5, 205, 14, 4, {64, 57, 52});             // iron lock bar
        renderer.fillRect(15, 199, 5, 11, {146, 118, 54});          // dull knob
    }
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
    return (static_cast<int>(text.size()) * 6 - 1) * scale;
}

void drawPixelText(Renderer& renderer, std::u32string_view text, int left, int top,
    int scale, Color color) {
    if (scale <= 0) return;
    for (std::size_t index = 0; index < text.size(); ++index) {
        const Glyph* glyph = glyphFor(text[index]);
        if (!glyph) continue;
        const int glyphLeft = left + static_cast<int>(index) * 6 * scale;
        for (int row = 0; row < 7; ++row) {
            for (int column = 0; column < 5; ++column) {
                if ((*glyph)[row][column] == '#') {
                    renderer.fillRect(glyphLeft + column * scale, top + row * scale,
                        scale, scale, color);
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
    const std::u32string_view text = victory ? U"ПОБЕДА" : U"ПОРАЖЕНИЕ";
    const int scale = victory ? 7 : 6;
    const int textWidth = pixelTextWidth(text, scale);
    const int textHeight = 7 * scale;
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
    const std::u32string_view text = U"ПАУЗА";
    const int scale = 8;
    const int textWidth = pixelTextWidth(text, scale);
    drawPixelText(renderer, text, (width - textWidth) / 2, (height - 7 * scale) / 2,
        scale, {255, 246, 204});
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

    // Room-transition door: room 1 right wall, room 2 left wall.
    renderer.fillRect(state.room == 1 ? 616 : 0, 176, 24, 64,
        state.doorOpen ? Color{23, 28, 26} : Color{136, 100, 26});

    // Room-1 left-wall exit is separate from every transition door.
    if (state.room == 1) drawExit(renderer, state.hasKey);

    renderer.fillRect(static_cast<int>(state.obstacle.x), static_cast<int>(state.obstacle.y),
        static_cast<int>(state.obstacle.width), static_cast<int>(state.obstacle.height),
        {91, 102, 94});

    const int guardX = static_cast<int>(state.guard.x);
    const int guardY = static_cast<int>(state.guard.y);
    renderer.fillRect(guardX, guardY, 16, 16,
        state.guard.dead ? Color{90, 90, 65} : Color{215, 75, 65});
    if (state.guard.dead) {
        renderer.drawLine(guardX, guardY, guardX + 15, guardY + 15, {23, 28, 26});
        renderer.drawLine(guardX + 15, guardY, guardX, guardY + 15, {23, 28, 26});
    }

    const int playerX = static_cast<int>(state.x);
    const int playerY = static_cast<int>(state.y);
    const int playerSize = static_cast<int>(state.size);
    renderer.fillRect(playerX, playerY, playerSize, playerSize, {221, 194, 113});
    renderer.drawLine(playerX + 8, playerY + 8,
        static_cast<int>(state.x + 8 + state.aimX * 24),
        static_cast<int>(state.y + 8 + state.aimY * 24), {221, 194, 113});

    // World key is only present in room 2 and only until it is collected.
    if (state.room == 2 && !state.hasKey) drawWorldKey(renderer, state.keyX, state.keyY);

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

    if (state.won || state.lost) drawResultOverlay(renderer, state);
}
