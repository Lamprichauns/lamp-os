# Embedded heap discipline

`malloc(N)` needs N *contiguous* bytes, so the number that bites is the largest
free block, not total free. Two figures matter and they measure different things:

- **Settled-boot headroom** (idle, no BLE client): free heap ~46.5 KB,
  fragmented to a ~40.9 KB largest contiguous block, with BLE, ESP-NOW, and wifi
  resident. This is the quiet-desk best case.
- **Runtime largest free block** (BLE client connected + a full roster): the
  largest contiguous block craters to **~23.5 KB**. This is the real cap on
  roster / per-peer / hot-path growth, and the number to design against.

A whole column of independent bugs turned out to be one anti-pattern hitting the
runtime ceiling. Read this before adding any feature that snapshots the roster,
tracks per-peer state, or answers "who's nearby." Measure with the probe (below)
on the bench under real BLE + fleet load; the quiet-desk figure hides the cliff.

The softAP webapp and BLE are heap-exclusive: a config client connecting can
squeeze free heap into a range too tight for the AsyncWebServer to serve the
config page, so `ble_control::deinitForWebapp()` tears BLE (NimBLE host + BT
controller) down on client-connect to give the serve headroom. BLE comes back
only via a reboot on session end (`webapp::tick()`), never an in-place re-init.

## The recurring anti-pattern

Fleet-awareness features (crowd-dim, claims, nearby lists, greetings, personality)
all answer "who's around me." The convenient implementation snapshots the roster
into a fresh `std::vector`, one `std::string` per entry, then sorts it. That is
fine on a laptop and lethal here, and each feature reinvented it. `personality_engine`
ran it on every main-loop tick (kHz); the wisp ran it at ~200 Hz. They were the top
fragmentation drivers on the heap.

Two properties make it lethal:

- **kHz alloc/free churn of variable-size blocks fragments a no-compaction heap.**
  The vector and its N strings allocate and free every pass. Nothing moves once
  placed, so a long-lived allocation that lands in a freed gap keeps that gap from
  merging back. The largest contiguous block erodes and stays eroded. It does not
  even recover when the triggering BLE client disconnects, because the churn never
  stops.
- **A 17-char lampId (`"AA:BB:CC:DD:EE:FF"`) defeats std::string SSO** (the inline
  buffer holds ~15 chars). Every address becomes a separate heap allocation.

## Rules

1. **Watch `largest`, not just `free`.** `heap_caps_get_largest_free_block()` is the
   real budget. `util/heap_probe.hpp` (`lamp::logHeap`, LAMP_DEBUG) logs both at
   boot / mesh / webapp / ble-connect / ble-disconnect / ble-pre / ble-post /
   ota-stream / web-save. Tap the serial, grep `[heap]`, find the checkpoint that
   craters the block. Measure the fragmenter; do not theorize it.
2. **No heap churn on the hot path.** Never rebuild-and-sort a fresh allocation per
   loop tick. Throttle it, dirty-gate it (rebuild only when the roster changed), or
   reuse a member buffer `reserve()`d once to its max.
3. **No per-entry `std::string` for addresses or short keys.** Use a fixed
   `char[18]` or the raw 6-byte address. Anything past ~15 chars heap-allocates.
4. **Scan in place, do not snapshot.** "Closest Smitten peer", "nearest by RSSI",
   "count of X": iterate the roster where it lives, allocation-free. Do not
   materialize a sorted copy to read one value off it.
5. **Front-load permanents; preallocate.** Long-lived allocations belong at boot,
   while the heap is still one contiguous slab, `reserve()`d to their max, so they
   pack low instead of bisecting the runtime free region.
6. **No heavy work under a spinlock or on the BLE task.** A JSON build inside
   `portENTER_CRITICAL` masks interrupts for milliseconds; a 10 KB JSON built on the
   NimBLE host task competes with the largest free block. Pre-build on a Core-1
   cache and let the mux cover only the pointer swap. Every section payload works
   this way except `exprcat`, which is generated into `.rodata` at build time and
   served by reference, so it never builds on the host task at all; match one or
   the other.
7. **Prefer a shared roster view over a new snapshot.** "Who's around" gets asked
   from many features, so a single shared roster view earns its keep. Do not let
   the next feature roll its own vector-of-strings copy. The shared views are
   `snapshotNear`, `getMesh`, and `getAll`. Each copies the matching entries
   under the roster mutex into a member vector of a slim, trivially copyable
   projection holding only the fields its callers read (`NearbyCopy`,
   `MeshPeer`, `ListedPeer`, a fraction of `RosterEntry`'s size), then filters
   or sorts outside the lock, which keeps the critical section short enough for
   the ESP-NOW receive path's bounded takes. The near and mesh buffers are
   `reserve()`d to `kCapacity` (50) when the roster is constructed at boot, so
   a growing roster never reallocates them. The `getAll` buffer serves only the
   app-facing nearby list: it is reserved to `kCapacity` in one allocation on
   the first build after an app or webapp client connects and released when the
   last one disconnects, and the build skips (logging OOM) if that reservation
   throws. A new roster consumer reads one of these views, adding a field to its
   projection if needed, and never takes a full `RosterEntry` copy per peer.

   The buffers are **single-task and non-reentrant**: loop task (Core 1) only,
   and a reference is invalidated by the next call to the same getter.
   `LAMP_DEBUG` asserts both (unexpected task, re-entrant fill). A path that
   hands each peer to a **user callback** walks the `snapshotNear` reference: a
   callback re-entering `getMesh`/`getAll` fills a different buffer and cannot
   invalidate the walk. The one uncovered case is a nested `snapshotNear` that
   rebuilds (a `forEachNearby` or `crowd()` inside a near-walk callback) and
   overwrites the buffer mid-walk; no near-walk nests today. `snapshotNear` also
   caches: repeated calls with the same `maxAgeMs` within `kSnapshotCacheMs`
   (1 s) return the existing copy without re-locking or sorting, so a behavior
   calls it per frame and does not roll its own throttle. A different
   `maxAgeMs` rebuilds.

## Reading a crash record

Every build, released ones included, keeps a crash record (`core/crash_record`).
The loop samples the running minimum largest block, minimum free heap, roster
peak, and uptime into RTC memory at 1 Hz; the panic handler adds the reason, pc,
first four backtrace addresses, and core. RTC memory survives a panic or
watchdog reset but not power loss, so a magic value gates it. On the next boot a
panic, int/task/other watchdog, or brownout reset (or a captured panic under any
reset) bumps a counter and writes the record to NVS (namespace `crash`, key
`last`). A clean `esp_restart` (OTA, webapp reboot) and power-on do not count.

Read it from the BLE `lamp` section:

```json
"resetReason": 4,
"lastCrash": {"count": 3, "reset": 4, "fw": 66055,
  "uptimeMs": 5412000, "minLargestBlock": 1300, "minFreeHeap": 5120,
  "rosterPeak": 12, "reason": "abort() was called", "core": 0,
  "pc": "0x4008a1b2", "bt": ["0x4008a1b2", "0x400d3c44", "0x400e0123"]}
```

`reset` and `resetReason` are `esp_reset_reason_t` ordinals (4 panic, 5 int WDT,
6 task WDT, 7 other WDT, 9 brownout, 3 clean restart, 1 power-on). The heap and
roster fields are absent when RTC memory failed its check (a brownout often
does); `reason`/`core`/`pc`/`bt` are absent unless the panic handler ran.
`minLargestBlock` near 1-2 KB with a high `rosterPeak` points at fragmentation.
Feed `pc` and `bt` to `xtensa-esp32-elf-addr2line -e firmware.elf` for the build
matching `fw` (packed `major<<16 | minor<<8 | patch`).

## Why it recurs

The features that make the fleet feel alive constantly re-answer "who's near me,"
so the snapshot pattern gets reinvented per feature. It compiles clean, passes the
native tests, and only surfaces as a functional failure (the web config UI will not
serve, the wisp goes invisible) under real fleet and BLE load on the bench, never on
a quiet desk. Assume heap pressure, measure with the probe, and reach for
allocation-free access to shared fleet state.

Related: [`code-smells.md`](code-smells.md) (the reinvention smell) and
[`networking.md`](networking.md) (roster and mesh).
