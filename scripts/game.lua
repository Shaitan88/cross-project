local room = { width = 640, height = 400, wall = 24 }
local player = { x = 64, y = 200, size = 16, speed = 120, ammo = 6, aim_x = 1, aim_y = 0 }
local alarm = false
local current_room, door_open = 1, false
local has_key, won, lost = false, false, false
local flash = 0
local bullet = { active = false, x = 0, y = 0, dx = 0, dy = 0, remaining = 0 }
local rooms = {
    { guard = { x = 440, y = 80, phase = 40, direction = 1, detected = false, health = 2 },
      obstacle = { x = 0, y = 0, width = 0, height = 0 } },
    { guard = { x = 440, y = 200, phase = 200, direction = -1, detected = false, health = 2 },
      obstacle = { x = 300, y = 120, width = 24, height = 160 },
      key = { x = 552, y = 192, width = 16, height = 16 } }
}

local function in_doorway()
    return player.y >= 176 and player.y + player.size <= 240
end

function interact()
    if won or lost then return end
    local key = rooms[2].key
    if current_room == 2 and not has_key
        and player.x <= key.x + key.width + 8
        and player.x + player.size >= key.x - 8
        and player.y <= key.y + key.height + 8
        and player.y + player.size >= key.y - 8 then
        has_key = true
        return
    end
    if current_room == 1 and in_doorway() and player.x <= room.wall + 32 then
        if has_key then won = true end
        return
    end
    local door_x = current_room == 1 and room.width - room.wall - player.size or room.wall
    if in_doorway() and math.abs(player.x - door_x) <= 32 then
        door_open = not door_open
    end
end

local function move(horizontal, vertical, dt)
    local obstacle = rooms[current_room].obstacle
    local x = math.max(room.wall, math.min(room.width - room.wall - player.size,
        player.x + horizontal * player.speed * dt))
    if obstacle.width > 0 and player.y < obstacle.y + obstacle.height
        and player.y + player.size > obstacle.y then
        if horizontal > 0 and player.x + player.size <= obstacle.x then
            x = math.min(x, obstacle.x - player.size)
        elseif horizontal < 0 and player.x >= obstacle.x + obstacle.width then
            x = math.max(x, obstacle.x + obstacle.width)
        end
    end
    player.x = x
    local y = math.max(room.wall, math.min(room.height - room.wall - player.size,
        player.y + vertical * player.speed * dt))
    if obstacle.width > 0 and player.x < obstacle.x + obstacle.width
        and player.x + player.size > obstacle.x then
        if vertical > 0 and player.y + player.size <= obstacle.y then
            y = math.min(y, obstacle.y - player.size)
        elseif vertical < 0 and player.y >= obstacle.y + obstacle.height then
            y = math.max(y, obstacle.y + obstacle.height)
        end
    end
    player.y = y
    if dt > 0 and door_open and in_doorway() then
        if current_room == 1 and horizontal > 0 and player.x == room.width - room.wall - player.size then
            current_room, player.x = 2, room.wall
        elseif current_room == 2 and horizontal < 0 and player.x == room.wall then
            current_room, player.x = 1, room.width - room.wall - player.size
        end
    end
end

local function occluded(ax, ay, bx, by, obstacle)
    if obstacle.width == 0 then return false end
    local low, high = 0, 1
    local function clip(start, delta, minimum, maximum)
        if delta == 0 then return start >= minimum and start <= maximum end
        local first, last = (minimum - start) / delta, (maximum - start) / delta
        if first > last then first, last = last, first end
        low, high = math.max(low, first), math.min(high, last)
        return low <= high
    end
    local intersects = clip(ax, bx - ax, obstacle.x, obstacle.x + obstacle.width)
        and clip(ay, by - ay, obstacle.y, obstacle.y + obstacle.height)
    return intersects, low
end

function shoot()
    if won or lost then return end
    if player.ammo == 0 then return end
    player.ammo = player.ammo - 1
    alarm = true
    flash = 0.08

    local data = rooms[current_room]
    local guard = data.guard
    local ax, ay = player.x + 8, player.y + 8
    local aim_length = math.sqrt(player.aim_x * player.aim_x + player.aim_y * player.aim_y)
    local unit_x, unit_y = player.aim_x, player.aim_y
    if aim_length > 0 then
        unit_x, unit_y = player.aim_x / aim_length, player.aim_y / aim_length
    end
    local bx, by = ax + unit_x * 320, ay + unit_y * 320
    local ex, ey = bx, by

    if guard.health > 0 then
        local hit, fraction = occluded(ax, ay, bx, by, { x = guard.x, y = guard.y, width = 16, height = 16 })
        if hit then
            local hx = ax + (bx - ax) * fraction
            local hy = ay + (by - ay) * fraction
            if not occluded(ax, ay, hx, hy, data.obstacle) then
                guard.health = guard.health - 1
                if guard.health == 0 then guard.detected = false end
                ex, ey = hx, hy
            end
        end
    end

    local blocked, entry = occluded(ax, ay, ex, ey, data.obstacle)
    if blocked then
        ex = ax + (ex - ax) * entry
        ey = ay + (ey - ay) * entry
    end

    local distance = math.sqrt((ex - ax) * (ex - ax) + (ey - ay) * (ey - ay))
    if distance > 0 then
        bullet.active = true
        bullet.x, bullet.y = ax, ay
        bullet.dx, bullet.dy = unit_x, unit_y
        bullet.remaining = distance
    else
        bullet.active = false
    end
end

local function update_guards(dt)
    for index, data in ipairs(rooms) do
        local guard = data.guard
        if guard.health > 0 then
        guard.phase = (guard.phase + (dt % 6) * 40) % 240
        guard.direction = guard.phase < 120 and 1 or -1
        guard.x = 400 + (guard.phase < 120 and guard.phase or 240 - guard.phase)
        local dx, dy = player.x - guard.x, player.y - guard.y
        guard.detected = index == current_room and dx * guard.direction >= 0
            and dx * dx + dy * dy <= 400 * 400 and math.abs(dy) <= math.abs(dx)
            and not occluded(guard.x + 8, guard.y + 8, player.x + 8, player.y + 8, data.obstacle)
        if guard.detected then alarm = true end
        end
    end
end

local function update_effects(dt)
    if flash > 0 then
        flash = math.max(0, flash - dt)
    end
    if bullet.active then
        local step = 900 * dt
        if step >= bullet.remaining then step = bullet.remaining end
        if step < 0 then step = 0 end
        bullet.x = bullet.x + bullet.dx * step
        bullet.y = bullet.y + bullet.dy * step
        bullet.remaining = bullet.remaining - step
        if bullet.remaining <= 0 then bullet.active = false end
    end
end

function update(dt, horizontal, vertical)
    update_effects(dt)
    if won or lost then return end
    local length = math.sqrt(horizontal * horizontal + vertical * vertical)
    if length > 1 then
        horizontal, vertical = horizontal / length, vertical / length
    end
    if length > 0 then player.aim_x, player.aim_y = horizontal, vertical end
    move(horizontal, vertical, dt)
    update_guards(dt)
    local guard = rooms[current_room].guard
    if guard.health > 0
        and player.x < guard.x + 16 and player.x + player.size > guard.x
        and player.y < guard.y + 16 and player.y + player.size > guard.y then
        lost = true
        alarm = true
    end
end

function restart()
    player.x, player.y = 64, 200
    player.aim_x, player.aim_y = 1, 0
    player.ammo = 6
    alarm = false
    current_room, door_open = 1, false
    has_key, won, lost = false, false, false
    flash = 0
    bullet.active = false
    bullet.x, bullet.y = 0, 0
    bullet.dx, bullet.dy = 0, 0
    bullet.remaining = 0
    local initial = {
        { x = 440, y = 80, phase = 40, direction = 1 },
        { x = 440, y = 200, phase = 200, direction = -1 }
    }
    for index, data in ipairs(rooms) do
        local guard = data.guard
        guard.x, guard.y = initial[index].x, initial[index].y
        guard.phase, guard.direction = initial[index].phase, initial[index].direction
        guard.health, guard.detected = 2, false
    end
end

function snapshot()
    local data = rooms[current_room]
    local guard, obstacle = data.guard, data.obstacle
    local key = rooms[2].key
    return player.x, player.y, player.size, room.width, room.height, room.wall,
        current_room, door_open and 1 or 0, guard.detected and 1 or 0,
        guard.x, guard.y, guard.direction, guard.detected and 1 or 0,
        obstacle.x, obstacle.y, obstacle.width, obstacle.height,
        guard.health, guard.health == 0 and 1 or 0, player.ammo, alarm and 1 or 0,
        player.aim_x, player.aim_y,
        has_key and 1 or 0, won and 1 or 0, lost and 1 or 0,
        flash, bullet.active and 1 or 0, bullet.x, bullet.y,
        bullet.dx, bullet.dy, key.x, key.y
end
