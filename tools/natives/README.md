# Native and Crossmap Maintenance

This BigBaseV2 branch targets **GTAV Enhanced only**.

```text
Edition: GTAV Enhanced
Executable: GTA5_Enhanced.exe
Native table source: YimMenu/YimMenuV2 enhanced branch
```

Do not import Legacy `GTA5.exe` native mappings into this branch.

## Enhanced native architecture

YimMenuV2 Enhanced does not use the old pair-based crossmap format. Its
`Crossmap.hpp` is an ordered array of original native hashes. GTA Enhanced's
`InitNativeTables` routine rewrites those array entries into native handler
pointers.

BigBaseV2 now follows that architecture while preserving its existing typed,
hash-based wrapper API:

1. Copy the ordered Enhanced hashes into handler-sized storage.
2. Pass a temporary `rage::scrProgram` to `InitNativeTables`.
3. Let the game replace each hash with its native handler.
4. Build BigBaseV2's original-hash to handler cache from the populated array.

## 1. Synchronize YimMenuV2's Enhanced table

From the repository root:

```powershell
py tools/natives/sync_yimmenuv2_crossmap.py
```

This downloads the official file from:

```text
YimMenu/YimMenuV2
branch: enhanced
src/game/gta/invoker/Crossmap.hpp
```

It validates the declared entry count, rejects zero or duplicate hashes, and
writes:

```text
BigBaseV2/src/crossmap_enhanced.hpp
```

For an offline or reviewed source copy:

```powershell
py tools/natives/sync_yimmenuv2_crossmap.py `
  --input tools/natives/data/YimMenuV2-Crossmap.hpp `
  --source-ref "YimMenu/YimMenuV2:COMMIT_SHA"
```

Pin a reviewed commit SHA for release builds rather than relying on a moving
branch head.

## 2. Generate current typed NativeDB wrappers

The ordered hash table resolves handlers, while `natives.hpp` defines C++
function names, parameters, return types, and original hashes. Refresh those
separately:

```powershell
py tools/natives/generate_natives.py
```

This writes:

```text
BigBaseV2/src/natives.generated.hpp
```

To use a reviewed local NativeDB file:

```powershell
py tools/natives/generate_natives.py --input tools/natives/data/natives.json
```

Review unknown parameter types and compile the generated header before replacing
`BigBaseV2/src/natives.hpp`.

## 3. Runtime validation

`native_invoker::cache_handlers()` now reports:

- ordered Enhanced hash count;
- successfully populated handlers;
- missing handlers;
- duplicate original hashes;
- YimMenuV2 source URL and source reference.

When `crossmap_enhanced.hpp` is absent, BigBaseV2 remains compilable but native
execution is disabled with this instruction in the log:

```text
py tools/natives/sync_yimmenuv2_crossmap.py
```

Do not test gameplay features unless the Enhanced native cache reports complete
and healthy coverage.

## 4. Required update checklist

1. Confirm the running executable is `GTA5_Enhanced.exe`.
2. Record its Windows file version and online version.
3. Synchronize the official YimMenuV2 Enhanced table.
4. Record the YimMenuV2 commit SHA used for the release.
5. Generate and review typed wrappers from NativeDB.
6. Regenerate the Visual Studio solution.
7. Build in Debug.
8. Confirm the `InitNativeTables` pattern resolves.
9. Confirm every ordered hash receives a non-null handler.
10. Test harmless natives first: player ID, coordinates, and model validation.
11. Only then test state-changing features such as spawning or teleporting.

## Legacy pair importer

`import_crossmap.py` is retained only for analyzing historical pair-based tables.
It is not the preferred GTA V Enhanced runtime path and should not overwrite
`crossmap_enhanced.hpp`.

## Source policy

Keep the source repository, branch or commit, executable edition, game version,
and generated-file timestamp with every native-table update. Do not accept
anonymous crossmap dumps as release data.
