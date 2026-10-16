import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';

import '../../../../core/routing/routes.dart';
import '../../domain/color_harmony.dart';
import '../../domain/lamp_color.dart';

/// Home card line shown while a surface auto-picks its colors. The scheme
/// name links to the Color harmony screen. A neutral [primary] can't seed a
/// scheme, so the line asks for a more colorful [primaryPosition] color
/// instead ("top" or "bottom", where the primary sits on that surface).
class AutoPickNote extends StatelessWidget {
  const AutoPickNote({
    super.key,
    required this.lampId,
    required this.primary,
    required this.schemeId,
    required this.primaryPosition,
  });

  final String lampId;
  final LampColor primary;
  final int schemeId;
  final String primaryPosition;

  @override
  Widget build(BuildContext context) {
    final style = Theme.of(context).textTheme.bodySmall?.copyWith(
          color: Theme.of(context).colorScheme.onSurfaceVariant,
        );
    if (isNeutralColor(primary)) {
      return Text(
        'Auto-picked · pick a more colorful $primaryPosition color',
        style: style,
      );
    }
    return Wrap(
      crossAxisAlignment: WrapCrossAlignment.center,
      children: [
        Text('Auto-picked · ', style: style),
        TextButton(
          key: const Key('auto-pick-link'),
          style: TextButton.styleFrom(
            padding: EdgeInsets.zero,
            minimumSize: Size.zero,
            visualDensity: VisualDensity.compact,
            textStyle: Theme.of(context).textTheme.bodySmall,
          ),
          onPressed: () => context.push(AppRoutes.colorHarmony(lampId)),
          child: Text(harmonySchemeById(schemeId).name),
        ),
      ],
    );
  }
}
