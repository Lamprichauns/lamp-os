import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:lamp_app/core/theme/app_theme.dart';
import 'package:lamp_app/core/widgets/app_sheet.dart';
import 'package:lamp_app/features/control/domain/lamp_color.dart';
import 'package:lamp_app/features/control/presentation/widgets/color_blocks_bar.dart';
import 'package:lamp_app/features/control/presentation/widgets/color_stops_sheet.dart';

const _red = LampColor(r: 0xFF, g: 0, b: 0, w: 0);
const _green = LampColor(r: 0, g: 0xFF, b: 0, w: 0);
const _blue = LampColor(r: 0, g: 0, b: 0xFF, w: 0);

Finder _rowHex(int position, String hex) => find.descendant(
      of: find.byKey(ValueKey('color-stop-$position')),
      matching: find.text(hex),
    );

void main() {
  testWidgets('ColorBlocksBar renders one block per color and fires onTap',
      (tester) async {
    var taps = 0;
    await tester.pumpWidget(MaterialApp(
      theme: appTheme,
      home: Scaffold(
        body: ColorBlocksBar(
          colors: const [_red, _green],
          onTap: () => taps++,
        ),
      ),
    ));
    expect(
      find.descendant(
        of: find.byType(ColorBlocksBar),
        matching: find.byType(ColoredBox),
      ),
      findsNWidgets(2),
    );
    await tester.tap(find.byType(ColorBlocksBar));
    expect(taps, 1);
  });

  testWidgets('ColorBlocksBar shows the empty state with no colors',
      (tester) async {
    await tester.pumpWidget(const MaterialApp(
      home: Scaffold(body: ColorBlocksBar(colors: [])),
    ));
    expect(
      find.descendant(
        of: find.byType(ColorBlocksBar),
        matching: find.byType(ColoredBox),
      ),
      findsNothing,
    );
    expect(find.text('Tap to set colors'), findsOneWidget);
  });

  testWidgets(
      'ColorStopsSheet non-reorderable: no drag handles, add streams onChanged, '
      'Save commits and closes', (tester) async {
    final changes = <List<LampColor>>[];
    List<LampColor>? saved;

    await tester.pumpWidget(MaterialApp(
      theme: appTheme,
      home: Builder(
        builder: (ctx) => Scaffold(
          body: TextButton(
            onPressed: () => showColorStopsSheet(
              ctx,
              initial: const [_red, _green],
              title: 'Palette',
              max: 4,
              reorderable: false,
              onChanged: (c) => changes.add(c),
              onSave: (c) async => saved = c,
            ),
            child: const Text('open'),
          ),
        ),
      ),
    ));
    await tester.tap(find.text('open'));
    await tester.pumpAndSettle();

    expect(find.text('Palette'), findsOneWidget);
    expect(find.byIcon(Icons.drag_indicator), findsNothing);

    await tester.tap(find.text('Add Color'));
    await tester.pumpAndSettle();
    expect(changes.last.length, 3);

    await tester.tap(find.widgetWithText(FilledButton, 'Save'));
    await tester.pumpAndSettle();
    expect(saved, isNotNull);
    expect(saved!.length, 3);
    expect(find.text('Palette'), findsNothing);
  });

  testWidgets('ColorStopsSheet Cancel reverts the live preview to initial',
      (tester) async {
    List<LampColor>? lastPreview;

    await tester.pumpWidget(MaterialApp(
      theme: appTheme,
      home: Builder(
        builder: (ctx) => Scaffold(
          body: TextButton(
            onPressed: () => showColorStopsSheet(
              ctx,
              initial: const [_red, _green],
              title: 'Palette',
              max: 4,
              reorderable: false,
              onChanged: (c) => lastPreview = c,
              onSave: (_) async {},
            ),
            child: const Text('open'),
          ),
        ),
      ),
    ));
    await tester.tap(find.text('open'));
    await tester.pumpAndSettle();

    // Remove a stop, then cancel — the last preview restores the two-stop start.
    await tester.tap(find.byIcon(Icons.close).last);
    await tester.pumpAndSettle();
    expect(lastPreview!.length, 1);

    await tester.tap(find.text('Cancel'));
    await tester.pumpAndSettle();
    expect(lastPreview!.length, 2);
    expect(find.text('Palette'), findsNothing);
  });

  Future<List<List<LampColor>>> openPaletteSheet(WidgetTester tester) async {
    final changes = <List<LampColor>>[];
    await tester.pumpWidget(MaterialApp(
      theme: appTheme,
      home: Builder(
        builder: (ctx) => Scaffold(
          body: TextButton(
            onPressed: () => showColorStopsSheet(
              ctx,
              initial: const [_red, _green],
              title: 'Palette',
              max: 4,
              onChanged: changes.add,
              onSave: (_) async {},
            ),
            child: const Text('open'),
          ),
        ),
      ),
    ));
    await tester.tap(find.text('open'));
    await tester.pumpAndSettle();
    return changes;
  }

  testWidgets(
      'ColorStopsSheet swipe-down on a modified sheet reverts like Cancel',
      (tester) async {
    final changes = await openPaletteSheet(tester);

    await tester.tap(find.byIcon(Icons.close).last);
    await tester.pumpAndSettle();
    expect(changes.last, const [_red]);

    // A drag that starts on the list goes to its scroll recognizer and the
    // sheet never moves, so drag from the title.
    await tester.drag(find.text('Palette'), const Offset(0, 400));
    await tester.pumpAndSettle();

    expect(find.text('Palette'), findsNothing);
    expect(find.text('Discard changes?'), findsNothing);
    expect(changes.last, const [_red, _green]);
  });

  testWidgets('ColorStopsSheet swipe-down on an unmodified sheet sends nothing',
      (tester) async {
    final changes = await openPaletteSheet(tester);

    await tester.drag(find.text('Palette'), const Offset(0, 400));
    await tester.pumpAndSettle();

    expect(find.text('Palette'), findsNothing);
    expect(changes, isEmpty);
  });

  group('ColorStopsSheet topFirst', () {
    Future<List<List<LampColor>>> openTopFirstSheet(WidgetTester tester) async {
      tester.view.physicalSize = const Size(800, 1600);
      tester.view.devicePixelRatio = 1;
      addTearDown(tester.view.reset);
      final changes = <List<LampColor>>[];
      await tester.pumpWidget(MaterialApp(
        theme: appTheme,
        home: Builder(
          builder: (ctx) => Scaffold(
            body: TextButton(
              onPressed: () => showAppSheet<void>(
                ctx,
                builder: (_) => FractionallySizedBox(
                  heightFactor: 0.6,
                  child: ColorStopsSheet(
                    initial: const [_red, _green, _blue],
                    title: 'Palette',
                    max: 6,
                    topFirst: true,
                    onChanged: changes.add,
                    onSave: (_) async {},
                  ),
                ),
              ),
              child: const Text('open'),
            ),
          ),
        ),
      ));
      await tester.tap(find.text('open'));
      await tester.pumpAndSettle();
      return changes;
    }

    testWidgets('lists the last stored color first', (tester) async {
      await openTopFirstSheet(tester);

      expect(_rowHex(0, '#0000FF'), findsOneWidget);
      expect(_rowHex(1, '#00FF00'), findsOneWidget);
      expect(_rowHex(2, '#FF0000'), findsOneWidget);
    });

    testWidgets('dragging the top row below the second stores it reversed',
        (tester) async {
      final changes = await openTopFirstSheet(tester);

      final handle = find.descendant(
        of: find.byKey(const ValueKey('color-stop-0')),
        matching: find.byIcon(Icons.drag_indicator),
      );
      final rowHeight =
          tester.getSize(find.byKey(const ValueKey('color-stop-0'))).height;
      final gesture = await tester.startGesture(tester.getCenter(handle));
      for (var i = 0; i < 6; i++) {
        await gesture.moveBy(Offset(0, rowHeight / 6));
        await tester.pump(const Duration(milliseconds: 50));
      }
      await gesture.up();
      await tester.pumpAndSettle();

      expect(changes.last, const [_red, _blue, _green]);
      expect(_rowHex(0, '#00FF00'), findsOneWidget);
      expect(_rowHex(1, '#0000FF'), findsOneWidget);
    });

    testWidgets('removing the top row removes the last stored color',
        (tester) async {
      final changes = await openTopFirstSheet(tester);

      await tester.tap(find.descendant(
        of: find.byKey(const ValueKey('color-stop-0')),
        matching: find.byIcon(Icons.close),
      ));
      await tester.pumpAndSettle();

      expect(changes.last, const [_red, _green]);
    });

    testWidgets('Add inserts at storage index 0, seeded from it',
        (tester) async {
      final changes = await openTopFirstSheet(tester);

      await tester.tap(find.text('Add Color'));
      await tester.pumpAndSettle();

      expect(changes.last, const [_red, _red, _green, _blue]);
      expect(_rowHex(3, '#FF0000'), findsOneWidget);
    });

    testWidgets('picker title uses the display position', (tester) async {
      await openTopFirstSheet(tester);

      await tester.tap(find.byKey(const ValueKey('color-stop-0')));
      await tester.pumpAndSettle();

      expect(find.text('Stop 1'), findsOneWidget);
    });
  });
}
