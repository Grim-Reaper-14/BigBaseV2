bigbase.log_info("example.lua loaded with " .. bigbase.version)

local ticks = 0

function on_tick()
    ticks = ticks + 1

    if ticks == 600 then
        bigbase.log_info("example.lua has completed 600 ticks")
    end
end

function on_unload()
    bigbase.log_info("example.lua unloaded")
end
