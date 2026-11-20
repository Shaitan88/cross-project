local width, height, wall, size = 640, 400, 24, 16
local maps = {
    { hint = "THE GUARDS KEEP THE WAY OUT.",
      links = {{1,"R",2,"L"},{2,"R",3,"L"},{2,"D",4,"U"},{4,"R",3,"D"},{3,"R",5,"L","key"}},
      rooms = {{},{},{guard=100, object="key"},{cover={280,115,65,165}},{object="exit"}} },
    { hint = "THE GATE FEEDS ON TWO CIRCUITS.",
      links = {{1,"L",2,"R"},{2,"L",3,"R"},{1,"R",4,"L"},{4,"R",5,"L"},{1,"U",6,"D","power"}},
      rooms = {{},{cover={260,90,100,200}},{guard=76,object="switch"},
               {cover={300,165,40,165}},{guard=322,object="switch"},{object="exit"}} },
    { hint = "TWO GATES STAND BETWEEN YOU AND DAYLIGHT.",
      links = {{1,"R",2,"L"},{1,"D",3,"U"},{3,"R",2,"D"},{2,"R",4,"L","gate1"},
               {2,"U",5,"L","gate1"},{5,"R",4,"U"},{4,"R",6,"L","gate2"},{5,"D",6,"U","gate2"}},
      rooms = {{},{guard=151,cover={342,105,34,84},object="gate1"},{cover={270,120,64,165}},
               {guard=130,cover={334,95,30,142},object="gate2"},{cover={330,100,70,175}},{object="exit"}} }
}
local pendingExit
local level, room, player, rooms, flags, alarm, grace, won, lost, flash, bullets, interacted
local gateNames = {gate1=true,gate2=true}
local function rect(x,y,w,h) return {x,y,w,h} end
local function doorRect(side)
    if side == "L" then return rect(0,170,24,60) end
    if side == "R" then return rect(616,170,24,60) end
    if side == "U" then return rect(290,0,60,24) end
    return rect(290,376,60,24)
end
local function enterLevel(n, ammo)
    pendingExit = false
    level, room, flags, alarm, grace = n, 1, {}, nil, 1.5
    flash, bullets, interacted = 0, {}, false
    player = {x=64,y=192,dx=1,dy=0,ammo=ammo}
    rooms = {}
    for i, spec in ipairs(maps[n].rooms) do
        local guards = {}
        if spec.guard then
            local offset = spec.guard > 240 and 16 or -16
            guards[1] = {x=430,y=spec.guard,phase=30,dir=1,faceX=1,faceY=0,hp=2,
                route={{400,spec.guard},{520,spec.guard},{520,spec.guard+offset},
                       {400,spec.guard+offset}}}
        end
        rooms[i] = {doors={},guards=guards,cover=spec.cover and {spec.cover} or {},
            kind=spec.object, object=rect(spec.object == "exit" and (n == 3 and 190 or 530) or 552,
                gateNames[spec.object] and 294 or spec.object == "switch" and (i == 3 and 310 or 42)
                    or (n == 3 and spec.object == "exit" and 144 or 192),16,16)}
    end
    for _, link in ipairs(maps[n].links) do
        local state = {open=false, lock=link[5]}
        local a, x, b, y = link[1],link[2],link[3],link[4]
        table.insert(rooms[a].doors,{side=x, target=b, arrival=y, r=doorRect(x), state=state})
        table.insert(rooms[b].doors,{side=y, target=a, arrival=x, r=doorRect(y), state=state})
    end
end
function restart()
    won, lost = false, false
    enterLevel(1, 6)
end
local function locked(door)
    local lock = door.state.lock
    if lock == "power" then return not (flags[3] and flags[5]) end
    return lock ~= nil and not flags[lock]
end
local function near(r)
    local x,y = player.x+8,player.y+8
    return x >= r[1]-33 and x <= r[1]+r[3]+33 and y >= r[2]-33 and y <= r[2]+r[4]+33
end
function interact()
    interacted = false
    if won or lost then return false end
    local here = rooms[room]
    if here.kind and near(here.object) then
        local kind = here.kind
        if kind == "exit" then
            pendingExit = true
        elseif kind == "key" and not flags.key then
            flags.key = true
        elseif kind == "switch" and not flags[room] then
            flags[room] = true
        elseif gateNames[kind] and not flags[kind] then
            flags[kind] = true
        else
            return false
        end
        interacted = true
        return true
    end
    for _, door in ipairs(here.doors) do
        if near(door.r) and not locked(door) then
            door.state.open = not door.state.open
            interacted = true
            return true
        end
    end
    return false
end
local function overlaps(x,y,r)
    return x < r[1]+r[3] and x+size > r[1] and y < r[2]+r[4] and y+size > r[2]
end
local function move(dx,dy,dt)
    local p = player
    local x = math.max(wall,math.min(width-wall-size,p.x+dx*120*dt))
    local y = math.max(wall,math.min(height-wall-size,p.y+dy*120*dt))
    for _, r in ipairs(rooms[room].cover) do
        if overlaps(x,p.y,r) or (p.y < r[2]+r[4] and p.y+size > r[2]) then
            if dx > 0 and p.x+size <= r[1] then x = math.min(x,r[1]-size) end
            if dx < 0 and p.x >= r[1]+r[3] then x = math.max(x,r[1]+r[3]) end
        end
        if x < r[1]+r[3] and x+size > r[1] then
            if dy > 0 and p.y+size <= r[2] then y = math.min(y,r[2]-size) end
            if dy < 0 and p.y >= r[2]+r[4] then y = math.max(y,r[2]+r[4]) end
        end
    end
    p.x,p.y=x,y
    for _, door in ipairs(rooms[room].doors) do
        local side,r = door.side,door.r
        local horizontal = side=="L" or side=="R"
        local aligned = horizontal and (y>=r[2] and y+size<=r[2]+r[4])
            or (not horizontal and x>=r[1] and x+size<=r[1]+r[3])
        local crossing = side=="L" and dx<0 and x==wall or side=="R" and dx>0 and x==600
            or side=="U" and dy<0 and y==wall or side=="D" and dy>0 and y==360
        if aligned and crossing and door.state.open and not locked(door) then
            room,grace,bullets,flash=door.target,1.5,{},0
            local s = door.arrival
            p.x = s=="L" and 26 or s=="R" and 598 or 312
            p.y = s=="U" and 26 or s=="D" and 358 or 192
            return
        end
    end
end
local function hit(x,y,dx,dy,r)
    local low,high=0,1
    local function clip(p,d,a,b)
        if d==0 then return p>=a and p<=b end
        local m,n=(a-p)/d,(b-p)/d
        low,high=math.max(low,math.min(m,n)),math.min(high,math.max(m,n))
        return low<=high
    end
    if clip(x,dx,r[1],r[1]+r[3]) and clip(y,dy,r[2],r[2]+r[4]) then return low end
end
function aim(dx,dy)
    local length=math.sqrt(dx*dx+dy*dy)
    if length>0 then player.dx,player.dy=dx/length,dy/length end
end
function shoot()
    if won or lost or player.ammo==0 then return false end
    player.ammo,flash=player.ammo-1,0.08
    for _, guard in ipairs(rooms[room].guards) do
        local x, y = player.x-guard.x, player.y-guard.y
        local distance = math.sqrt(x*x+y*y)
        if guard.hp>0 and distance>0 and distance<=300 then
            guard.faceX,guard.faceY,guard.listen=x/distance,y/distance,0.75
        end
    end
    bullets[#bullets+1]={x=player.x+8,y=player.y+8,dx=player.dx,dy=player.dy,left=320}
    return true
end
local function updateBullets(dt)
    local here=rooms[room]
    for i=#bullets,1,-1 do
        local b=bullets[i]
        local distance=math.min(b.left,900*dt)
        local dx,dy=b.dx*distance,b.dy*distance
        local first,target=math.huge,nil
        for _,r in ipairs(here.cover) do
            local t=hit(b.x,b.y,dx,dy,r)
            if t and t<first then first=t end
        end
        for _,g in ipairs(here.guards) do
            local t=g.hp>0 and hit(b.x,b.y,dx,dy,rect(g.x,g.y,16,16))
            if t and t<first then first,target=t,g end
        end
        if target then
            target.hp=target.hp-1
            if target.hp==0 then target.timer,target.sees=nil,false end
        end
        b.x,b.y,b.left=b.x+dx,b.y+dy,b.left-distance
        if first<=1 or b.left<=0 then table.remove(bullets,i) end
    end
end
local function patrol(guard, dt)
    if guard.listen and guard.listen > 0 then
        guard.listen=guard.listen-dt
        return
    end
    local distance = (guard.phase+40*dt)%272
    guard.phase=distance
    for i, from in ipairs(guard.route) do
        local to=guard.route[i%#guard.route+1]
        local dx,dy=to[1]-from[1],to[2]-from[2]
        local length=math.abs(dx)+math.abs(dy)
        if distance<length then
            guard.x,guard.y=from[1]+dx/length*distance,from[2]+dy/length*distance
            if dx~=0 then guard.dir=dx/length end
            guard.faceX,guard.faceY=guard.dir,0
            return
        end
        distance=distance-length
    end
end
local function tick(dt, dx, dy)
    if won or lost then return end
    grace,flash=math.max(0,grace-dt),math.max(0,flash-dt)
    if dt>0 then move(dx,dy,dt) end
    updateBullets(dt)
    if alarm then
        alarm=alarm-dt
        if alarm<=1e-9 then alarm,lost=0,true return end
    end
    for i,area in ipairs(rooms) do
        for _,g in ipairs(area.guards) do
            if g.hp>0 then
                patrol(g,dt)
                local x,y=player.x-g.x,player.y-g.y
                local forward=x*g.faceX+y*g.faceY
                local sideways=x*g.faceY-y*g.faceX
                g.sees=grace<=1e-9 and i==room and forward>=math.abs(sideways)
                    and x*x+y*y<=90000
                if g.sees then
                    for _,r in ipairs(area.cover) do
                        if hit(g.x+8,g.y+8,x,y,r) then g.sees=false break end
                    end
                end
                if not alarm then
                    if g.timer then
                        g.timer=g.timer-dt
                        if g.timer<=1e-9 then alarm=10 end
                    elseif g.sees then g.timer=3 end
                end
                if i==room and grace<=1e-9 and overlaps(player.x,player.y,rect(g.x,g.y,16,16)) then
                    lost=true
                end
            end
        end
    end
    if pendingExit then
        pendingExit=false
        if lost then return end
        if level==#maps then won=true else enterLevel(level+1, player.ammo) end
    end
end
function update(dt, horizontal, vertical)
    aim(horizontal,vertical)
    local length=math.max(1,math.sqrt(horizontal*horizontal+vertical*vertical))
    local count=math.floor(math.min(dt,120)*60+0.5)
    for _=1,count do tick(1/60,horizontal/length,vertical/length) end
end
local function seconds(value)
    return value and math.max(0,math.ceil(value-1e-9)) or 0
end
function snapshot()
    local a,p=rooms[room],player
    local g=a.guards[1] or {x=0,y=0,dir=1,hp=0}
    local d=a.doors[1]
    local o=a.cover[1] or rect(0,0,0,0)
    local b=bullets[#bullets]
    local c=a.object
    local doors,guards={},{}
    for _,door in ipairs(a.doors) do
        doors[#doors+1]={table.unpack(door.r)}
        doors[#doors][5]=locked(door) and 2 or door.state.open and 1 or 0
    end
    for _,guard in ipairs(a.guards) do
        guards[#guards+1]={guard.x,guard.y,guard.hp,not alarm and seconds(guard.timer) or 0,
            guard.listen and guard.listen>0 and 1 or 0}
    end
    local objects={}
    if a.kind and not (a.kind=="key" and flags.key) then
        local marker= a.kind=="key" and 1 or a.kind=="exit" and 2
            or (flags[a.kind] or flags[room]) and 4 or 3
        objects[1]={c[1],c[2],c[3],c[4],marker}
    end
    return p.x,p.y,size,width,height,wall,room,d and d.state.open and 1 or 0,g.sees and 1 or 0,
        g.x,g.y,g.dir,g.sees and 1 or 0,o[1],o[2],o[3],o[4],g.hp,g.hp==0 and 1 or 0,
        p.ammo,alarm and 1 or 0,p.dx,p.dy,flags.key and 1 or 0,won and 1 or 0,lost and 1 or 0,
        flash,b and 1 or 0,b and b.x or 0,b and b.y or 0,b and b.dx or 0,b and b.dy or 0,
        c[1],c[2],level,#maps,d and d.r[1] or 0,d and d.r[2] or 0,d and d.r[3] or 0,
        d and d.r[4] or 0,d and d.side=="L" and 1 or 0,0,0,0,0,0,
        room==1 and maps[level].hint or "",interacted and 1 or 0,
        doors,guards,a.cover,objects,seconds(alarm)
end
restart()
