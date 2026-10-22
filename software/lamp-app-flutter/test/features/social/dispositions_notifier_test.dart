// The dispositions map is written back whole, so a read the lamp could not
// answer ("!") leaves it unwritable: an edit after it must not reach the
// lamp, where it would replace every stored disposition.

import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';

import '../../_support/in_memory_ble_client.dart';
import 'package:lamp_app/core/ble/ble_client_provider.dart';
import 'package:lamp_app/core/ble/uuids.dart';
import 'package:lamp_app/features/social/application/dispositions_notifier.dart';

Future<String> _stored(InMemoryBleClient ble, String id) async => utf8.decode(
    await ble.read(id, BleUuids.controlService, BleUuids.socialDispositions));

Future<InMemoryBleClient> _lampAnswering(String id, String answer) async {
  final ble = InMemoryBleClient();
  await ble.connect(id);
  await ble.write(id, BleUuids.controlService, BleUuids.socialDispositions,
      Uint8List.fromList(utf8.encode(answer)));
  return ble;
}

void main() {
  test('an edit after an unreadable map is not written', () async {
    final ble = await _lampAnswering('lamp', '!');
    final container =
        ProviderContainer(overrides: [bleClientProvider.overrideWithValue(ble)]);
    addTearDown(container.dispose);
    final sub = container.listen(dispositionsProvider('lamp'), (_, _) {});
    addTearDown(sub.close);
    await container.read(dispositionsProvider('lamp').future);
    container.read(dispositionsProvider('lamp').notifier).set('AA:BB:CC:DD:EE:FF', 5);
    await Future<void>.delayed(const Duration(milliseconds: 700));
    expect(await _stored(ble, 'lamp'), '!');
  });

  test('an edit after an empty read is not written', () async {
    final ble = await _lampAnswering('lamp', '');
    final container =
        ProviderContainer(overrides: [bleClientProvider.overrideWithValue(ble)]);
    addTearDown(container.dispose);
    final sub = container.listen(dispositionsProvider('lamp'), (_, _) {});
    addTearDown(sub.close);
    await container.read(dispositionsProvider('lamp').future);
    container.read(dispositionsProvider('lamp').notifier).set('AA:BB:CC:DD:EE:FF', 5);
    await Future<void>.delayed(const Duration(milliseconds: 700));
    expect(await _stored(ble, 'lamp'), '');
  });

  test('an edit after a null map (released firmware, no entries) is written', () async {
    final ble = await _lampAnswering('lamp', 'null');
    final container =
        ProviderContainer(overrides: [bleClientProvider.overrideWithValue(ble)]);
    addTearDown(container.dispose);
    final sub = container.listen(dispositionsProvider('lamp'), (_, _) {});
    addTearDown(sub.close);
    await container.read(dispositionsProvider('lamp').future);
    container.read(dispositionsProvider('lamp').notifier).set('AA:BB:CC:DD:EE:FF', 5);
    await Future<void>.delayed(const Duration(milliseconds: 700));
    expect(jsonDecode(await _stored(ble, 'lamp')), {'AA:BB:CC:DD:EE:FF': 5});
  });

  test('an edit after a read map is written whole', () async {
    final ble = await _lampAnswering('lamp', '{"11:22:33:44:55:66":2}');
    final container =
        ProviderContainer(overrides: [bleClientProvider.overrideWithValue(ble)]);
    addTearDown(container.dispose);
    final sub = container.listen(dispositionsProvider('lamp'), (_, _) {});
    addTearDown(sub.close);
    await container.read(dispositionsProvider('lamp').future);
    container.read(dispositionsProvider('lamp').notifier).set('AA:BB:CC:DD:EE:FF', 5);
    await Future<void>.delayed(const Duration(milliseconds: 700));
    expect(jsonDecode(await _stored(ble, 'lamp')),
        {'11:22:33:44:55:66': 2, 'AA:BB:CC:DD:EE:FF': 5});
  });
}
