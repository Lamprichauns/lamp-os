import 'package:flutter/material.dart';

import '../../../../core/theme/app_spacing.dart';
import '../../../../core/widgets/section_header.dart';
import '../../domain/wisp_source_mode.dart';

/// Inline notice shown under the Straylight source pill while the wisp has
/// no live palette stream. The wisp holds the lamps at Off meanwhile and
/// resumes painting on its own when palettes arrive.
class WaitingForStraylightNotice extends StatelessWidget {
  const WaitingForStraylightNotice({super.key});

  @override
  Widget build(BuildContext context) {
    final secondary = Theme.of(context).colorScheme.secondary;
    return Container(
      padding: const EdgeInsets.all(AppSpace.md),
      decoration: BoxDecoration(
        color: secondary.withValues(alpha: 0.10),
        border: Border.all(
          color: secondary.withValues(alpha: 0.40),
        ),
        borderRadius: BorderRadius.circular(AppSpace.sm),
      ),
      child: Row(
        children: [
          Icon(
            Icons.graphic_eq,
            size: 16, // deliberate dimension, not spacing
            color: secondary,
          ),
          const SizedBox(width: AppSpace.sm),
          Expanded(
            child: Text(
              'Waiting for Straylight.\n'
              'Lamps stay on their own colours until a palette arrives.',
              style: Theme.of(context).textTheme.bodySmall,
            ),
          ),
        ],
      ),
    );
  }
}

/// Picks which Straylight palette role the wisp follows.
class OscRolePicker extends StatelessWidget {
  const OscRolePicker({
    super.key,
    required this.current,
    required this.onSelect,
  });

  final OscRole current;
  final ValueChanged<OscRole> onSelect;

  static String _label(OscRole r) {
    switch (r) {
      case OscRole.primary:
        return 'Primary';
      case OscRole.secondary:
        return 'Secondary';
      case OscRole.tertiary:
        return 'Tertiary';
      case OscRole.fill:
        return 'Fill';
    }
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const SectionHeader('Straylight palette'),
        const SizedBox(height: AppSpace.sm),
        SegmentedButton<OscRole>(
          key: const Key('wisp-osc-role-picker'),
          style: SegmentedButton.styleFrom(
            shape: RoundedRectangleBorder(
              borderRadius: BorderRadius.circular(AppRadius.card),
            ),
          ),
          segments: [
            for (final r in OscRole.values)
              ButtonSegment(value: r, label: Text(_label(r))),
          ],
          selected: {current},
          showSelectedIcon: false,
          onSelectionChanged: (set) {
            if (set.isEmpty) return;
            onSelect(set.first);
          },
        ),
      ],
    );
  }
}
