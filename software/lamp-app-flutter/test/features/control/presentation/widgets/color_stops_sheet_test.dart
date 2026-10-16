import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:lamp_app/core/theme/app_theme.dart';
import 'package:lamp_app/core/widgets/app_sheet.dart';
import 'package:lamp_app/features/control/domain/color_harmony.dart';
import 'package:lamp_app/features/control/domain/lamp_color.dart';
import 'package:lamp_app/features/control/presentation/widgets/color_blocks_bar.dart';
import 'package:lamp_app/features/control/presentation/widgets/color_stops_sheet.dart';

const _red = LampColor(r: 0xFF, g: 0, b: 0, w: 0);
const _green = LampColor(r: 0, g: 0xFF, b: 0, w: 0);
const _blue = LampColor(r: 0, g: 0, b: 0xFF, w: 0);
const _orange = LampColor(r: 0xFF, g: 0x80, b: 0, w: 0x20);
const _triadic = 4;
const _topLockLabel =
    'Auto-picked from the top color. Turn off Auto-pick in Setup to edit.';
const _bottomLockLabel =
    'Auto-picked from the bottom color. Turn off Auto-pick in Setup to edit.';

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

  group('ColorStopsSheet autoPick', () {
    // Storage order; the primary is the last index (_red).
    const opened = [_green, _blue, _red];

    Future<List<List<LampColor>>> openSheet(
      WidgetTester tester, {
      List<LampColor> initial = opened,
      bool topFirst = true,
      AutoPickConfig? autoPick = const AutoPickConfig(schemeId: _triadic),
    }) async {
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
                    initial: initial,
                    title: 'Palette',
                    max: 6,
                    topFirst: topFirst,
                    autoPick: autoPick,
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

    Finder rowClose(int position) => find.descendant(
          of: find.byKey(ValueKey('color-stop-$position')),
          matching: find.byIcon(Icons.close),
        );

    Future<void> enterPickerHex(WidgetTester tester, String hex) async {
      await tester.enterText(find.byType(TextField), hex);
      await tester.pump();
    }

    testWidgets('a primary edit regenerates the palette, live and on Done',
        (tester) async {
      final changes = await openSheet(tester);
      final expected = harmonizePalette(const [_green, _blue, _orange], _triadic);

      await tester.tap(find.byKey(const ValueKey('color-stop-0')));
      await tester.pumpAndSettle();
      await enterPickerHex(tester, '#FF800020');
      expect(changes.last, expected);

      await tester.tap(find.text('Done'));
      await tester.pumpAndSettle();
      expect(changes.last, expected);
      expect(changes.last.last, _orange);
      expect(_rowHex(1, expected[1].toHex().substring(0, 7)), findsOneWidget);
    });

    testWidgets('a palette above the cap keeps every primary edit after shrink',
        (tester) async {
      final changes = await openSheet(
        tester,
        initial: const [_green, _blue, _green, _blue, _green, _red],
      );

      await tester.tap(find.byKey(const ValueKey('color-stop-0')));
      await tester.pumpAndSettle();
      await enterPickerHex(tester, '#00FF0000');
      await enterPickerHex(tester, '#FF800020');
      expect(changes.last, harmonizePalette(const [_orange], _triadic, count: 5));

      await tester.tap(find.text('Done'));
      await tester.pumpAndSettle();
      expect(changes.last, hasLength(5));
      expect(changes.last.last, _orange);
    });

    testWidgets('without autoPick a primary edit changes only the primary',
        (tester) async {
      final changes = await openSheet(tester, autoPick: null);

      await tester.tap(find.byKey(const ValueKey('color-stop-0')));
      await tester.pumpAndSettle();
      await enterPickerHex(tester, '#FF800020');
      await tester.tap(find.text('Done'));
      await tester.pumpAndSettle();

      expect(changes.last, const [_green, _blue, _orange]);
    });

    testWidgets('locks derived rows and the primary row remove button',
        (tester) async {
      final changes = await openSheet(tester);

      await tester.tap(find.byKey(const ValueKey('color-stop-1')));
      await tester.pumpAndSettle();
      expect(find.text('Done'), findsNothing);

      expect(rowClose(0), findsNothing);
      expect(rowClose(1), findsOneWidget);
      expect(rowClose(2), findsOneWidget);
      expect(find.byTooltip(_topLockLabel), findsNWidgets(2));
      // The row merges the label with its hex once; a second source would
      // repeat it.
      expect(
        find.bySemanticsLabel(
          RegExp('^${RegExp.escape(_topLockLabel)}\\n#[0-9A-F]{6}\$'),
        ),
        findsNWidgets(2),
      );

      final handles = tester.widgetList<ReorderableDragStartListener>(
        find.byType(ReorderableDragStartListener),
      );
      expect(handles, hasLength(opened.length));
      expect(handles.every((h) => !h.enabled), isTrue);

      // Screen-reader Move actions reach onReorderItem past the handles.
      tester
          .widget<ReorderableListView>(find.byType(ReorderableListView))
          .onReorderItem!(0, 1);
      await tester.pumpAndSettle();
      expect(changes, isEmpty);

      // The primary's placeholder keeps its hex aligned with the other rows.
      expect(
        tester.getTopRight(_rowHex(0, '#FF0000')).dx,
        tester.getTopRight(_rowHex(1, '#0000FF')).dx,
      );
    });

    testWidgets('Add and remove while on change the count and regenerate',
        (tester) async {
      final changes = await openSheet(tester);

      await tester.tap(find.text('Add Color'));
      await tester.pumpAndSettle();
      expect(changes.last, harmonizePalette(opened, _triadic, count: 4));

      await tester.tap(find.text('Add Color'));
      await tester.pumpAndSettle();
      expect(changes.last, harmonizePalette(opened, _triadic, count: 5));
      expect(find.text('Add Color'), findsNothing);

      await tester.tap(rowClose(2));
      await tester.pumpAndSettle();
      expect(changes.last, harmonizePalette(opened, _triadic, count: 4));
      expect(changes.last.last, _red);
    });

    testWidgets('opening shows saved colors and ignores later props',
        (tester) async {
      tester.view.physicalSize = const Size(800, 1600);
      tester.view.devicePixelRatio = 1;
      addTearDown(tester.view.reset);
      // Saved colors that the generator would not produce from _red.
      final props = ValueNotifier<
          (List<LampColor>, String, AutoPickConfig?)>(
        (opened, 'Palette', const AutoPickConfig(schemeId: _triadic)),
      );
      addTearDown(props.dispose);
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
                  child: ValueListenableBuilder(
                    valueListenable: props,
                    builder: (_, p, _) => ColorStopsSheet(
                      initial: p.$1,
                      title: p.$2,
                      max: 6,
                      topFirst: true,
                      autoPick: p.$3,
                      onChanged: changes.add,
                      onSave: (_) async {},
                    ),
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

      expect(changes, isEmpty);
      expect(_rowHex(0, '#FF0000'), findsOneWidget);
      expect(_rowHex(1, '#0000FF'), findsOneWidget);
      expect(_rowHex(2, '#00FF00'), findsOneWidget);

      props.value = (const [_orange, _orange], 'Renamed', null);
      await tester.pumpAndSettle();

      expect(find.text('Renamed'), findsOneWidget);
      expect(changes, isEmpty);
      expect(_rowHex(0, '#FF0000'), findsOneWidget);
      expect(_rowHex(1, '#0000FF'), findsOneWidget);
      expect(_rowHex(2, '#00FF00'), findsOneWidget);
      await tester.tap(find.byKey(const ValueKey('color-stop-1')));
      await tester.pumpAndSettle();
      expect(find.text('Done'), findsNothing);
      expect(rowClose(0), findsNothing);
    });

    testWidgets('picker Cancel restores the list from before the picker',
        (tester) async {
      final changes = await openSheet(tester);

      await tester.tap(find.byKey(const ValueKey('color-stop-0')));
      await tester.pumpAndSettle();
      await enterPickerHex(tester, '#FF800020');
      expect(
        changes.last,
        harmonizePalette(const [_green, _blue, _orange], _triadic),
      );

      await tester.tap(find.text('Cancel').last);
      await tester.pumpAndSettle();
      expect(changes.last, opened);
    });

    testWidgets('without topFirst the primary is the last row', (tester) async {
      final changes = await openSheet(tester, topFirst: false);

      expect(_rowHex(2, '#FF0000'), findsOneWidget);
      expect(rowClose(2), findsNothing);
      expect(rowClose(0), findsOneWidget);
      expect(find.byTooltip(_bottomLockLabel), findsNWidgets(2));
      expect(find.byTooltip(_topLockLabel), findsNothing);

      await tester.tap(find.byKey(const ValueKey('color-stop-0')));
      await tester.pumpAndSettle();
      expect(find.text('Done'), findsNothing);

      await tester.tap(find.byKey(const ValueKey('color-stop-2')));
      await tester.pumpAndSettle();
      await enterPickerHex(tester, '#FF800020');
      await tester.tap(find.text('Done'));
      await tester.pumpAndSettle();

      final expected =
          harmonizePalette(const [_green, _blue, _orange], _triadic);
      expect(changes.last, expected);
      expect(changes.last.last, _orange);
      expect(_rowHex(2, '#FF8000'), findsOneWidget);
      expect(_rowHex(0, expected[0].toHex().substring(0, 7)), findsOneWidget);
    });
  });
}
