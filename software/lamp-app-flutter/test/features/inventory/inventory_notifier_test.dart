import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:lamp_app/features/inventory/application/inventory_notifier.dart';
import 'package:lamp_app/features/inventory/domain/inventory_lamp.dart';
import 'package:shared_preferences/shared_preferences.dart';

void main() {
  setUp(() => SharedPreferences.setMockInitialValues({}));

  test('empty store yields empty inventory', () async {
    final container = ProviderContainer();
    addTearDown(container.dispose);
    final list = await container.read(inventoryNotifierProvider.future);
    expect(list, isEmpty);
  });

  test('add then load returns the lamp', () async {
    final container = ProviderContainer();
    addTearDown(container.dispose);
    await container.read(inventoryNotifierProvider.future);
    await container
        .read(inventoryNotifierProvider.notifier)
        .add(const InventoryLamp(
          id: 'aa:bb:cc:dd:ee:ff',
          name: 'jacko',
        ));
    final list = await container.read(inventoryNotifierProvider.future);
    expect(list, hasLength(1));
    expect(list.first.name, 'jacko');
  });

  test('add survives a fresh container (persisted to prefs)', () async {
    final c1 = ProviderContainer();
    await c1.read(inventoryNotifierProvider.future);
    await c1
        .read(inventoryNotifierProvider.notifier)
        .add(const InventoryLamp(id: 'aa', name: 'a'));
    c1.dispose();

    final c2 = ProviderContainer();
    addTearDown(c2.dispose);
    final list = await c2.read(inventoryNotifierProvider.future);
    expect(list, hasLength(1));
  });

  test('remove drops by id', () async {
    final container = ProviderContainer();
    addTearDown(container.dispose);
    await container.read(inventoryNotifierProvider.future);
    final notifier = container.read(inventoryNotifierProvider.notifier);
    await notifier.add(const InventoryLamp(id: '1', name: 'a'));
    await notifier.add(const InventoryLamp(id: '2', name: 'b'));
    await notifier.remove('1');
    final list = await container.read(inventoryNotifierProvider.future);
    expect(list.map((l) => l.id).toList(), ['2']);
  });

  test('updateName changes the lamp name and persists', () async {
    final c1 = ProviderContainer();
    await c1.read(inventoryNotifierProvider.future);
    final n1 = c1.read(inventoryNotifierProvider.notifier);
    await n1.add(const InventoryLamp(id: 'aa', name: 'original'));
    await n1.updateName('aa', 'renamed');
    final after = await c1.read(inventoryNotifierProvider.future);
    expect(after.single.name, 'renamed');
    c1.dispose();

    // Survives container teardown — actually went to prefs.
    final c2 = ProviderContainer();
    addTearDown(c2.dispose);
    final list = await c2.read(inventoryNotifierProvider.future);
    expect(list.single.name, 'renamed');
  });

  test('updateName is a no-op for unknown ids', () async {
    final container = ProviderContainer();
    addTearDown(container.dispose);
    await container.read(inventoryNotifierProvider.future);
    final notifier = container.read(inventoryNotifierProvider.notifier);
    await notifier.add(const InventoryLamp(id: 'aa', name: 'alpha'));
    await notifier.updateName('bb', 'beta');
    final list = await container.read(inventoryNotifierProvider.future);
    expect(list.single.name, 'alpha');
  });

  test('corrupt inventory JSON recovers to empty instead of erroring',
      () async {
    SharedPreferences.setMockInitialValues({'inventory.v1': 'not-json{{{'});
    final container = ProviderContainer();
    addTearDown(container.dispose);
    final list = await container.read(inventoryNotifierProvider.future);
    expect(list, isEmpty);
  });

  test('concurrent mutations both persist (no read-modify-write clobber)',
      () async {
    final container = ProviderContainer();
    addTearDown(container.dispose);
    await container.read(inventoryNotifierProvider.future);
    final n = container.read(inventoryNotifierProvider.notifier);
    await n.add(const InventoryLamp(id: '1', name: 'a'));
    await n.add(const InventoryLamp(id: '2', name: 'b'));
    // Fire both without awaiting between: unserialized, each reads the same
    // pre-mutation snapshot and the second persist clobbers the first.
    final f1 = n.updateName('1', 'a2');
    final f2 = n.updateName('2', 'b2');
    await Future.wait([f1, f2]);
    final list = await container.read(inventoryNotifierProvider.future);
    final byId = {for (final l in list) l.id: l.name};
    expect(byId['1'], 'a2');
    expect(byId['2'], 'b2');
  });

  group('recordLastSeen', () {
    Future<ProviderContainer> withLamps(List<InventoryLamp> lamps) async {
      final c = ProviderContainer();
      addTearDown(c.dispose);
      await c.read(inventoryNotifierProvider.future);
      for (final l in lamps) {
        await c.read(inventoryNotifierProvider.notifier).add(l);
      }
      return c;
    }

    int? seenOf(ProviderContainer c, String id) => c
        .read(inventoryNotifierProvider)
        .value!
        .firstWhere((l) => l.id == id)
        .lastSeenEpochMs;

    test('writes newer times, keeps older ones, ignores unpaired ids',
        () async {
      final c = await withLamps(const [
        InventoryLamp(id: 'a', name: 'a', lastSeenEpochMs: 5000),
        InventoryLamp(id: 'b', name: 'b', lastSeenEpochMs: 9000),
        InventoryLamp(id: 'n', name: 'n'),
      ]);
      await c.read(inventoryNotifierProvider.notifier).recordLastSeen(
          {'a': 7000, 'b': 8000, 'n': 1234, 'stranger': 9999});
      expect(seenOf(c, 'a'), 7000);
      expect(seenOf(c, 'b'), 9000);
      expect(seenOf(c, 'n'), 1234);
      expect(
        c.read(inventoryNotifierProvider).value!.map((l) => l.id),
        ['a', 'b', 'n'],
      );
    });

    test('minStep holds back a write until the time moves by the step',
        () async {
      final c = await withLamps(const [
        InventoryLamp(id: 'a', name: 'a', lastSeenEpochMs: 10000),
      ]);
      final inv = c.read(inventoryNotifierProvider.notifier);
      const step = Duration(seconds: 30);
      await inv.recordLastSeen({'a': 10000 + step.inMilliseconds - 1},
          minStep: step);
      expect(seenOf(c, 'a'), 10000);
      await inv.recordLastSeen({'a': 10000 + step.inMilliseconds},
          minStep: step);
      expect(seenOf(c, 'a'), 10000 + step.inMilliseconds);
    });

    test('persists across a restart', () async {
      final c1 = ProviderContainer();
      await c1.read(inventoryNotifierProvider.future);
      final inv = c1.read(inventoryNotifierProvider.notifier);
      await inv.add(const InventoryLamp(id: 'a', name: 'a'));
      await inv.recordLastSeen({'a': 4242});
      c1.dispose();
      final c2 = ProviderContainer();
      addTearDown(c2.dispose);
      final list = await c2.read(inventoryNotifierProvider.future);
      expect(list.single.lastSeenEpochMs, 4242);
    });
  });
}
