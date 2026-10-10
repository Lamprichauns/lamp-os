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
   A config save takes no heap at all, so it never waits on a large free block:
   the expression records and knockout are written from stack buffers, and the
   "cfg" JSON streams through `JsonWriter` (`util/json_writer.hpp`, compact
   JSON straight into a fixed buffer, byte-identical to ArduinoJson) into a
   2176-byte buffer reserved in .bss (`kMaxStoredCfgJsonLen`, above the
   2121-byte worst case with every free-text field fully escaped and every
   number at its widest). The dispositions flush writes its blob from the
   stack. A
   failed save (a full NVS, or a cfg JSON that outgrew its buffer) backs off 5 s
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

`config_codec::saveConfig` writes one of two shapes, chosen per save:

- **Whole**, when everything fits the 2176-byte save buffer: one `cfg` JSON that
  carries the expressions and the knockout levels too. Every firmware reads
  this form, so a lamp flashed back to an older release keeps its
  expressions. A typical setup (up to about four expressions with full colour
  lists) is saved this way.
- **Split**, for anything bigger, so no value outgrows a page and the whole
  fits at the caps (`config/config_types.hpp`):

| Key | Content | Worst case | Entries |
|---|---|---|---|
| `x0`..`x20` | one expression each, compact record (`encodeExpression`) | 121 B | 21 x 6 = 126 |
| `xn` | expression count | 1 B | 3 |
| `ko` | knockout level per pixel | 255 B | 10 |
| `cfg` | the rest as JSON (`writeStoredJson`) | 2121 B fully escaped | 68 |
| `disp` | dispositions, `[mac:6][value:1]` per peer | 701 B | 24 |
| `lampType` | variant id | under 16 B | 2 |

Outside it, 166 entries (`systemEntries` in `test_config_records`):

- 5 namespace entries: `lamp`, `crash`, `phy`, `nvs.net80211`, `nimble_bond`.
- PHY calibration: a 1904-byte blob (62), a 6-byte blob (3), a u32 (1): 66.
- The WiFi driver's namespace: empty. The lamp keeps WiFi config in RAM and
  erases that namespace at boot.
- The crash record: one blob under 64 bytes, 4.
- NimBLE's bond store (`ble_store_nvs.c`, `nimble_bond`; bonding and
  host-based privacy are on, so legacy bonds may be present): with
  `MAX_BONDS` 3, `MAX_CCCDS` 8 and `MAX_CSFCS` 1 (the connection count),
  `our_sec_N` and `peer_sec_N` blobs of `ble_store_value_sec` (88 bytes on
  the ESP32, 5 entries each) for 3 bonds is 30; `p_dev_rec_N` blobs of
  `ble_hs_dev_records` (44 bytes, 4 each) for 4 is 16; `cccd_sec_N` (16
  bytes, 3 each) for 8 is 24; `csfc_sec_N` (8 bytes) 3; `local_irk_N` and
  `rpa_rec_N` (under 32 bytes, 3 each) for 3 each is 18. Total 91.

The rewrite copy of the largest item adds 68. Total 467 of 504 with every
free-text field quote- or backslash-filled, 451 with plain text, and 433 for
the standard variant's real worst case (its six types at every target). A
whole save needs 332, and the moment a split config turns whole (the new
`cfg` lands before the records go) peaks at 471. All are measured by
`test_config_records`, each with at least 32 entries spare. An expression
record holds at most 12 parameters (`kMaxExpressionParams`; every type carries
at most 11, guarded by `test_builtin_descriptors`) and carries each key as its
index in `kExpressionParamKeys`, so that table is append-only, pinned by
`test_builtin_descriptors`, and kept in byte-wise key order
(`test_param_wire_compat`); the JSON forms carry key names. A key no
expression reads is dropped at parse.

The caps that bound `cfg`: 8 colours per surface segment (the app offers 6),
4 segments per role (variants define at most 3), 16-byte segment names, a
32-byte lamp name, a 64-byte password, a 32-byte SSID, a byte order the strip
driver knows, 8 home-mode type names (type ids only). The BLE settings path
and the web PUT both refuse an over-long password or SSID (BLE logs the
refusal, the web answers 400), and both ignore an unknown byte order;
`fromJson` also bounds what it loads (truncating a password or SSID stored
before the cap; the BOOT button clears a password), so no field can make
every save fail. A repeated (type, target) in the cfg JSON loads once, the
first kept, as the records do.

Arduino's `initArduino` runs `nvs_flash_init` before `setup()`, erasing and
re-initializing on `NO_FREE_PAGES` or `NEW_VERSION_FOUND`.
`NvsConfigStore::initFlash` runs again first thing in `setup()` (a no-op on an
initialized partition) and erases only for those two codes or
`NOT_ENOUGH_SPACE`, the codes that mean the partition itself is unusable:
every stored setting is lost, but the lamp boots with Bluetooth, where leaving
it would need a USB reflash. Any other error (out of memory, a transient flash
fault) erases nothing and the lamp runs without NVS. A failed NimBLE init
skips every later BLE start, OTA radio pause and resume included.

A partition that initializes but is full gets space back on the first boot:
the WiFi driver's namespace is erased, and the legacy dispositions JSON (about
2.2 KB) is rewritten as the 701-byte blob, blob first, then erased. A legacy
JSON that does not parse, or parses to nothing while the blob holds entries,
is erased without replacing the blob.

Saves go through the `nvs_*` API, never `Preferences`: Preferences logs a
failed write through a formatting path that faulted on a starved heap. Every
save runs on the loop task: a web save is handed from the AsyncTCP task to
`webapp::tick` under a sequence number, and the handler takes only the result
for its own number. A handler that waits 3 s withdraws its body if the loop
has not taken it (500, nothing saved); if the loop has it, the handler
answers 202 and the save lands on its own. A landed web save arms the reboot
that loads it, drops any pending retry, and makes `persistConfig` decline
until that reboot, so the running config cannot overwrite it.

A reset at any point of a save leaves a whole config. A whole save writes
`cfg` (always with an `expressions` array, empty included, so deleting every
expression and losing power before the erase cannot bring them back), then
erases every record slot `x0`..`x31`, then `xn` and `ko`. A split save builds
its `cfg` JSON first (TooLarge writes nothing), then writes the records,
`ko`, the count `xn`, erases every record slot past the count, and writes
`cfg` last. Sweeping every slot means a save cut short before its count
lands leaves no record that a later save does not erase. On load the `cfg`
JSON wins wherever it carries the expressions or the knockout itself, so a
whole save cut short, or a config an older firmware rewrote after a
downgrade, loads as written. The dispositions JSON key wins over the blob the
same way. An edit rewrites one record and NVS skips the unchanged ones; a
remove shifts later records down a slot, so a reset mid-shift can leave one
record in two slots, and the load keeps the first of any repeated (type,
target). The app's edits are removes, edits and appends. A web save sends the
whole document and can reorder the list: saved whole that is one write and
reset-safe, but saved split a reset mid-save can leave an entry that moved in
two slots and drop the one it displaced. A record that fails to read drops only itself, logged as
`[cfg] expression record xN unreadable`. `test_config_records` resets the
store at every write of a remove, an add-and-edit, a first migration and a
switch between the two shapes. The boot log's
`[cfg] expressions=N from records|cfg json` is the loaded count; the
`cfg_bytes` on the line before is the `cfg` JSON alone.

**Downgrades.** On an older release a whole-saved lamp keeps everything; a
split-saved one loses its expressions there (that release reads only the
`cfg` JSON, which then has none), and dispositions reset there in either
case, since the older release reads only the JSON key the migration removed.
A parameter key the older release does not know (one appended to
`kExpressionParamKeys` later) is dropped there, and stays dropped if that
release saves. Flashed forward again, whatever the older release saved is
what loads.

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
  completes (up to 180 s). `admitTransient`
  (`expressions/transient_admission.hpp`) admits them: at most 4 in flight,
  at most 2 of them a peer's (cascades and event echoes), so the lamp's own
  Test and greetings always have room, and only when the largest free block
  holds 2 KB per target plus the headroom, so a transient that starts can
  finish allocating. A dropped one logs `[expr] transient drop` on every
  build, at most every 5 s. The vector is reserved at
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
  12 parameter keys; an expression upsert (app, or a relayed `expressionOp`)
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
- **ESP-NOW frames in the driver.** Each queued frame holds a WiFi TX buffer
  until it airs. Past the driver's queue, `esp_now_send` fails with
  `ESP_ERR_ESPNOW_NO_MEM` (0x3067), and on the bench a burst of those was
  followed minutes later by the TX path wedging with its buffers (about 8 KB)
  held and every broadcast failing. `TxCredit`
  (`components/network/transport/tx_credit.hpp`) bounds the frames handed to
  the driver and not yet reported by its send callback to 6: every submit
  takes a credit, the callback returns it, a refused submit returns it at
  once, and when credits are out and no completion has come back for 2 s
  they are presumed lost and the count resets. Timestamps compare by signed
  delta and never move backwards, since the WiFi task can stamp a completion
  just after the loop read `millis()`. A send past the bound fails before it
  reaches the driver. A reset logs `[espnow.tx] no send callback` on every
  build (at most every 10 s), and the running count shows as `txstalls` in
  `heap.get` and the `[heap]` trend line.
- **Cascade fan-out.** A locally fired cascade (a cascading expression, or the
  app's Test) reaches every mesh peer, closest first. `CascadeScheduler`
  (`expressions/cascade_scheduler.hpp`) holds up to two fan-outs (peers and one
  serialized invocation each, about 2.2 KB of .bss: 50 MACs and a 768-byte
  payload per fan-out, above the 536-byte worst case). The loop sends up to 6
  frames per pass (`kBatchSize`, held to the 6 TX credits), only while credits
  remain (a frame with none waits for a later pass), and passes are 100 ms
  apart, past a command's two resend copies
  40 ms apart, so a batch never overwrites the previous batch's resends in the
  10-slot ring. Each frame's `delayMs` is rewritten in place to keep the
  peer's intended fire time, (i + 1) x stagger capped at 1 s from the original
  fire, less the time the drain has taken, never below zero. Fifty peers take
  9 passes, so the last leaves at least 800 ms after the fire (later when relay
  traffic holds credits), and with a short stagger the far end of a 50-lamp
  crowd fires up to about a second late. A second cascade waits
  behind the one draining; a third replaces the one waiting. An OTA start
  stops the drain, and a frame whose delay rewrite would overflow is dropped
  and logged.
- **Reads nobody asked for.** The section caches warm, and the state notify
  builds its payload, only while a BLE client holds the link; with no client
  the caches are freed every tick and rebuild on the next read, and the tick
  after a client connects warms them while the heap allows. The
  expressions section (about 4 KB at the worst case) is measured, then written
  by `JsonWriter` into one exactly-sized block; the smaller sections build
  their document only with the headroom free. A heap check cannot reserve
  the block, so the cache assign and the copy-out also catch `bad_alloc`
  (best effort, with no exception emergency pool).
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
