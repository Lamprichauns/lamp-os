import 'dart:async';

import 'package:riverpod_annotation/riverpod_annotation.dart';

import '../../../core/ble/ble_scanner.dart';
import '../../../core/utils/clock.dart';
import '../../inventory/application/inventory_notifier.dart';
import '../../inventory/domain/lamp_presence.dart';
import '../domain/nearby_lamp.dart';
import 'ota_busy_seen_provider.dart';

part 'nearby_lamps_notifier.g.dart';

const _staleAfter = kCurrentlySeenWindow;

/// How often to sweep the roster and prune stale entries. Without it, a lamp
/// that goes silently out of range while no other lamp emits an adv would
/// stick in `state` forever (`_onAd` only rebuilds when an adv lands).
const _pruneInterval = Duration(seconds: 5);

/// Minimum step between a paired lamp's stored last-seen writes from the adv
/// stream. Exact times land when the lamp leaves the roster or the scan ends;
/// an app kill skips both, so a relaunch can list a lamp up to this much less
/// than its full listing window.
const _lastSeenWriteStep = Duration(seconds: 30);

/// Max rate `state` is re-emitted to consumers. Leading-edge: the first change
/// in a window emits immediately (a new lamp appears within ~16 ms); later
/// changes coalesce into one trailing emit. 1000 ms keeps a 22-lamp fleet's
/// ~44 advs/sec from rebuilding every roster watcher at 44 Hz.
const _stateEmitWindow = Duration(milliseconds: 1000);

// Scoped to consumers: when no screen watches `nearbyLampsNotifierProvider`
// the notifier auto-disposes and its `ref.onDispose` calls `scanner.stop()`,
// handing the scan budget back to Android.
@Riverpod(name: 'bleScannerProvider')
BleScanner bleScanner(Ref ref) => FbpBleScanner();

@Riverpod(name: 'nearbyLampsNotifierProvider')
class NearbyLampsNotifier extends _$NearbyLampsNotifier {
  StreamSubscription<BleAdvertisement>? _sub;
  Timer? _pruneTimer;
  late DateTime Function() _now;
  late InventoryNotifier _inv;
  // The most current roster, pending or emitted.
  List<NearbyLamp> _latest = const [];
  // Latest computed roster waiting to be flushed to `state`. Null when
  // nothing is pending. Used by the leading-edge emit throttle.
  List<NearbyLamp>? _pendingRoster;
  // True while the throttle window is open: accumulate into `_pendingRoster`
  // instead of emitting. The trailing-edge timer flushes when it closes.
  Timer? _emitWindowTimer;

  @override
  List<NearbyLamp> build() {
    _now = ref.read(clockProvider);
    _inv = ref.read(inventoryNotifierProvider.notifier);
    final scanner = ref.read(bleScannerProvider);
    scanner.start();
    _sub = scanner.results().listen(_onAd);
    ref.onDispose(() {
      _recordLastSeen(_latest);
      _sub?.cancel();
      _pruneTimer?.cancel();
      _pruneTimer = null;
      _emitWindowTimer?.cancel();
      _emitWindowTimer = null;
      _pendingRoster = null;
      scanner.stop();
    });
    return const [];
  }

  void _onAd(BleAdvertisement ad) {
    final now = _now().millisecondsSinceEpoch;
    // Carry the OTA-distributing bit into the keepAlive tracker so the busy
    // state survives navigation into the lamp's screen (this scanner is
    // scoped to the picker and tears down on the way there).
    ref
        .read(otaBusySeenProvider.notifier)
        .observe(ad.id, distributing: ad.otaDistributing);
    final updated = NearbyLamp(
      id: ad.id,
      name: ad.name,
      rssi: ad.rssi,
      serviceUuids: ad.serviceUuids,
      baseRgb: ad.baseRgb,
      shadeRgb: ad.shadeRgb,
      lastSeenEpochMs: now,
      isMesh: ad.isMesh,
      configured: ad.configured,
      otaDistributing: ad.otaDistributing,
    );
    // Build the next-roster against whatever's MOST CURRENT: either the
    // pending list waiting to flush, or the live state if no window is
    // open. Without checking _pendingRoster here, a burst of advs in a
    // single window would each rebuild against stale `state`.
    final base = _pendingRoster ?? state;
    final next = [
      ..._withoutStale(base.where((l) => l.id != ad.id), now),
      updated,
    ];
    _sortRoster(next);
    _scheduleStateEmit(next);
    // Lazy-start the prune timer on first adv. Widget tests that don't pipe
    // through the scanner never create a timer, so they don't trip the test
    // framework's `!timersPending` check.
    _pruneTimer ??= Timer.periodic(_pruneInterval, (_) => _prune());
    _trackPairedLamp(ad, now);
  }

  /// Leading-edge emit throttle. Without it, every adv on a 22-lamp fleet
  /// (~44 Hz) reassigned `state` and rebuilt every consumer not using
  /// `.select()`. First adv in a window emits immediately; later advs land in
  /// `_pendingRoster` and flush when the window closes (skipped if unchanged).
  void _scheduleStateEmit(List<NearbyLamp> next) {
    _latest = next;
    if (_emitWindowTimer == null) {
      // Leading edge: emit + open the window.
      state = next;
      _pendingRoster = null;
      _emitWindowTimer = Timer(_stateEmitWindow, _onEmitWindowClose);
    } else {
      // Inside the window: stash the latest roster; flush when it closes.
      _pendingRoster = next;
    }
  }

  void _onEmitWindowClose() {
    _emitWindowTimer = null;
    final pending = _pendingRoster;
    if (pending == null) return;
    _pendingRoster = null;
    // In steady state every adv mutates rssi, so every flush would emit and
    // rebuild every unselect-watching consumer. NearbyLamp is @freezed (== on
    // all fields); skip the assignment when the pending roster already matches.
    if (_listEquals(pending, state)) return;
    state = pending;
  }

  /// Stable roster order: bucket by RSSI tier (20 dBm bands so typical ±5 dBm
  /// jitter doesn't flip neighbours at a boundary), then alphabetical by name
  /// within a bucket. Name beats recency/raw-RSSI as the tiebreaker because
  /// those re-flip on every adv, defeating stability. 20 dBm collapses "same
  /// room or one wall over" into one bucket; proximity ordering still
  /// separates far-end-of-house peers.
  static void _sortRoster(List<NearbyLamp> list) {
    list.sort((a, b) {
      // RSSI is negative; less-negative = closer. Bucket by 20 dBm,
      // sort DESC so bucket -3 (-60..-79 dBm) precedes bucket -4
      // (-80..-99 dBm) etc.
      final bucketA = a.rssi ~/ 20;
      final bucketB = b.rssi ~/ 20;
      if (bucketA != bucketB) return bucketB.compareTo(bucketA);
      return a.name.toLowerCase().compareTo(b.name.toLowerCase());
    });
  }

  /// Lightweight content equality for the roster. Lists of @freezed
  /// NearbyLamp; element `==` checks every field.
  static bool _listEquals(List<NearbyLamp> a, List<NearbyLamp> b) {
    if (identical(a, b)) return true;
    if (a.length != b.length) return false;
    for (var i = 0; i < a.length; i++) {
      if (a[i] != b[i]) return false;
    }
    return true;
  }

  /// Keeps a paired lamp's inventory entry current from its adv: its stored
  /// last-seen time and mesh capability, or its id when the platform id
  /// changed.
  void _trackPairedLamp(BleAdvertisement ad, int nowMs) {
    // Use the adv fields directly; the adv may have landed in `_pendingRoster`
    // but not yet `state` during the emit throttle.
    final deviceId = ad.id;
    final inv = ref.read(inventoryNotifierProvider).value;
    if (inv == null) return;
    final isPaired = inv.any((l) => l.id == deviceId);
    if (!isPaired) {
      // Cross-platform / iOS-reinstall reconciliation: the new id
      // doesn't match any inventory entry. Check whether the adv's
      // name + colors uniquely identify a paired lamp under an older
      // platform id. If so, the inventory entry's id is updated in
      // place and subsequent advs hit the fast `isPaired = true`
      // branch above.
      unawaited(_inv.reconcileIdByIdentity(
        newId: deviceId,
        name: ad.name,
        baseRgb: ad.baseRgb,
        shadeRgb: ad.shadeRgb,
      ));
      return;
    }
    unawaited(_inv.recordLastSeen(
      {deviceId: nowMs},
      minStep: _lastSeenWriteStep,
    ));
    // Cache the observed mesh capability so the route resolver can
    // route correctly even when the lamp is out of range. Self-caps
    // at "only writes when the value changes" so this is also safe
    // to call on every adv.
    unawaited(_inv.rememberMeshState(deviceId, isMesh: ad.isMesh));
  }

  /// [lamps] heard within [_staleAfter] of [nowMs]. A lamp aging out has its
  /// exact last-seen time recorded, since the adv-stream writes are capped.
  List<NearbyLamp> _withoutStale(Iterable<NearbyLamp> lamps, int nowMs) {
    final kept = <NearbyLamp>[];
    final stale = <NearbyLamp>[];
    for (final l in lamps) {
      (nowMs - l.lastSeenEpochMs < _staleAfter.inMilliseconds ? kept : stale)
          .add(l);
    }
    _recordLastSeen(stale);
    return kept;
  }

  void _recordLastSeen(List<NearbyLamp> lamps) {
    if (lamps.isEmpty) return;
    unawaited(_inv.recordLastSeen({
      for (final l in lamps) l.id: l.lastSeenEpochMs,
    }));
  }

  /// Drop stale entries from `state`. Called both implicitly by
  /// `_onAd` (which filters during its rebuild) and on a 5-second
  /// periodic timer so lamps that disappear silently still get
  /// pruned without waiting for another lamp's adv.
  void _prune() {
    final now = _now().millisecondsSinceEpoch;
    final base = _pendingRoster ?? state;
    final next = _withoutStale(base, now);
    if (next.length != base.length) {
      _scheduleStateEmit(next);
    }
  }
}
