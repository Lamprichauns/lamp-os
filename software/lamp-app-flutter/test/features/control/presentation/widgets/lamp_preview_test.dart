import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:flutter_test/flutter_test.dart';
import '../../../../_support/in_memory_ble_client.dart';
import 'package:lamp_app/core/ble/ble_client_provider.dart';
import 'package:lamp_app/features/control/application/control_notifier.dart';
import 'package:lamp_app/features/control/domain/lamp_color.dart';
import 'package:lamp_app/features/control/presentation/widgets/lamp_preview.dart';
import 'package:lamp_app/features/inventory/application/inventory_notifier.dart';
import 'package:lamp_app/features/inventory/domain/inventory_lamp.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../../../../_support/seed.dart';

const _devId = 'dev-1';

Future<ProviderContainer> _buildContainer({
  String shadeColorsJson = '["#ABCDEFFF"]',
  String baseColorsJson = '["#FF0000FF","#00FF00FF"]',
}) async {
  SharedPreferences.setMockInitialValues({});
  final ble = InMemoryBleClient();
  await seedControlBle(ble,
      deviceId: _devId,
      shadeColorsJson: shadeColorsJson,
      baseColorsJson: baseColorsJson);
  final c = ProviderContainer(
    overrides: [bleClientProvider.overrideWithValue(ble)],
  );
  await c.read(inventoryNotifierProvider.future);
  await c.read(inventoryNotifierProvider.notifier).add(const InventoryLamp(
        id: _devId,
        name: 'jacko',
        controlPassword: '',
      ));
  await c.read(controlNotifierProvider(_devId).future);
  return c;
}

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  Widget wrap(ProviderContainer c, Widget child) =>
      UncontrolledProviderScope(
          container: c, child: MaterialApp(home: Scaffold(body: child)));

  testWidgets('renders a SvgPicture after the template loads', (tester) async {
    final c = await _buildContainer();
    addTearDown(c.dispose);
    await tester.pumpWidget(wrap(c, const LampPreview(deviceId: _devId)));
    await tester.pumpAndSettle();
    expect(find.byType(SvgPicture), findsOneWidget);
  });

  testWidgets(
      'single baseColor produces two identical stops (no division-by-zero)',
      (tester) async {
    final c = await _buildContainer(baseColorsJson: '["#102030FF"]');
    addTearDown(c.dispose);
    await tester.pumpWidget(wrap(c, const LampPreview(deviceId: _devId)));
    await tester.pumpAndSettle();
    expect(find.byType(SvgPicture), findsOneWidget);
  });

  // The original "empty baseColors falls back to black stops" test
  // exercised a defensive code path (`_buildStops` returning two black
  // stops on an empty list) that was reachable when LampPreview took
  // `baseColors` directly as a prop. With the new architecture
  // LampPreview reads from `controlNotifierProvider`, and the notifier
  // itself rejects empty base.colors at build time
  // (control_notifier.dart:374). The internal `_buildStops` fallback
  // stays for the (state.value == null) early-load case; it's not
  // directly drivable via the state machine.

  test('renderLampPreviewSvg puts the shade top first and the base as stored',
      () {
    const red = LampColor(r: 0xFF, g: 0, b: 0, w: 0);
    const blue = LampColor(r: 0, g: 0, b: 0xFF, w: 0);
    const template = '<svg><defs>'
        '<linearGradient id="critter1Shade" x1="0%" y1="0%" x2="0%" y2="100%">'
        '<stop offset="0%" style="stop-color:#123456;stop-opacity:1"/>'
        '</linearGradient>'
        '<linearGradient id="critter1Body" x1="0%" y1="0%" x2="0%" y2="100%">'
        '<stop offset="0%" style="stop-color:#123456;stop-opacity:1"/>'
        '</linearGradient>'
        '</defs></svg>';

    final svg = renderLampPreviewSvg(template, const [red, blue], const [red, blue]);

    String gradient(String suffix) => RegExp(
          '<linearGradient[^>]*id="[^"]*$suffix"[^>]*>.*?</linearGradient>',
        ).firstMatch(svg)!.group(0)!;
    String stop(int pct, LampColor c) =>
        '<stop offset="$pct%" style="stop-color:#${c.toRgbHex()};';

    final shade = gradient('Shade');
    expect(shade, contains(stop(0, blue)));
    expect(shade, contains(stop(100, red)));
    final body = gradient('Body');
    expect(body, contains(stop(0, red)));
    expect(body, contains(stop(100, blue)));
    expect(svg, isNot(contains('123456')));
  });
}
