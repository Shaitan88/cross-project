#pragma once

#include <array>
#include <string>
#include <vector>

struct lua_State;

struct GuardState {
    double x, y;
    int direction;
    bool detected;
    int health;
    bool dead;
};

struct RectState {
    double x, y, width, height;
};

struct GameState {
    double x, y, size, width, height, wall;
    int room;
    bool doorOpen, detected;
    GuardState guard;
    RectState obstacle;
    int ammo;
    bool alarm;
    double aimX, aimY;
    bool hasKey;
    bool won;
    bool lost;
    double flash;
    bool bulletActive;
    double bulletX;
    double bulletY;
    double bulletDX;
    double bulletDY;
    double keyX;
    double keyY;
    int level;
    int levelCount;
    RectState door;
    bool doorOnLeft;
    RectState exit;
    bool exitOnLeft;
    std::string objective;
    bool interactSucceeded;
    std::vector<std::array<double, 5>> doors, objects, guards;
    std::vector<std::array<double, 4>> covers;
    int alarmSeconds;
};

class Game {
public:
    Game();
    ~Game();
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;
    void update(double dt, int horizontal, int vertical);
    bool interact();
    bool shoot();
    void aim(int horizontal, int vertical);
    void restart();
    GameState snapshot();
private:
    lua_State* lua_ = nullptr;
    void call(int arguments, int results);
};
