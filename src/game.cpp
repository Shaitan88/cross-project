#include "game.hpp"
#include "game_script.hpp"

#include <lua.hpp>
#include <cmath>
#include <stdexcept>
#include <string>

Game::Game() : lua_(luaL_newstate()) {
    if (!lua_) throw std::runtime_error("Could not create Lua state");
    try {
        luaL_openlibs(lua_);
        if (luaL_loadstring(lua_, game_script) != LUA_OK) {
            throw std::runtime_error(lua_tostring(lua_, -1));
        }
        call(0, 0);
    } catch (...) {
        lua_close(lua_);
        throw;
    }
}

Game::~Game() { lua_close(lua_); }

void Game::call(int arguments, int results) {
    if (lua_pcall(lua_, arguments, results, 0) != LUA_OK) {
        const char* error = lua_tostring(lua_, -1);
        const std::string message = error ? error : "Unknown Lua error";
        lua_pop(lua_, 1);
        throw std::runtime_error(message);
    }
}

void Game::update(double dt, int horizontal, int vertical) {
    if (!std::isfinite(dt) || dt < 0 || horizontal < -1 || horizontal > 1 || vertical < -1 || vertical > 1)
        throw std::invalid_argument("Invalid game time or movement input");
    lua_getglobal(lua_, "update");
    lua_pushnumber(lua_, dt);
    lua_pushinteger(lua_, horizontal);
    lua_pushinteger(lua_, vertical);
    call(3, 0);
}

bool Game::interact() {
    lua_getglobal(lua_, "interact");
    call(0, 1);
    const bool result = lua_toboolean(lua_, -1);
    lua_pop(lua_, 1);
    return result;
}

bool Game::shoot() {
    lua_getglobal(lua_, "shoot");
    call(0, 1);
    const bool result = lua_toboolean(lua_, -1);
    lua_pop(lua_, 1);
    return result;
}

void Game::aim(int horizontal, int vertical) {
    lua_getglobal(lua_, "aim");
    lua_pushinteger(lua_, horizontal);
    lua_pushinteger(lua_, vertical);
    call(2, 0);
}

void Game::restart() {
    lua_getglobal(lua_, "restart");
    call(0, 0);
}

GameState Game::snapshot() {
    lua_getglobal(lua_, "snapshot");
    constexpr int count = 48;
    if (!lua_checkstack(lua_, 56)) throw std::runtime_error("Lua stack unavailable");
    call(0, count + 5);
    double values[count]{};
    std::string objective;
    for (int i = 0; i < count; ++i) {
        if (i == 46) {
            if (!lua_isstring(lua_, i - count - 5)) {
                lua_pop(lua_, count);
                throw std::runtime_error("Lua objective must be a string");
            }
            objective = lua_tostring(lua_, i - count - 5);
            continue;
        }
        if (!lua_isnumber(lua_, i - count - 5)) {
            lua_pop(lua_, count);
            throw std::runtime_error("Lua snapshot contains an invalid value");
        }
        values[i] = lua_tonumber(lua_, i - count - 5);
    }
    GameState state{values[0], values[1], values[2], values[3], values[4], values[5],
        static_cast<int>(values[6]), values[7] != 0, values[8] != 0,
        {values[9], values[10], static_cast<int>(values[11]), values[12] != 0,
            static_cast<int>(values[17]), values[18] != 0},
        {values[13], values[14], values[15], values[16]},
        static_cast<int>(values[19]), values[20] != 0, values[21], values[22],
        values[23] != 0, values[24] != 0, values[25] != 0, values[26],
        values[27] != 0, values[28], values[29], values[30], values[31],
        values[32], values[33], static_cast<int>(values[34]), static_cast<int>(values[35]),
        {values[36], values[37], values[38], values[39]}, values[40] != 0,
        {values[41], values[42], values[43], values[44]}, values[45] != 0,
        std::move(objective), values[47] != 0};
    const auto readRows = [&](int index, auto& rows) {
        const auto length = lua_rawlen(lua_, index);
        for (std::size_t i = 1; i <= length; ++i) {
            lua_rawgeti(lua_, index, i);
            auto& row = rows.emplace_back();
            for (std::size_t j = 0; j < row.size(); ++j) {
                lua_rawgeti(lua_, -1, j + 1);
                row[j] = lua_tonumber(lua_, -1);
                lua_pop(lua_, 1);
            }
            lua_pop(lua_, 1);
        }
    };
    readRows(-5, state.doors);
    readRows(-4, state.guards);
    readRows(-3, state.covers);
    readRows(-2, state.objects);
    state.alarmSeconds = static_cast<int>(lua_tointeger(lua_, -1));
    lua_pop(lua_, count + 5);
    return state;
}
