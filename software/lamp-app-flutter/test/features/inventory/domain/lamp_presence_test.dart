import 'package:flutter_test/flutter_test.dart';
import 'package:lamp_app/features/inventory/domain/lamp_presence.dart';

void main() {
  final now = DateTime.fromMillisecondsSinceEpoch(1700000000000);
  int seenAgo(Duration d) => now.subtract(d).millisecondsSinceEpoch;
  LampPresence at(Duration ago) =>
      lampPresence(lastSeenEpochMs: seenAgo(ago), connected: false, now: now);
  const ms = Duration(milliseconds: 1);

  test('the windows nest: currently-seen ends before the listing does', () {
    expect(kCurrentlySeenWindow, lessThan(kUnreachableListingWindow));
  });

  test('reachable while seen inside the currently-seen window', () {
    expect(at(Duration.zero), LampPresence.reachable);
    expect(at(kCurrentlySeenWindow - ms), LampPresence.reachable);
  });

  test('unreachable from the window until the listing window ends', () {
    expect(at(kCurrentlySeenWindow), LampPresence.unreachable);
    expect(at(kUnreachableListingWindow), LampPresence.unreachable);
  });

  test('hidden once last seen longer ago than the listing window', () {
    expect(at(kUnreachableListingWindow + ms), LampPresence.hidden);
    expect(at(const Duration(days: 3)), LampPresence.hidden);
  });

  test('a never-seen lamp is hidden', () {
    expect(
      lampPresence(lastSeenEpochMs: null, connected: false, now: now),
      LampPresence.hidden,
    );
  });

  test('a connected lamp is reachable however long ago it was last heard', () {
    expect(
      lampPresence(
        lastSeenEpochMs: seenAgo(kUnreachableListingWindow + ms),
        connected: true,
        now: now,
      ),
      LampPresence.reachable,
    );
    expect(
      lampPresence(lastSeenEpochMs: null, connected: true, now: now),
      LampPresence.reachable,
    );
  });
}
