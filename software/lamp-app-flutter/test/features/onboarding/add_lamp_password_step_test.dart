import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';

import '../../_support/in_memory_ble_client.dart';
import 'package:lamp_app/core/ble/ble_client_provider.dart';
import 'package:lamp_app/core/theme/app_theme.dart';
import 'package:lamp_app/features/onboarding/application/add_lamp_notifier.dart';
import 'package:lamp_app/features/onboarding/presentation/widgets/add_lamp_password_step.dart';

void main() {
  ProviderContainer makeContainer() => ProviderContainer(
        overrides: [bleClientProvider.overrideWithValue(InMemoryBleClient())],
      );

  Widget wrap(ProviderContainer c) => UncontrolledProviderScope(
        container: c,
        child: MaterialApp(
          theme: appTheme,
          home: const Scaffold(body: AddLampPasswordStep()),
        ),
      );

  testWidgets('submit button label interpolates name as Take {name} home',
      (tester) async {
    final c = makeContainer();
    addTearDown(c.dispose);

    final notifier = c.read(addLampNotifierProvider.notifier);
    notifier.select('test-lamp');
    notifier.next(); // adoptConfirm → name
    notifier.setName('Sparky');
    notifier.next(); // name → password

    await tester.pumpWidget(wrap(c));
    await tester.pump();

    expect(find.text('Take Sparky home'), findsOneWidget);
    expect(find.text('Welcome them home'), findsNothing);
  });

  testWidgets('a password too short for WPA2 cannot be submitted',
      (tester) async {
    final c = makeContainer();
    addTearDown(c.dispose);

    final notifier = c.read(addLampNotifierProvider.notifier);
    notifier.select('test-lamp');
    notifier.next();
    notifier.setName('Sparky');
    notifier.next();

    await tester.pumpWidget(wrap(c));
    await tester.pump();

    final fields = find.byType(TextField);
    await tester.enterText(fields.first, 'short');
    await tester.enterText(fields.last, 'short');
    await tester.pump();

    expect(find.text('Must be at least 8 characters'), findsOneWidget);
    final short = tester.widget<FilledButton>(
        find.widgetWithText(FilledButton, 'Take Sparky home'));
    expect(short.onPressed, isNull);

    await tester.enterText(fields.first, 'longenough');
    await tester.enterText(fields.last, 'longenough');
    await tester.pump();

    expect(find.text('Must be at least 8 characters'), findsNothing);
    final ok = tester.widget<FilledButton>(
        find.widgetWithText(FilledButton, 'Take Sparky home'));
    expect(ok.onPressed, isNotNull);
  });
}
