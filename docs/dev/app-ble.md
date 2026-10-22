# App BLE links

How the Flutter app opens and holds BLE links to lamps, how a connect shares the
phone's radio with the scan, and how My Lamps decides which paired lamps to
list. Source: `software/lamp-app-flutter/lib/core/ble/` and the inventory,
nearby and control features. The GATT layout and payloads the link carries are
in [`networking.md`](networking.md).

The app links only to a lamp the user opens (or is onboarding). My Lamps never
connects on its own; it lists lamps from the scan and from stored last-seen
times.

## Connect retry: `connectWithRetry`

`connectWithRetry` (`core/ble/connect_with_retry.dart`) is the app's one connect
policy. Android intermittently fails a first connect in a crowded BLE
environment with GATT_ERROR(133) or a `deviceIsDisconnected` race, and its stack
needs about a second to release the slot after a failed attempt.

- Makes at most `attempts` tries (default 4) with `backoff` between them
  (default 1.2 s).
- Checks `shouldAbort` before each attempt and after a successful one, and
  returns false once it is true. Returns true once connected.
- Rethrows the last attempt's failure, so the caller maps it to a user-visible
  error.

Callers:

| Caller | Policy |
|---|---|
| `ControlNotifier.build` (opening a lamp) | defaults, aborts when the notifier is unmounted |
| `ControlNotifier._tryReconnect` (the reconnect ladder) | `attempts: 1` per ladder step |
| `ControlNotifier._awaitReconnectAndReload` (after a settings save reboots the lamp) | `attempts: 1` per poll |
| `AddLampNotifier.submit` (onboarding) | `attempts: 2`, 1.5 s backoff |
| `AdoptPulseController.start` (onboarding pulse) | defaults, aborts on `stop` |

`AddLampNotifier._reconnectWithRetry`, the reconnect after an adopted lamp
reboots, is a separate policy: it drops the stale handle, waits out the reboot
and calls `BleClient.connect` directly with a per-attempt timeout.

## Link ownership: `claimLink` / `ownsLink`

A connect can outlive the screen that started it: the user backs out while an
attempt is still in flight, and the link lands afterwards. `BleClient` keeps
one owner per device id so the right party decides whether that late link
stays.

- `BleClient.claimLink(deviceId, owner)` makes `owner` the party that answers
  for the lamp's link. The latest claim wins.
- `BleClient.ownsLink(deviceId, owner)` is true while `owner` holds the latest
  claim.

Who claims:

- `connectWithRetry` claims the lamp for its `owner` on entry.
- `ControlNotifier.build` claims the lamp for its session before it checks
  `isConnected`, so it also claims a link that is already up and adopts it.
  The abandoned connect that raised that link may not have reached its abort
  check yet.

What it guarantees: when `connectWithRetry` lands a link after its owner gave
up (`shouldAbort` true), it disconnects only while that owner still holds the
claim. Backing out of a lamp mid-connect and opening it again keeps the new
session's link, whichever of the two connects lands it. A connect abandoned
with nobody claiming the lamp since still drops its link, so a lamp the user
walked away from is not held open.

A caller that connects through `connectWithRetry` must pass an owner that is
stable for its session (`ControlNotifier` uses one `_linkOwner` object per
notifier) and must claim before adopting a live link it did not connect itself.

## Scan hold during a connect

The phone has one BLE scan, and a running scan competes with a connect
handshake for the radio. Neither flutter_blue_plus platform plugin pauses its
own scan around a connect, so the app does:

- `scanPause` (`core/ble/scan_pause.dart`) is the one shared `ScanPause`. Its
  `during` holds scanning off for the body's duration; holds nest, and
  `released` fires when the last one ends, whether the body completed or
  threw.
- `FbpBleClient.connect` (`core/ble/fbp_ble_client.dart`) stops a running scan
  and runs flutter_blue_plus's `BluetoothDevice.connect` inside
  `scanPause.during`, with an 8 s connect timeout. Connection priority and
  service discovery run after the hold ends.
- `FbpBleScanner` (`core/ble/ble_scanner.dart`) never starts a scan while
  `scanPause.isPaused`. On `released` it restarts through `_restartScan`, which
  waits `_restartBackoff` (3 s) first, and stops a scan that started while a
  new hold was taken.

How long the scan stays off:

- **One attempt that fails to connect** holds the scan for up to its 8 s
  timeout. The scan comes back `_restartBackoff` (3 s) after the hold ends, so
  about 11 s at most.
- **A connect that fails every attempt** keeps the scan off for the whole run:
  the 1.2 s backoff between attempts is shorter than the scanner's 3 s restart
  backoff, so the scan never restarts in between. With `connectWithRetry`'s
  defaults that is 4 x 8 s + 3 x 1.2 s + the 3 s restart backoff, about 39 s.
- **A connect that lands on Android** also runs flutter_blue_plus's MTU request
  inside the hold. In flutter_blue_plus 2.3.4, `BluetoothDevice.connect`
  defaults `mtu` to 512 and calls `requestMtu` after the link is up;
  `requestMtu` waits a 0.35 s predelay and times out after 15 s. That request
  is outside the 8 s connect timeout, so an attempt whose link comes up and
  whose MTU request then stalls holds the scan for up to 8 s + 0.35 s + 15 s.
  A failed MTU request throws out of `connect` and counts as a failed attempt.

While the scan is off, the app hears no lamp. A connect can run with My Lamps
on screen (My Lamps opened over a lamp that is still connecting or
reconnecting, or an abandoned attempt still in flight), and during a long hold
lamps there can show offline until the scan comes back.

## My Lamps presence

`lampPresence` (`features/inventory/domain/lamp_presence.dart`) places each
paired lamp from its last sighting, the later of the stored
`InventoryLamp.lastSeenEpochMs` and the live scan roster's time:

| Last sighting | Presence | On My Lamps |
|---|---|---|
| connected, any age | `reachable` | listed, online |
| under `kCurrentlySeenWindow` (30 s) | `reachable` | listed, online |
| up to `kUnreachableListingWindow` (5 min) | `unreachable` | listed, offline |
| older, or never seen | `hidden` | not listed, still paired |

A hidden lamp keeps its inventory entry and comes back to the list as soon as
it is heard again.

`kCurrentlySeenWindow` is sized against the scan holds above. Lamps advertise
every 100-200 ms (`BLE_ADVERTISING_INTERVAL_MIN` / `_MAX` in the firmware's
`ble_gap.hpp`), so an in-range lamp is heard many times inside 30 s. The window
rides out one failed attempt's hold plus the restart backoff (about 11 s). It
does not ride out a connect that fails every attempt (about 39 s). The nearby
roster ages lamps out on the same window (`_staleAfter` in
`nearby_lamps_notifier.dart`).

## Last-seen stamps

`lastSeenEpochMs` on each `InventoryLamp` survives an app kill and decides
presence after a relaunch. Two `InventoryNotifier` methods write it:
`recordLastSeen`, which writes an entry only when it is newer than the stored
one by at least `minStep`, and `updateSeen`, which writes the time together
with the lamp's cached colours.

**Lamps the scan hears** (`NearbyLampsNotifier`):

- Each advertisement from a paired lamp calls `recordLastSeen` with
  `minStep: _lastSeenWriteStep` (30 s), which caps disk writes.
- A lamp aging out of the roster has its exact last advertisement time
  recorded, and so does every lamp still in the roster when the scanner is
  disposed (leaving My Lamps).
- An app kill skips both, so it loses at most `_lastSeenWriteStep` of a heard
  lamp's last-seen time.

**A lamp the phone holds open** is not heard by the scan: the lamp takes one
BLE connection (`CONFIG_BT_NIMBLE_MAX_CONNECTIONS=1`) and stops advertising
while it is held. `ControlNotifier` stamps it instead:

- on connect, through `_updateSeen` once the session's sections are read;
- on colour edits, through `_queueSeen`, coalesced into one write 500 ms
  after the last edit;
- when the link drops, in `_onConnectionChange` (`_recordSeenNow`);
- when the lamp is closed while still linked, in the session's dispose
  (`_recordSeenNow`);
- when the app goes to the background, on `AppLifecycleState.paused`
  (`_recordSeenNow`, while linked).

The background stamp is what keeps an open lamp listed after the app is killed:
a kill from the background follows `paused`, so a relaunch finds a recent
last-seen time for the lamp the phone held moments before.
