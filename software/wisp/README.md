# Wisp

A small magical light that signals to others. Wisp is dedicated infrastructure (Seeed Xiao ESP32-C6 with a remote antenna) — the rebuilt identity of the old `software/artnet-repeater/` board. It is not a lamp; it is a non-participating helper that sits at the edge of the mesh.

## Three jobs

1. **Palette bridge.** Follows a palette from Straylight over OSC 1.0 (SLIP frames on the USB serial console, or UDP port 9000 on the softAP or its STA LAN join) and fans out per-lamp paired colors to the mesh over ESP-NOW. The app picks which Straylight palette role it follows (primary, secondary, tertiary or fill); the address is `/straylight/palette/<role>/colours`, flat RGB floats 0..1.
2. **Firmware carrier.** Holds the latest signed lamp firmware blob and pushes it out to any lamp on the mesh running an older version (single-peer-at-a-time, signed with ed25519, channel-matched). Wisp itself is flashed via USB; it never self-updates.
3. **ArtNet bridge.** Paints pre-mesh lamps, which have no mesh stack and take color only as ArtNet DMX over WiFi. This is the reason wisp runs a softAP and a BLE stage beacon at all — see below.

## Painting pre-mesh lamps

Pre-mesh lamp firmware joins a WiFi network whose credentials it finds in a non-connectable BLE advert (magic `42007`, payload `<ssid\0><password\0>`), then listens for ArtNet DMX on UDP/6454. So wisp brings up a softAP on the mesh channel in `WIFI_AP_STA` (`main.cpp`), advertises those credentials via `StageBeacon`, and emits universe-1 frames from `src/artnet/`.

One frame carries 8 fixtures of 10 channels (shade RGBW, base RGBW, mode, parameter). A pre-mesh lamp picks its fixture slot at random on boot, so assignment is neither stable nor unique — unlike the mesh path, which keys each lamp's colors on its real MAC. Wisp fills the 8 slots by running synthetic MACs through the same `TupleSampler` the mesh path uses, so both fleets sample one palette. Base and shade are swapped into the frame to match `PaintDistributor`'s surface mapping; without that a mixed room renders the two fleets inverted.

Emission is ~1 Hz (a faster stream would steal TX airtime from ESP-NOW RX on the shared radio) and unicasts to each joined station in AP mode, because WiFi broadcast has no MAC-layer ack. Turning paint off just stops the frames: pre-mesh lamps hit their 30 s ArtNet timeout and fade back to their own scene.

## Off-by-default paint

Status reporting, mesh-inventory tracking, and firmware push all run unconditionally at boot. The OSC UDP listener is bound, and the WiFi STA join to a stored network runs, only while the source mode is Osc; each attempt scans the mesh channel only, joins only if the network is there, and backs off (10 s doubling to 5 min) when it isn't, so the radio stays on the mesh. **Choosing Manual or Straylight in the app is the opt-in to paint, and the choice is stored.** A fresh wisp starts Off and paints nothing until someone picks a source. After a reboot or power blip the stored source comes back and painting resumes: Manual repaints its stored palette, Straylight resumes as soon as a palette arrives. Off comes back as Off.

In Osc mode a stream gap is not an opt-out: after 30 s with nothing from Straylight (or as soon as Straylight sends a blank one for an unbound role) the wisp lets lamps ease home and shows its off color, but stays in Osc mode, and the next palette resumes painting with no app action.

Prefer USB serial for the OSC feed. Over UDP the wisp hears Straylight's whole feed, not just its palette, on the radio it shares with the mesh, and a restate burst can overflow lwIP's 6-datagram socket queue and drop the palette, possibly on every restate, so a change lost that way waits for the next change.

Lamp overrides are transient (RAM-only on the lamp side). When wisp drops or paint mode goes off, lamps re-render their saved base config. The user's stored config is never touched.

## How the user talks to it

Wisp has no BLE. The app talks to wisp via any nearby lamp acting as a proxy: the lamp surfaces wisp's last-overheard `MSG_WISP_HELLO` on a BLE characteristic, and forwards ops back to wisp over the mesh. WiFi credentials are configured the same way — out of the box, flash wisp once over USB and configure it from the app.

## Wire format

Wisp/lamp message types (`MSG_WISP_HELLO`, `MSG_WISP_PALETTE`, `MSG_OVERRIDE_COLORS`/`MSG_RESTORE_COLORS`, `MSG_CONTROL_OP` wisp envelopes) and the OTA exchange (`MSG_FW_OFFER` … `MSG_FW_RESULT`) are documented in [`docs/dev/networking.md`](../../docs/dev/networking.md). LSIG signed-firmware footer format lives alongside the build scripts at [`scripts/README.md`](../../scripts/README.md).
