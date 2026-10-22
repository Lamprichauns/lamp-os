import '../../../core/widgets/status_dot.dart';
import '../../inventory/domain/lamp_presence.dart';
import '../../nearby/domain/nearby_lamp.dart';

/// Derives a [StatusKind] for a lamp at the current moment.
///
/// - `otaReceiving`: the lamp is learning a new image (the phone is pushing to
///   it, or a mesh peer is). Wins over `mesh`; the BLE link is held for a push.
/// - `otaSending`: the lamp is teaching a peer (sourcing firmware over mesh).
/// - `mesh`: the active connection right now. Pulses; the app is actually
///   reading + writing this lamp.
/// - `bluetooth`: heard via BLE adv within the nearby staleness window but
///   not the one connected (or connection is currently dropped).
/// - `searching`: not heard yet but the BLE scanner just started; the
///   caller passes `inScanGrace=true` while the grace window is open.
/// - `offline`: not heard, scan grace has expired.
StatusKind statusFor({
  required String lampId,
  required List<NearbyLamp> nearby,
  required bool connected,
  bool receiving = false,
  bool sending = false,
  bool inScanGrace = false,
}) {
  if (receiving) return StatusKind.otaReceiving;
  if (sending) return StatusKind.otaSending;
  if (connected) return StatusKind.mesh;
  NearbyLamp? hit;
  for (final l in nearby) {
    if (l.id == lampId) {
      hit = l;
      break;
    }
  }
  return _kindFromHit(hit, inScanGrace);
}

/// My Lamps' status for a paired lamp in [presence]. [isMesh] is the lamp's
/// last-known mesh capability, used while it is reachable.
StatusKind statusForPresence({
  required LampPresence presence,
  required bool connected,
  required bool isMesh,
  bool receiving = false,
  bool sending = false,
  bool inScanGrace = false,
}) {
  if (receiving) return StatusKind.otaReceiving;
  if (sending) return StatusKind.otaSending;
  if (connected) return StatusKind.mesh;
  if (presence == LampPresence.reachable) {
    return isMesh ? StatusKind.mesh : StatusKind.bluetooth;
  }
  return inScanGrace ? StatusKind.searching : StatusKind.offline;
}

StatusKind _kindFromHit(NearbyLamp? hit, bool inScanGrace) {
  if (hit == null) {
    return inScanGrace ? StatusKind.searching : StatusKind.offline;
  }
  // `isMesh` = "firmware speaks the app's mesh protocol". Lamps with
  // the current build show as `mesh` whenever in range; legacy v1
  // firmware (BT-only) shows as `bluetooth`.
  return hit.isMesh ? StatusKind.mesh : StatusKind.bluetooth;
}
