// ignore_for_file: depend_on_referenced_packages
// path_provider_platform_interface: transitive via path_provider, used here
// to fake the docs directory.

import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:lamp_app/core/routing/router.dart';
import 'package:lamp_app/features/firmware/data/firmware_release_client.dart';
import 'package:lamp_app/features/firmware/data/firmware_release_client_provider.dart';
import 'package:lamp_app/features/inventory/application/inventory_notifier.dart';
import 'package:lamp_app/features/inventory/domain/inventory_lamp.dart';
import 'package:path_provider_platform_interface/path_provider_platform_interface.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  late Directory tempDir;

  setUp(() async {
    tempDir = await Directory.systemTemp.createTemp('router_fw_sync_');
    PathProviderPlatform.instance = _FakeDocsPath(tempDir.path);
  });

  tearDown(() => tempDir.delete(recursive: true));

  test('the first inventory triggers one firmware fetch round; inventory '
      'changes after it trigger none', () async {
    const lamp = InventoryLamp(id: 'a', name: 'a', lampType: 'standard');
    final client = _CountingReleaseClient();
    final inventory = _FakeInventory(const [lamp]);
    final container = ProviderContainer(
      overrides: [
        inventoryNotifierProvider.overrideWith(() => inventory),
        firmwareReleaseClientProvider.overrideWithValue(client),
      ],
    );
    addTearDown(container.dispose);

    container.read(appRouterProvider);
    await container.read(inventoryNotifierProvider.future);
    await client.reached(FirmwareChannel.values.length);

    for (final next in [
      [lamp.copyWith(name: 'renamed')],
      [lamp.copyWith(lastSeenEpochMs: 1)],
      [lamp, const InventoryLamp(id: 'b', name: 'b', lampType: 'standard')],
      <InventoryLamp>[],
    ]) {
      inventory.emit(next);
      await pumpEventQueue();
    }
    expect(client.calls, FirmwareChannel.values.length);
  });
}

class _FakeInventory extends InventoryNotifier {
  _FakeInventory(this._seed);
  final List<InventoryLamp> _seed;

  @override
  Future<List<InventoryLamp>> build() async => _seed;

  void emit(List<InventoryLamp> list) => state = AsyncData(list);
}

class _CountingReleaseClient extends FirmwareReleaseClient {
  int calls = 0;
  int _target = 0;
  final _reached = Completer<void>();

  Future<void> reached(int n) {
    _target = n;
    if (calls >= n) _reached.complete();
    return _reached.future.timeout(const Duration(seconds: 5));
  }

  @override
  Future<Uint8List> fetchLatest(
    FirmwareChannel channel, {
    required String lampType,
  }) async {
    calls++;
    if (calls == _target && !_reached.isCompleted) _reached.complete();
    throw const FirmwareDownloadException('test: no network');
  }
}

class _FakeDocsPath extends PathProviderPlatform {
  _FakeDocsPath(this.path);
  final String path;

  @override
  Future<String?> getApplicationDocumentsPath() async => path;
}
