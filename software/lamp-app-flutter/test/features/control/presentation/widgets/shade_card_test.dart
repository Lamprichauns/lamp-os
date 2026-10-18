import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:go_router/go_router.dart';
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
  bool? shadeAutoColors,
  int? shadeColorScheme,
  String? shadeSegmentsJson,
}) async {
  SharedPreferences.setMockInitialValues({});
  final ble = InMemoryBleClient();
  await seedControlBle(ble,
      deviceId: _devId,
      shadeColorsJson: shadeColorsJson,
      shadeBpp: 4,
      shadeAutoColors: shadeAutoColors,
      shadeColorScheme: shadeColorScheme,
      shadeSegmentsJson: shadeSegmentsJson);
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

Future<void> _pumpCard(WidgetTester tester, ProviderContainer c) =>
    tester.pumpWidget(UncontrolledProviderScope(
      container: c,
      child: MaterialApp(
        home: Scaffold(
          body: ShadeCard(lampId: _devId, spec: shadeSegmentSpec(0, 'Shade')),
        ),
      ),
    ));

GoRouter _routerWithHarmonyStub() => GoRouter(
      initialLocation: '/home',
      routes: [
        GoRoute(
          path: '/home',
          builder: (_, _) => Scaffold(
            body:
                ShadeCard(lampId: _devId, spec: shadeSegmentSpec(0, 'Shade')),
          ),
        ),
        GoRoute(
          path: '/lamp/:id/setup/color-harmony',
          builder: (_, _) => const Scaffold(body: Text('harmony-stub')),
        ),
      ],
    );

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

  group('Auto-pick note', () {
    testWidgets('shows the scheme link, which pushes the Color harmony route',
        (tester) async {
      final c = await _buildContainer(
        shadeColorsJson: '["#FF0000C8"]',
        shadeAutoColors: true,
        shadeColorScheme: 4,
      );
      addTearDown(c.dispose);
      final router = _routerWithHarmonyStub();
      addTearDown(router.dispose);
      await tester.pumpWidget(UncontrolledProviderScope(
        container: c,
        child: MaterialApp.router(routerConfig: router),
      ));
      await tester.pumpAndSettle();

      expect(find.text('Auto-picked · '), findsOneWidget);
      expect(
        find.descendant(
          of: find.byKey(const Key('auto-pick-link')),
          matching: find.text('Triadic'),
        ),
        findsOneWidget,
      );

      await tester.tap(find.byKey(const Key('auto-pick-link')));
      await tester.pumpAndSettle();
      expect(find.text('harmony-stub'), findsOneWidget);
    });

    testWidgets('tapping the card body still opens the editor',
        (tester) async {
      final c = await _buildContainer(
        shadeColorsJson: '["#FF0000C8"]',
        shadeAutoColors: true,
        shadeColorScheme: 4,
      );
      addTearDown(c.dispose);
      final router = _routerWithHarmonyStub();
      addTearDown(router.dispose);
      await tester.pumpWidget(UncontrolledProviderScope(
        container: c,
        child: MaterialApp.router(routerConfig: router),
      ));
      await tester.pumpAndSettle();

      await tester.tap(find.text('Shade'));
      await tester.pumpAndSettle();
      expect(find.byType(ShadeEditorSheet), findsOneWidget);
      expect(find.text('harmony-stub'), findsNothing);
    });

    testWidgets('a neutral primary asks for a more colorful top color',
        (tester) async {
      final c = await _buildContainer(
        shadeColorsJson: '["#FFFFFF00"]',
        shadeAutoColors: true,
      );
      addTearDown(c.dispose);
      await _pumpCard(tester, c);
      await tester.pumpAndSettle();

      expect(find.text('Auto-picked · pick a more colorful top color'),
          findsOneWidget);
      expect(find.byKey(const Key('auto-pick-link')), findsNothing);
    });

    testWidgets('reads the primary from the last stored color',
        (tester) async {
      final c = await _buildContainer(
        shadeColorsJson: '["#FF0000C8","#FFFFFF00"]',
        shadeAutoColors: true,
      );
      addTearDown(c.dispose);
      await _pumpCard(tester, c);
      await tester.pumpAndSettle();

      expect(find.text('Auto-picked · pick a more colorful top color'),
          findsOneWidget);
    });

    testWidgets('no note while Auto-pick is off', (tester) async {
      final c = await _buildContainer(
        shadeColorsJson: '["#FF0000C8"]',
        shadeColorScheme: 4,
      );
      addTearDown(c.dispose);
      await _pumpCard(tester, c);
      await tester.pumpAndSettle();

      expect(find.textContaining('Auto-picked'), findsNothing);
      expect(find.byKey(const Key('auto-pick-link')), findsNothing);
    });

    testWidgets('no note on any card of a three-segment shade',
        (tester) async {
      final c = await _buildContainer(
        shadeAutoColors: true,
        shadeColorScheme: 4,
        shadeSegmentsJson:
            '[{"name":"A","px":12,"colors":["#FF0000C8"]},'
            '{"name":"B","px":12,"colors":["#00FF00C8"]},'
            '{"name":"C","px":14,"colors":["#0000FFC8"]}]',
      );
      addTearDown(c.dispose);
      await tester.pumpWidget(UncontrolledProviderScope(
        container: c,
        child: MaterialApp(
          home: Scaffold(
            body: Column(
              children: [
                for (var i = 0; i < 3; i++)
                  ShadeCard(lampId: _devId, spec: shadeSegmentSpec(i, 'S$i')),
              ],
            ),
          ),
        ),
      ));
      await tester.pumpAndSettle();

      expect(find.byType(ShadeCard), findsNWidgets(3));
      expect(find.textContaining('Auto-picked'), findsNothing);
    });

    testWidgets('a long scheme name wraps on a 360 dp phone', (tester) async {
      tester.view.physicalSize = const Size(360, 800);
      tester.view.devicePixelRatio = 1;
      addTearDown(tester.view.reset);
      final c = await _buildContainer(
        shadeColorsJson: '["#FF0000C8"]',
        shadeAutoColors: true,
        shadeColorScheme: 3,
      );
      addTearDown(c.dispose);
      await _pumpCard(tester, c);
      await tester.pumpAndSettle();

      expect(tester.takeException(), isNull);
      expect(find.text('Split complementary'), findsOneWidget);
    });
  });
}
