import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:lamp_app/core/ble/ble_client_provider.dart';
import 'package:lamp_app/features/wisp/application/wisp_notifier.dart';
import 'package:lamp_app/features/wisp/domain/wisp_status.dart';
import 'package:lamp_app/features/wisp/presentation/widgets/drift_controls.dart';
import 'package:lamp_app/features/wisp/presentation/widgets/space_brightness_slider.dart';
import 'package:lamp_app/features/wisp/presentation/widgets/wisp_led_config.dart';

import '../../_support/in_memory_ble_client.dart';

const _lampId = 'lamp-pending-test';
const _pending = Key('wisp-settings-pending');

// The wisp is present but its settings have not arrived.
const _unknown = WispStatus(wispMac: 'AA:BB:CC:DD:EE:FF');

// One client for every pump, so a re-pump updates the same scope.
final _ble = InMemoryBleClient();

// Watches the notifier the way the wisp screen does, so the auto-dispose
// provider the forms write through stays alive for the test.
Widget _wrap(Widget child) => ProviderScope(
  overrides: [bleClientProvider.overrideWithValue(_ble)],
  child: MaterialApp(
    home: Scaffold(
      body: Consumer(
        builder: (context, ref, _) {
          ref.watch(wispNotifierProvider(_lampId));
          return SingleChildScrollView(child: child);
        },
      ),
    ),
  ),
);

/// Settings forms never show placeholder defaults: until the wisp's config
/// is known they show a pending state, and a stateful form seeds from the
/// real values the moment they arrive.
void main() {
  testWidgets('LED config shows pending, no picker, until config is known', (
    tester,
  ) async {
    await tester.pumpWidget(
      _wrap(const WispLedConfig(lampId: _lampId, status: _unknown)),
    );

    expect(find.byKey(_pending), findsOneWidget);
    expect(find.byType(SegmentedButton<String>), findsNothing);
    expect(find.byKey(const Key('wisp-led-count-row')), findsNothing);
  });

  testWidgets('drift sliders seed from the real config once it arrives', (
    tester,
  ) async {
    await tester.pumpWidget(
      _wrap(const DriftControls(lampId: _lampId, status: _unknown)),
    );
    expect(find.byKey(_pending), findsOneWidget);
    expect(find.byType(Slider), findsNothing);

    const known = WispStatus(
      wispMac: 'AA:BB:CC:DD:EE:FF',
      driftIntervalMs: 600000,
      driftFadePct: 15,
      configKnown: true,
    );
    await tester.pumpWidget(
      _wrap(const DriftControls(lampId: _lampId, status: known)),
    );

    expect(find.byKey(_pending), findsNothing);
    final sliders = tester.widgetList<Slider>(find.byType(Slider)).toList();
    expect(sliders, hasLength(2));
    expect(sliders[0].value, closeTo(msToPos(600000), 1e-9));
    expect(sliders[1].value, 15);
  });

  testWidgets('space brightness seeds from the real config once it arrives', (
    tester,
  ) async {
    await tester.pumpWidget(
      _wrap(const SpaceBrightnessSlider(lampId: _lampId, status: _unknown)),
    );
    expect(find.byKey(_pending), findsOneWidget);
    expect(find.byType(Slider), findsNothing);

    const known = WispStatus(
      wispMac: 'AA:BB:CC:DD:EE:FF',
      brightness: 40,
      configKnown: true,
    );
    await tester.pumpWidget(
      _wrap(const SpaceBrightnessSlider(lampId: _lampId, status: known)),
    );

    expect(find.byKey(_pending), findsNothing);
    expect(tester.widget<Slider>(find.byType(Slider)).value, 40);
  });

  // A later status from the wisp with different values moves the sliders;
  // the configKnown edge is not the only re-seed.
  testWidgets('drift sliders follow a changed value from the wisp', (
    tester,
  ) async {
    const first = WispStatus(
      wispMac: 'AA:BB:CC:DD:EE:FF',
      driftFadePct: 15,
      configKnown: true,
    );
    await tester.pumpWidget(
      _wrap(const DriftControls(lampId: _lampId, status: first)),
    );
    const next = WispStatus(
      wispMac: 'AA:BB:CC:DD:EE:FF',
      driftFadePct: 70,
      configKnown: true,
    );
    await tester.pumpWidget(
      _wrap(const DriftControls(lampId: _lampId, status: next)),
    );

    final sliders = tester.widgetList<Slider>(find.byType(Slider)).toList();
    expect(sliders[1].value, 70);
  });

  // While the user holds the slider, an incoming value must not yank it.
  testWidgets('a held brightness slider ignores incoming values', (
    tester,
  ) async {
    const first = WispStatus(
      wispMac: 'AA:BB:CC:DD:EE:FF',
      brightness: 40,
      configKnown: true,
    );
    await tester.pumpWidget(
      _wrap(const SpaceBrightnessSlider(lampId: _lampId, status: first)),
    );
    final gesture = await tester.startGesture(
      tester.getCenter(find.byType(Slider)),
    );
    await gesture.moveBy(const Offset(30, 0));
    await tester.pump(const Duration(milliseconds: 200));
    final held = tester.widget<Slider>(find.byType(Slider)).value;

    const next = WispStatus(
      wispMac: 'AA:BB:CC:DD:EE:FF',
      brightness: 90,
      configKnown: true,
    );
    await tester.pumpWidget(
      _wrap(const SpaceBrightnessSlider(lampId: _lampId, status: next)),
    );
    expect(tester.widget<Slider>(find.byType(Slider)).value, held);
    await gesture.up();
    await tester.pump(const Duration(milliseconds: 300));
  });

  // Writes are debounced, so a status from mid-drag can arrive after release
  // carrying an intermediate value. The slider holds the released value until
  // the wisp echoes it, then follows the wisp again.
  testWidgets('a released brightness slider holds until the wisp echoes it', (
    tester,
  ) async {
    WispStatus at(int b) => WispStatus(
      wispMac: 'AA:BB:CC:DD:EE:FF',
      brightness: b,
      configKnown: true,
    );
    Future<void> deliver(int b) => tester.pumpWidget(
      _wrap(SpaceBrightnessSlider(lampId: _lampId, status: at(b))),
    );
    double shown() => tester.widget<Slider>(find.byType(Slider)).value;

    // The notifier writes each dragged value into its state, so the widget
    // rebuilds with them mid-drag, as it does on screen.
    await deliver(30);
    tester.widget<Slider>(find.byType(Slider)).onChangeStart!(30);
    tester.widget<Slider>(find.byType(Slider)).onChanged!(45);
    await deliver(45);
    tester.widget<Slider>(find.byType(Slider)).onChanged!(60);
    await deliver(60);
    tester.widget<Slider>(find.byType(Slider)).onChangeEnd!(60);
    await tester.pump();

    // A rebuild with the unchanged optimistic 60 is not the wisp's echo.
    await deliver(60);
    await deliver(45);
    expect(shown(), 60);

    await deliver(60);
    expect(shown(), 60);
    await deliver(70);
    expect(shown(), 70);
    await tester.pump(const Duration(milliseconds: 300));
  });

  testWidgets('a released drift slider holds until the wisp echoes it', (
    tester,
  ) async {
    WispStatus at(int fade) => WispStatus(
      wispMac: 'AA:BB:CC:DD:EE:FF',
      driftFadePct: fade,
      configKnown: true,
    );
    Future<void> deliver(int fade) => tester.pumpWidget(
      _wrap(DriftControls(lampId: _lampId, status: at(fade))),
    );
    Slider fade() => tester.widgetList<Slider>(find.byType(Slider)).last;

    await deliver(30);
    fade().onChangeStart!(30);
    fade().onChanged!(45);
    await tester.pump();
    fade().onChanged!(60);
    await tester.pump();
    fade().onChangeEnd!(60);
    await tester.pump();

    await deliver(45);
    expect(fade().value, 60);

    await deliver(60);
    await deliver(70);
    expect(fade().value, 70);
    await tester.pump(const Duration(milliseconds: 400));
  });
}
