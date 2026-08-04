# Memory Utilities

This folder contains the in-process address and pattern-scanning utilities used during BigBaseV2 startup.

## Components

- `handle.hpp` — typed address wrapper, offsets, trivial reads, and RIP-relative resolution.
- `range.hpp/.cpp` — bounded address ranges and single/all pattern scans.
- `module.hpp/.cpp` — validated 64-bit PE module ranges and exported symbols.
- `pattern.hpp/.cpp` — validated IDA signatures and byte/mask patterns.
- `pattern_batch.hpp/.cpp` — named pattern resolution with callbacks and aggregated diagnostics.
- `all.hpp` — convenience include for the complete memory layer.

## IDA signatures

Tokens must be separated by whitespace. Wildcards may use `?` or `??`.

```cpp
memory::pattern signature("48 8B 05 ? ? ? ? 48 85 C0");
```

Malformed, incomplete, or empty signatures throw `std::invalid_argument` during construction rather than silently producing a partial pattern.

## Scanning

Ranges use half-open boundaries: `[begin, end)`. The address returned by `end()` is one past the final valid byte and is not contained in the range.

```cpp
memory::module game(nullptr);
const auto result = game.scan("48 8B 05 ? ? ? ?");
if (result)
{
    const auto target = result.add(3).rip();
}
```

A pattern whose size equals the range size is valid. Patterns larger than the range return no result without unsigned underflow.

## Pattern batches

```cpp
memory::pattern_batch batch;
batch.add("Game state", "83 3D ? ? ? ? ?", [](memory::handle address)
{
    // Resolve and store the pointer.
});

batch.run(memory::module(nullptr));
```

By default, `run()` scans every entry, logs every missing pattern or failed callback, clears the batch, and throws one aggregated `std::runtime_error` if anything failed.

For diagnostics without throwing:

```cpp
const auto result = batch.run(memory::module(nullptr), false);
if (!result.success())
{
    // Inspect result.missing.
}
```

## Address handling

`memory::handle` does not own or validate the memory it refers to. Callers must only dereference addresses known to belong to a readable in-process range.

```cpp
memory::handle address = game.begin().add(0x1000);
auto pointer = address.as<void*>();
auto value = address.read<std::uint32_t>();
auto rip_target = address.rip();
```

`read<T>()` is limited to trivially copyable types and returns a zero-initialized value for a null handle.
