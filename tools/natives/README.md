# Native and Crossmap Maintenance

This BigBaseV2 branch targets **GTAV Enhanced only**.

```text
Edition: GTAV Enhanced
Executable: GTA5_Enhanced.exe
Crossmap build prefix: enhanced-
```

Do not import a Legacy `GTA5.exe` crossmap into this branch.

BigBaseV2 originally shipped with native wrappers generated in February 2019 and a fixed `crossmap.hpp`. Those files must be treated as versioned build data.

## Important distinction

- `natives.hpp` contains typed C++ wrappers, original native hashes, names, parameters, and return types.
- `crossmap.hpp` maps those original hashes to registration hashes for one specific executable build/edition.

Updating only `natives.hpp` does not update the runtime mappings. Updating only `crossmap.hpp` does not add new wrappers or corrected signatures.

Enhanced crossmaps must be generated for the exact `GTA5_Enhanced.exe` build being tested.

## 1. Generate current NativeDB wrappers

From the repository root:

```powershell
py tools/natives/generate_natives.py
```

This downloads alloc8or's public NativeDB and writes:

```text
BigBaseV2/src/natives.generated.hpp
```

To use a reviewed local database instead:

```powershell
py tools/natives/generate_natives.py --input tools/natives/data/natives.json
```

Review unknown parameter types and compile the generated header before replacing `BigBaseV2/src/natives.hpp`.

## 2. Import an Enhanced build-specific crossmap

The importer accepts JSON or CSV pairs:

```json
{
  "0xORIGINAL_HASH": "0xCURRENT_REGISTRATION_HASH"
}
```

or:

```text
0xORIGINAL_HASH,0xCURRENT_REGISTRATION_HASH
```

Generate the header:

```powershell
py tools/natives/import_crossmap.py tools/natives/data/crossmap.json `
  --edition enhanced `
  --executable GTA5_Enhanced.exe `
  --game-build enhanced-BUILD_NUMBER `
  --source "SOURCE NAME AND REVISION"
```

The importer refuses to write a crossmap when:

- the edition is not `enhanced`;
- the executable is not `GTA5_Enhanced.exe`;
- the build label does not begin with `enhanced-`;
- an original hash maps to conflicting current hashes;
- a hash is zero, malformed, or wider than 64 bits;
- the mapping does not cover the hashes referenced by `natives.hpp`.

Use `--allow-missing` only while investigating a new Enhanced build, never for a release build.

## 3. Runtime validation

`native_invoker::cache_handlers()` reports:

- total mappings;
- cached handlers;
- missing handlers;
- duplicate original hashes;
- direct-original-hash fallbacks.

A non-zero missing count means the Enhanced crossmap, native registration pointer, or executable build is wrong. Do not continue testing gameplay features until the cache is healthy.

## 4. Required Enhanced update checklist

1. Confirm the running executable is `GTA5_Enhanced.exe`.
2. Record its Windows file version and game build number.
3. Obtain a crossmap generated for that exact Enhanced build.
4. Generate and review typed wrappers from NativeDB.
5. Import the crossmap without `--allow-missing`.
6. Regenerate the Visual Studio solution.
7. Build in Debug.
8. Confirm native-cache startup coverage.
9. Test harmless natives first: player ID, coordinates, model validation, and UI status.
10. Only then test state-changing features.

## Source policy

Keep source name, revision/commit, edition, executable, and game build with every imported mapping. Do not accept anonymous crossmap dumps as release data.
