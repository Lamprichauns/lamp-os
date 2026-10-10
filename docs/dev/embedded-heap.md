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
   the other. `wispstatus` is the standing exception: it builds its merge (well
   under 1 KB) on the host task, the same build the direct `CHAR_WISP_STATUS`
   read has always run there.
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

8. **A save allocates once, exactly, and only when the heap can spare it.** A
   large document serialized into an Arduino `String` or `std::string` grows by
   repeated reallocation, a staircase of blocks on an already fragmented heap,
   and a failed `String` reserve leaves a null buffer that NVS then
   dereferences. `util/heap_guard.hpp` holds the pattern: `heapCanSpare(n)`
   asks whether the largest free block holds `n` plus `kHeapHeadroom` (4 KB, so
   the radio stacks still find their buffers right after); `allocateSpare(n)`
   is the nothrow allocation behind it; `serializeGuarded(doc, len)` measures
   the document, refuses one that overflowed while it was built (it would save
   truncated), and serializes into one exactly-sized buffer. A refused save
   never aborts: it logs, keeps the dirty state and retries later.
   `Config::persistConfig` gates the save on the headroom, writes the expression
   records and knockout from stack buffers, and builds only the bounded "cfg"
   JSON on the heap; the dispositions flush writes its blob from the stack and
   touches no heap. A failed save, short heap or full NVS alike, backs off 5 s
   doubling to 60 s (`util/retry_backoff.hpp`) for both, and the commit drain
   waits on the same backoff, so no failure path retries at loop rate. Old
   stored formats load unchanged and move to the current keys on the first
   save. Do not lean on `try`/`catch (std::bad_alloc)` alone: the build sets
   `CONFIG_CXX_EXCEPTIONS_EMG_POOL_SIZE=0`, so on a truly exhausted heap the
   throw itself cannot allocate and ends in `terminate()`.

## The NVS budget

The `nvs` partition is 20 KB (0x5000, five 4 KB pages) and OTA cannot change
the partition table. NVS keeps one page free for garbage collection, so four
pages of 126 32-byte entries hold everything: 504 entries. A string costs a
header entry plus its bytes; a blob adds an index entry; a string must fit one
page; and rewriting an item needs room for the new copy before the old one is
erased. A single growing JSON string broke all of that: past about 2.4 KB the
config stopped saving, inside what the app allows.

`config_codec::saveConfig` splits the lamp's namespace so no value outgrows a
page and the whole fits at the caps (`config/config_types.hpp`):

| Key | Content | Worst case | Entries |
|---|---|---|---|
| `x0`..`x20` | one expression each, compact record (`encodeExpression`) | 141 B | 21 x 7 = 147 |
| `xn` | expression count | 1 B | 3 |
| `ko` | knockout level per pixel | 255 B | 10 |
| `cfg` | the rest as JSON (`toStoredJson`) | about 1.85 KB | 60 |
| `disp` | dispositions, `[mac:6][value:1]` per peer | 701 B | 24 |
| `lampType` | variant id | under 16 B | 2 |

Outside it: PHY calibration (a 1904-byte blob plus two small keys, 66 entries),
the WiFi driver's namespace (at most 64; the lamp now keeps WiFi config in RAM,
so it only shrinks), the crash record (4) and four namespace entries. The
rewrite copy of the largest item adds 60. Total 442 of 504 at the caps, and 421
for the standard variant's real worst case (its six types at every target),
both measured by `test_config_records`. A record stores parameter keys as indexes into
`kExpressionParamKeys`, which is append-only and pinned by
`test_builtin_descriptors`; a key no expression reads is dropped at parse.

The caps that bound `cfg`: 8 colours per surface segment (the app offers 6),
4 segments per role (variants define at most 3), 16-byte segment names, a
32-byte lamp name, a 64-byte password (a longer one is refused, never
truncated), a 32-byte SSID, 8 home-mode type names.

A partition that will not initialize at all is erased and re-initialized in
`setup()` (`NvsConfigStore::initFlash`) before anything opens it: every stored
setting is lost, but the lamp boots with Bluetooth, where leaving it would
need a USB reflash. Nothing on an uninitialized partition is readable through
the API, so there is nothing to salvage first. A failed NimBLE init skips every
later BLE start instead of faulting in `NimBLEServer::start`.

Saves go through the `nvs_*` API, never `Preferences`: Preferences logs a
failed write through a formatting path that faulted on a starved heap.

A reset at any point of a save leaves a whole config. The order is fixed:
the records, then their count, then the erase of records past it, then
`ko`, then `cfg` (which carries the legacy expression list until that last
write, so a first save after an update that is cut short still loads the old
form). An edit rewrites one record and NVS skips the unchanged ones; a remove
shifts later records down a slot, so a reset mid-shift can leave one record in
two slots, and the load keeps the first of any repeated (type, target). A
record that fails to read drops only itself, logged as
`[cfg] expression record xN unreadable`. `test_config_records` resets the store
at every write of a remove, an add-and-edit and a first migration. The boot
log's `[cfg] expressions=N from records|cfg json` is the loaded count; the
`cfg_bytes` on the line before is the "cfg" JSON alone.

## What expressions cost at runtime

Measured with the xtensa compiler (`sizeof`) and the buffers each type keeps,
on the standard variant's 38-pixel shade; every heap block also pays about 12
bytes (TLSF header plus light poisoning):

| Type | Object | Buffers | Heap per instance |
|---|---|---|---|
| glitchy | 112 B | colours 32, saved pixels 152 | about 330 B |
| pulse | 132 B | colours 32 | about 190 B |
| breathing | 108 B | colours 32 | about 165 B |
| shifty | 148 B | colours 32, saved pixels 152, fill offsets 152 (directional fills only) | about 390-540 B |
| spotty | 112 B | colours 32, 5 spots 60 | about 240 B |
| flicker (shimmer) | 176 B | colours 32, a cell per zone pixel 304 | about 550 B |

A target-both expression is two instances. Each configured expression adds
its `Config::expressions` slot (140 B, grown four slots at a time) and colour
list (about 44 B), and a 32-byte manager entry. The standard worst case (six
types at every target: 18 configs, 24 instances) is about 8.1 KB of instances,
2.7 KB of config, 0.8 KB of colours and 1 KB of entries: about 12.6 KB, against
about 18.4 KB before entries stopped copying parameters, the config vector
stopped doubling and shifty dropped its two per-pixel fade buffers. Up to four
transients come on top (a shifty pair is about 1 KB).

## What a crowd drives with no phone attached

A lamp in a crowd allocates on its own, with no app and no wisp. These paths
scale with the number of peers or the mesh's message rate, so each is bounded:

- **Cascade transients.** Every accepted MSG_COMMAND or MSG_EVENT invocation can
  start a one-shot heap Expression per target, alive until its animation
  completes (up to 180 s). At most `ExpressionManager::kMaxTransients` (4) are in
  flight, and one is skipped unless the largest free block holds 2 KB plus the
  headroom; a cascade past either is dropped. The vector is reserved at
  `ExpressionManager::begin`, and the compositor reserves 16 behavior slots
  past its boot list for transients and live upserts.
- **Expression parameters live inline.** `ExpressionParams` holds one
  optional `uint32_t` per `kExpressionParamKeys` slot (88 bytes), so a config,
  an invocation, a transient and a manager entry carry their parameters with
  no heap node per key, and copying them allocates nothing. Each expression's
  config exists once, in `Config::expressions`; a manager entry (32 bytes)
  holds its Expression and type and reads the parameters from Config by
  (type, target). Only an entry with no Config entry behind it (a relayed op,
  an invocation) keeps its own copy.
- **Peer-supplied expression data.** An invocation keeps at most 8 colours and
  16 parameter keys; an expression upsert (app, or a relayed `expressionOp`)
  is rejected outside targets 1-3 and held to the same colour and parameter
  caps, and the stored list to 21 entries (`config/config_types.hpp`). An
  upsert of a type the variant does not register is rejected. A stored
  config over the caps is truncated on load with a `[cfg]` line and saved
  truncated on the next save; a stored out-of-range target is kept, since
  `remove` matches it exactly.
- **Frame authentication.** The COMMAND/EVENT HMAC runs on the WiFi task for
  every such frame overheard, before dedup. It hashes on stack-held SHA-256
  contexts and allocates nothing.
- **The BLE scan.** The scan callback hands the roster the six address bytes,
  so a lamp advertisement builds no 17-char string on the host task.
- **Cascade fan-out.** A locally fired cascading expression builds its
  invocation document once and reuses one string across up to 50 peers.
- **Reads nobody asked for.** The section caches warm, and the state notify
  builds its payload, only while a BLE client holds the link.
- **OTA sessions.** The receiver's chunk bitmap (up to 8 KB per OFFER) is
  allocated only when `heapCanSpare` allows it; otherwise the OFFER is declined
  busy and the sender retries.

## The console lock

Lamp logs go through `Serial`, never stdout, so the first stdout write would be
an IDF driver's own log line, the WiFi and coex drivers logging from the
`esp_timer` task on core 0. The UART VFS driver creates its write lock (a
FreeRTOS mutex) on that first write, and newlib aborts when the allocation
fails, so on a starved heap a driver's error line crashed the lamp in
`lock_init_generic`. `setup()` writes one newline to stdout while the heap is
whole, which creates the lock at boot.

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
