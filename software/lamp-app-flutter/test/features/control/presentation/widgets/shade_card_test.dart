import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import '../../../../_support/in_memory_ble_client.dart';
import 'package:lamp_app/core/ble/ble_client_provider.dart';
import 'package:lamp_app/features/control/application/control_notifier.dart';
import 'package:lamp_app/features/control/domain/lamp_color.dart';
import 'package:lamp_app/features/control/presentation/widgets/shade_card.dart';
import 'package:lamp_app/features/control/presentation/widgets/shade_editor_sheet.dart';
import 'package:lamp_app/features/inventory/application/inventory_notifier.dart';
import 'package:lamp_app/features/inventory/domain/inventory_lamp.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../../../../_support/seed.dart';

const _devId = 'aa:bb:cc:dd:ee:ff';

Future<ProviderContainer> _buildContainer({
  String shadeColorsJson = '["#300783FF"]',
}) async {
  SharedPreferences.setMockInitialValues({});
  final ble = InMemoryBleClient();
  await seedControlBle(ble,
      deviceId: _devId,
      shadeColorsJson: shadeColorsJson,
      shadeBpp: 4);
  final c = ProviderContainer(
    overrides: [bleClientProvider.overrideWithValue(ble)],
  );
  await c.read(inventoryNotifierProvider.future);
  await c.read(inventoryNotifierProvider.notifier).add(const InventoryLamp(
        id: _devId,
        name: 'jacko',
        controlPassword: 'secret',
      ));
  await c.read(controlNotifierProvider(_devId).future);
  return c;
}

void main() {
  testWidgets('renders the "Shade" label and no hex string', (tester) async {
    final c = await _buildContainer();
    addTearDown(c.dispose);
    await tester.pumpWidget(UncontrolledProviderScope(
      container: c,
      child: MaterialApp(
        home: Scaffold(
          body: ShadeCard(lampId: _devId, spec: shadeSegmentSpec(0,'Shade')),
        ),
      ),
    ));
    await tester.pumpAndSettle();
    expect(find.text('Shade'), findsOneWidget);
    expect(find.textContaining('#'), findsNothing);
    expect(find.textContaining('300783'), findsNothing);
  });

  testWidgets('renders a gradient preview swatch (matches BaseCard)',
      (tester) async {
    final c = await _buildContainer();
    addTearDown(c.dispose);
    await tester.pumpWidget(UncontrolledProviderScope(
      container: c,
      child: MaterialApp(
        home: Scaffold(
          body: ShadeCard(lampId: _devId, spec: shadeSegmentSpec(0,'Shade')),
        ),
      ),
    ));
    await tester.pumpAndSettle();
    // The shade preview is a Container with a LinearGradient decoration —
    // shade is now a gradient (parity with base), not a single color, so
    // the previous LampColorSwatch widget was replaced with the same
    // gradient pattern BaseCard uses.
    final containers = tester.widgetList<Container>(find.byType(Container));
    final hasGradientContainer = containers.any((c) {
      final dec = c.decoration;
      return dec is BoxDecoration && dec.gradient is LinearGradient;
    });
    expect(hasGradientContainer, isTrue);
  });

  testWidgets('gradient puts the last stored color at the top',
      (tester) async {
    const red = LampColor(r: 0xFF, g: 0, b: 0, w: 0);
    const blue = LampColor(r: 0, g: 0, b: 0xFF, w: 0);
    final c = await _buildContainer(shadeColorsJson: '["#FF000000","#0000FF00"]');
    addTearDown(c.dispose);
    await tester.pumpWidget(UncontrolledProviderScope(
      container: c,
      child: MaterialApp(
        home: Scaffold(
          body: ShadeCard(lampId: _devId, spec: shadeSegmentSpec(0, 'Shade')),
        ),
      ),
    ));
    await tester.pumpAndSettle();
    final gradient = tester
        .widgetList<Container>(find.descendant(
            of: find.byType(ShadeCard), matching: find.byType(Container)))
        .map((c) => c.decoration)
        .whereType<BoxDecoration>()
        .map((d) => d.gradient)
        .whereType<LinearGradient>()
        .single;
    expect(gradient.begin, Alignment.topCenter);
    expect(gradient.colors, [blue.toSwatch(), red.toSwatch()]);
  });
}
