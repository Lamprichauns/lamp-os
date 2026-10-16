import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../../../core/widgets/app_sheet.dart';
import '../../application/control_notifier.dart';
import 'color_stops_sheet.dart';

/// Hard cap on base gradient stops. Firmware renders at most 8; 6 keeps
/// the row list scannable.
const int _kMaxBaseStops = 6;

/// Modal sheet for editing the base gradient stops. A thin [ColorStopsSheet]
/// caller: live-previews via `setBaseColors` as the user edits and commits
/// through CHAR_COMMIT on Save. ControlScreen drives the base wisp edit-session
/// around the whole sheet.
class BaseEditorSheet extends ConsumerWidget {
  const BaseEditorSheet({super.key, required this.lampId, this.title = 'Base'});

  final String lampId;
  final String title;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final base = ref.watch(
      controlNotifierProvider(lampId).select((async) => async.value?.base),
    );
    if (base == null) return const SizedBox.shrink();

    final notifier = ref.read(controlNotifierProvider(lampId).notifier);
    final autoPick = base.harmonyEligible && base.autoColors
        ? AutoPickConfig(schemeId: base.colorScheme)
        : null;
    return ColorStopsSheet(
      initial: base.colors,
      title: '$title colors',
      description: autoPick != null
          ? 'Auto-pick is on. The other colors follow the bottom color; change '
              'the scheme in Setup.'
          : 'These colors blend into a gradient along the strip.',
      max: _kMaxBaseStops,
      bpp: base.bpp,
      autoPick: autoPick,
      onChanged: (colors) => notifier.setBaseColors(colors),
      onSave: (colors) async {
        await notifier.setBaseColors(colors);
        await notifier.commit();
      },
    );
  }
}

Future<void> showBaseEditorSheet(
  BuildContext context, {
  required String lampId,
  String title = 'Base',
}) {
  return showAppSheet<void>(
    context,
    builder: (ctx) => FractionallySizedBox(
      heightFactor: 0.6,
      child: BaseEditorSheet(lampId: lampId, title: title),
    ),
  );
}
