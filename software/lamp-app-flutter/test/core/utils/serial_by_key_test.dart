import 'dart:async';

import 'package:flutter_test/flutter_test.dart';
import 'package:lamp_app/core/utils/serial_by_key.dart';

void main() {
  test('runs one key\'s operations one at a time in call order', () async {
    final serial = SerialByKey();
    final log = <String>[];
    final first = Completer<void>();
    final a = serial.run('x', () async {
      log.add('a start');
      await first.future;
      log.add('a end');
    });
    final b = serial.run('x', () async => log.add('b'));
    await Future<void>.delayed(Duration.zero);
    expect(log, ['a start']);
    first.complete();
    await a;
    await b;
    expect(log, ['a start', 'a end', 'b']);
  });

  test('different keys do not wait on each other', () async {
    final serial = SerialByKey();
    final held = Completer<void>();
    unawaited(serial.run('x', () => held.future));
    var ran = false;
    await serial.run('y', () async => ran = true);
    expect(ran, isTrue);
    held.complete();
  });

  test('a failure reaches only its caller and the key keeps running', () async {
    final serial = SerialByKey();
    final failed = serial.run<void>('x', () async => throw StateError('boom'));
    final next = serial.run('x', () async => 42);
    await expectLater(failed, throwsStateError);
    expect(await next, 42);
    expect(await serial.run('x', () async => 7), 7);
  });
}
