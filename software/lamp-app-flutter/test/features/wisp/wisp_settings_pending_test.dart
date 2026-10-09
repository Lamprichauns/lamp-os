import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:lamp_app/core/ble/ble_client_provider.dart';
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

Widget _wrap(Widget child) => ProviderScope(
      overrides: [bleClientProvider.overrideWithValue(_ble)],
      child: MaterialApp(
        home: Scaffold(body: SingleChildScrollView(child: child)),
      ),
    );

/// Settings forms never show placeholder defaults: until the wisp's config
/// is known they show a pending state, and a stateful form seeds from the
/// real values the moment they arrive.
void main() {
  testWidgets('LED config shows pending, no picker, until config is known',
      (tester) async {
    await tester.pumpWidget(
      _wrap(const WispLedConfig(lampId: _lampId, status: _unknown)),
    );

    expect(find.byKey(_pending), findsOneWidget);
    expect(find.byType(SegmentedButton<String>), findsNothing);
    expect(find.byKey(const Key('wisp-led-count-row')), findsNothing);
  });

  testWidgets('drift sliders seed from the real config once it arrives',
      (tester) async {
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

  testWidgets('space brightness seeds from the real config once it arrives',
      (tester) async {
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
}
