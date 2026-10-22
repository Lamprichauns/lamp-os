/// A lamp heard within this long counts as currently seen. Lamps advertise
/// every 100-200 ms, so an in-range lamp is heard many times inside it. The
/// window rides out a single connect attempt's scan hold of up to 8 s plus the
/// scanner's 3 s restart backoff. A connect that fails attempt after attempt
/// holds the scan off for up to about 39 s, and lamps on a visible My Lamps
/// can show offline until it ends.
const kCurrentlySeenWindow = Duration(seconds: 30);

/// My Lamps keeps listing a lamp as unreachable for this long after the phone
/// last saw it, then hides it until it is seen again.
const kUnreachableListingWindow = Duration(minutes: 5);

enum LampPresence { reachable, unreachable, hidden }

/// Where a paired lamp stands on My Lamps at [now]. A connected lamp is
/// reachable whatever its last sighting; a lamp never seen is hidden.
LampPresence lampPresence({
  required int? lastSeenEpochMs,
  required bool connected,
  required DateTime now,
}) {
  if (connected) return LampPresence.reachable;
  if (lastSeenEpochMs == null) return LampPresence.hidden;
  final age = now.millisecondsSinceEpoch - lastSeenEpochMs;
  if (age < kCurrentlySeenWindow.inMilliseconds) return LampPresence.reachable;
  if (age <= kUnreachableListingWindow.inMilliseconds) {
    return LampPresence.unreachable;
  }
  return LampPresence.hidden;
}
