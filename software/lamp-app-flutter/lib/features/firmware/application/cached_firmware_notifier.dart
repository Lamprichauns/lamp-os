import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:path_provider/path_provider.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';

import '../../inventory/domain/inventory_lamp.dart';
import '../data/cached_firmware.dart';
import '../data/firmware_release_client.dart';
import '../data/firmware_release_client_provider.dart';
import '../data/firmware_signature_verifier.dart';

part 'cached_firmware_notifier.g.dart';

/// Whether to replace the cached firmware with a freshly-fetched GitHub
/// image. Skips the replace when the cached copy is a newer version than
/// GitHub's, so a manually-pushed newer build isn't clobbered. Versions are
/// packed ints; larger = newer.
bool shouldReplaceCachedFirmware(int? cachedVersion, int githubVersion) =>
    cachedVersion == null || cachedVersion <= githubVersion;

@Riverpod(keepAlive: true, name: 'cachedFirmwareNotifierProvider')
class CachedFirmwareNotifier extends _$CachedFirmwareNotifier {
  static const String _cacheDirName = 'firmware-cache';
  static const String _binSuffix = 'signed.bin';
  static const String _metaSuffix = 'meta.json';

  Directory? _cacheDir;
  bool _synced = false;
  final Set<String> _seenLampTypes = {};

  @override
  Future<Map<String, CachedFirmware>> build() async {
    return _loadIndex();
  }

  /// Map keyed by `{lampType}-{channel.name}`. Empty when the cache is
  /// clean or hasn't materialised yet.
  Map<String, CachedFirmware> get index =>
      state.value ?? const <String, CachedFirmware>{};

  Future<Directory> _ensureCacheDir() async {
    if (_cacheDir != null) return _cacheDir!;
    final docs = await getApplicationDocumentsDirectory();
    final dir = Directory('${docs.path}/$_cacheDirName');
    if (!await dir.exists()) {
      await dir.create(recursive: true);
    }
    _cacheDir = dir;
    return dir;
  }

  Future<Map<String, CachedFirmware>> _loadIndex() async {
    final dir = await _ensureCacheDir();
    final out = <String, CachedFirmware>{};
    await for (final entity in dir.list()) {
      if (entity is! File) continue;
      if (!entity.path.endsWith('.$_metaSuffix')) continue;
      try {
        final raw = await entity.readAsString();
        final meta = CachedFirmware.fromJson(
            jsonDecode(raw) as Map<String, dynamic>);
        final binPath = _binPathFor(dir, meta.lampType, meta.channel);
        if (await File(binPath).exists()) {
          out[meta.key] = meta;
        }
      } catch (_) {
        // Skip corrupt sidecars silently. The next sync overwrites.
      }
    }
    return out;
  }

  String _binPathFor(Directory dir, String lampType, FirmwareChannel channel) =>
      '${dir.path}/lamp-firmware-$lampType-${channel.name}-$_binSuffix';

  String _metaPathFor(Directory dir, String lampType, FirmwareChannel channel) =>
      '${dir.path}/lamp-firmware-$lampType-${channel.name}.$_metaSuffix';

  static Set<String> _lampTypesOf(List<InventoryLamp> inventory) => {
        for (final lamp in inventory)
          if (lamp.lampType case final t? when t.isNotEmpty) t,
      };

  /// Keeps the cache in step with the owned variants, reaching GitHub at most
  /// once per variant per app launch. The first call fetches every owned
  /// variant; a later call fetches only a variant it has never seen that has
  /// nothing cached. Every other call is free, so the caller may pass each
  /// inventory change. A failed fetch keeps the previous cached copy and is
  /// not retried until the next launch.
  Future<void> syncForInventory(List<InventoryLamp> inventory) async {
    final launch = !_synced;
    _synced = true;
    final newTypes = _lampTypesOf(inventory).difference(_seenLampTypes);
    if (newTypes.isEmpty) return;
    _seenLampTypes.addAll(newTypes);

    // Awaiting the initial load keeps a launch sync from reading the empty
    // mid-build index and letting an older GitHub release clobber a newer
    // manually-pushed build.
    final loaded = await future;
    final toFetch = launch
        ? newTypes
        : newTypes.where((t) => !loaded.values.any((m) => m.lampType == t));
    final client = ref.read(firmwareReleaseClientProvider);
    final dir = await _ensureCacheDir();
    for (final lampType in toFetch) {
      // Both channels, so a mixed fleet can be pushed whichever channel a
      // lamp is on. A stable-`latest` 404 is expected until a stable release
      // matching the asset naming exists.
      for (final channel in FirmwareChannel.values) {
        try {
          final bytes = await client.fetchLatest(channel, lampType: lampType);
          final verified = await verifyFirmwareImage(bytes);
          final key = '$lampType-${channel.name}';
          if (!shouldReplaceCachedFirmware(
              index[key]?.version, verified.footer.version)) {
            continue;
          }
          await File(_binPathFor(dir, lampType, channel))
              .writeAsBytes(bytes, flush: true);
          final meta = CachedFirmware(
            lampType: lampType,
            channel: channel,
            version: verified.footer.version,
            byteLength: bytes.length,
            fetchedAtMs: DateTime.now().millisecondsSinceEpoch,
          );
          await File(_metaPathFor(dir, lampType, channel))
              .writeAsString(jsonEncode(meta.toJson()));
          state = AsyncData({...index, meta.key: meta});
        } catch (_) {
          // Network / verify failure. Keep the previous entry (if any).
        }
      }
    }
  }

  /// Return the cached bytes + meta for a {lampType, channel} pair, or
  /// null if not cached. Backs the firmware panel's install-from-cache push.
  Future<CachedFirmwareBlob?> bytesFor({
    required String lampType,
    required FirmwareChannel channel,
  }) async {
    final meta = index['$lampType-${channel.name}'];
    if (meta == null) return null;
    try {
      final dir = await _ensureCacheDir();
      final bytes =
          await File(_binPathFor(dir, lampType, channel)).readAsBytes();
      return CachedFirmwareBlob(bytes: Uint8List.fromList(bytes), meta: meta);
    } catch (_) {
      return null;
    }
  }

  /// Wipe a single cache entry (file + sidecar + map slot). Used by the
  /// advanced-mode "remove" affordance.
  Future<void> delete({
    required String lampType,
    required FirmwareChannel channel,
  }) async {
    final key = '$lampType-${channel.name}';
    final meta = index[key];
    if (meta == null) return;
    final dir = await _ensureCacheDir();
    try {
      await File(_binPathFor(dir, lampType, channel)).delete();
    } catch (_) {}
    try {
      await File(_metaPathFor(dir, lampType, channel)).delete();
    } catch (_) {}
    final next = Map<String, CachedFirmware>.from(index)..remove(key);
    state = AsyncData(next);
  }
}
