import 'dart:async';

import 'package:flutter_test/flutter_test.dart';
import 'package:lamp_app/core/ble/scan_pause.dart';

void main() {
  test('pauses for the body and releases once it completes', () async {
    final pause = ScanPause();
    var releases = 0;
    pause.released.listen((_) => releases++);
    bool? pausedInside;
    final out = await pause.during(() async {
      pausedInside = pause.isPaused;
      return 7;
    });
    expect(out, 7);
    expect(pausedInside, isTrue);
    expect(pause.isPaused, isFalse);
    expect(releases, 1);
  });

  test('releases when the body throws, and the error reaches the caller',
      () async {
    final pause = ScanPause();
    var releases = 0;
    pause.released.listen((_) => releases++);
    await expectLater(
      pause.during<void>(() async => throw StateError('connect failed')),
      throwsStateError,
    );
    expect(pause.isPaused, isFalse);
    expect(releases, 1);
  });

  test('overlapping holds release only when the last one ends', () async {
    final pause = ScanPause();
    var releases = 0;
    pause.released.listen((_) => releases++);
    final first = Completer<void>();
    final second = Completer<void>();
    final a = pause.during(() => first.future);
    final b = pause.during(() => second.future);
    first.complete();
    await a;
    expect(pause.isPaused, isTrue);
    expect(releases, 0);
    second.complete();
    await b;
    expect(pause.isPaused, isFalse);
    expect(releases, 1);
  });
}
