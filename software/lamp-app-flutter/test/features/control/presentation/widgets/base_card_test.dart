import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:go_router/go_router.dart';
import '../../../../_support/in_memory_ble_client.dart';
import 'package:lamp_app/core/ble/ble_client_provider.dart';
import 'package:lamp_app/features/control/application/control_notifier.dart';
import 'package:lamp_app/features/control/domain/lamp_color.dart';
import 'package:lamp_app/features/control/presentation/widgets/base_card.dart';
import 'package:lamp_app/features/inventory/application/inventory_notifier.dart';
import 'package:lamp_app/features/inventory/domain/inventory_lamp.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../../../../_support/seed.dart';

const _devId = 'aa:bb:cc:dd:ee:ff';

Future<ProviderContainer> _buildContainer({
  String baseColorsJson = '["#300783FF","#FF0000FF"]',
  bool? baseAutoColors,
  int? baseColorScheme,
}) async {
  SharedPreferences.setMockInitialValues({});
  final ble = InMemoryBleClient();
  await seedControlBle(ble,
      deviceId: _devId,
      baseColorsJson: baseColorsJson,
      baseAutoColors: baseAutoColors,
      baseColorScheme: baseColorScheme);
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
        home: Scaffold(body: BaseCard(lampId: _devId, onTap: () {})),
      ),
    ));

void main() {
  testWidgets('renders the "Base" label and no "stops" subtitle',
      (tester) async {
    final c = await _buildContainer();
    addTearDown(c.dispose);
    await tester.pumpWidget(UncontrolledProviderScope(
      container: c,
      child: MaterialApp(
        home: Scaffold(
          body: BaseCard(lampId: _devId, onTap: () {}),
        ),
      ),
    ));
    await tester.pumpAndSettle();
    expect(find.text('Base'), findsOneWidget);
    expect(find.textContaining('stops'), findsNothing);
  });

  testWidgets('does not render per-color circle chips', (tester) async {
    final c = await _buildContainer(
        baseColorsJson: '["#300783FF","#FF0000FF","#00FF00FF"]');
    addTearDown(c.dispose);
    await tester.pumpWidget(UncontrolledProviderScope(
      container: c,
      child: MaterialApp(
        home: Scaffold(
          body: BaseCard(lampId: _devId, onTap: () {}),
        ),
      ),
    ));
    await tester.pumpAndSettle();
    final circles = tester
        .widgetList<Container>(find.descendant(
            of: find.byType(BaseCard), matching: find.byType(Container)))
        .where((c) {
      final d = c.decoration;
      return d is BoxDecoration && d.shape == BoxShape.circle;
    }).toList();
    expect(circles, isEmpty);
  });

  testWidgets('still renders the gradient ribbon container', (tester) async {
    final c = await _buildContainer();
    addTearDown(c.dispose);
    await tester.pumpWidget(UncontrolledProviderScope(
      container: c,
      child: MaterialApp(
        home: Scaffold(
          body: BaseCard(lampId: _devId, onTap: () {}),
        ),
      ),
    ));
    await tester.pumpAndSettle();
    final hasGradient = tester
        .widgetList<Container>(find.descendant(
            of: find.byType(BaseCard), matching: find.byType(Container)))
        .where((c) {
      final d = c.decoration;
      return d is BoxDecoration && d.gradient is LinearGradient;
    }).toList();
    expect(hasGradient, isNotEmpty);
  });

  testWidgets('gradient puts storage index 0 at the top', (tester) async {
    const red = LampColor(r: 0xFF, g: 0, b: 0, w: 0);
    const blue = LampColor(r: 0, g: 0, b: 0xFF, w: 0);
    final c = await _buildContainer(baseColorsJson: '["#FF000000","#0000FF00"]');
    addTearDown(c.dispose);
    await tester.pumpWidget(UncontrolledProviderScope(
      container: c,
      child: MaterialApp(
        home: Scaffold(
          body: BaseCard(lampId: _devId, onTap: () {}),
        ),
      ),
    ));
    await tester.pumpAndSettle();
    final gradient = tester
        .widgetList<Container>(find.descendant(
            of: find.byType(BaseCard), matching: find.byType(Container)))
        .map((c) => c.decoration)
        .whereType<BoxDecoration>()
        .map((d) => d.gradient)
        .whereType<LinearGradient>()
        .single;
    expect(gradient.begin, Alignment.topCenter);
    expect(gradient.colors, [red.toSwatch(), blue.toSwatch()]);
  });

  group('Auto-pick note', () {
    testWidgets('scheme link pushes the Color harmony route; the body opens '
        'the editor', (tester) async {
      final c = await _buildContainer(
        baseAutoColors: true,
        baseColorScheme: 4,
      );
      addTearDown(c.dispose);
      var bodyTaps = 0;
      final router = GoRouter(
        initialLocation: '/home',
        routes: [
          GoRoute(
            path: '/home',
            builder: (_, _) => Scaffold(
              body: BaseCard(lampId: _devId, onTap: () => bodyTaps++),
            ),
          ),
          GoRoute(
            path: '/lamp/:id/setup/color-harmony',
            builder: (_, _) => const Scaffold(body: Text('harmony-stub')),
          ),
        ],
      );
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

      await tester.tap(find.text('Base'));
      await tester.pumpAndSettle();
      expect(bodyTaps, 1);

      await tester.tap(find.byKey(const Key('auto-pick-link')));
      await tester.pumpAndSettle();
      expect(find.text('harmony-stub'), findsOneWidget);
      expect(bodyTaps, 1);
    });

    testWidgets('a neutral last color asks for a more colorful bottom color',
        (tester) async {
      final c = await _buildContainer(
        baseColorsJson: '["#FF0000FF","#FFFFFF00"]',
        baseAutoColors: true,
      );
      addTearDown(c.dispose);
      await _pumpCard(tester, c);
      await tester.pumpAndSettle();

      expect(find.text('Auto-picked · pick a more colorful bottom color'),
          findsOneWidget);
      expect(find.byKey(const Key('auto-pick-link')), findsNothing);
    });

    testWidgets('no note while Auto-pick is off', (tester) async {
      final c = await _buildContainer(baseColorScheme: 4);
      addTearDown(c.dispose);
      await _pumpCard(tester, c);
      await tester.pumpAndSettle();

      expect(find.textContaining('Auto-picked'), findsNothing);
      expect(find.byKey(const Key('auto-pick-link')), findsNothing);
    });
  });
}
