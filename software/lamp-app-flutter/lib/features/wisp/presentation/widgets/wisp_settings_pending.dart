import 'package:flutter/material.dart';

import '../../../../core/theme/app_spacing.dart';

/// Placeholder for a settings control while the wisp's real settings have
/// not arrived ([WispStatus.configKnown] false). Stands in for the control
/// so nothing a save could write back is ever shown as a default.
class WispSettingsPending extends StatelessWidget {
  const WispSettingsPending({super.key});

  @override
  Widget build(BuildContext context) {
    return Padding(
      key: const Key('wisp-settings-pending'),
      padding: const EdgeInsets.symmetric(vertical: AppSpace.md),
      child: Row(
        children: [
          const SizedBox(
            width: 14,
            height: 14, // deliberate dimension, not spacing
            child: CircularProgressIndicator(strokeWidth: 2),
          ),
          const SizedBox(width: AppSpace.md),
          Text(
            'Reading settings from wisp…',
            style: Theme.of(
              context,
            ).textTheme.bodySmall?.copyWith(fontStyle: FontStyle.italic),
          ),
        ],
      ),
    );
  }
}
