# Sol2 Lua setup

BigBaseV2 expects the following dependency layout:

```text
vendor/
├── lua/
│   └── src/
│       ├── lua.h
│       ├── lapi.c
│       └── ...
└── sol2/
    └── include/
        └── sol/
            └── sol.hpp
```

From the repository root, place the official Lua source tree in `vendor/lua` and the Sol2 repository in `vendor/sol2`.

Example setup commands:

```bat
git clone https://github.com/lua/lua.git vendor/lua
git clone https://github.com/ThePhD/sol2.git vendor/sol2
premake5 vs2019
```

The Premake workspace builds Lua as a static C library and treats Sol2 as a header-only dependency.

## Script directory

Lua scripts are scanned from a `scripts` directory relative to the process working directory. The menu creates this directory when it does not exist.

## Script lifecycle

A script is executed once when loaded. It may optionally declare:

```lua
function on_tick()
    -- Called from BigBaseV2's script fiber.
end

function on_unload()
    -- Called before the script is removed or BigBaseV2 shuts down.
end
```

## Built-in API

The initial safe API contains:

```lua
bigbase.version
bigbase.log_info(message)
bigbase.log_warning(message)
bigbase.log_error(message)
```

Sandboxed scripts receive base, coroutine, string, table, math, and UTF-8 functionality. File, operating-system, package-loading, and debug libraries are only available when Sandbox Scripts is disabled.
