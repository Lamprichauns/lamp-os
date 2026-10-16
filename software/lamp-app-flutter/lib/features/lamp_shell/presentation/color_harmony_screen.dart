import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../../core/theme/app_spacing.dart';
import '../../../core/widgets/app_snackbar.dart';
import '../../../core/widgets/back_button_leading.dart';
import '../../../core/widgets/empty_state_pane.dart';
import '../../../core/widgets/friendly_error.dart';
import '../../../core/widgets/info_panel.dart';
import '../../../core/widgets/section_header.dart';
import '../../control/application/control_notifier.dart';
import '../../control/application/control_state.dart';
import '../../control/domain/color_harmony.dart';
import '../../control/domain/lamp_color.dart';
import '../../control/presentation/widgets/lamp_color_swatch.dart';

/// Auto-pick settings: which surfaces auto-pick their colors and the one
/// scheme they share. Every change writes to the lamp immediately.
class ColorHarmonyScreen extends ConsumerStatefulWidget {
  const ColorHarmonyScreen({super.key, required this.lampId});
  final String lampId;

  @override
  ConsumerState<ColorHarmonyScreen> createState() =>
      _ColorHarmonyScreenState();
}

class _ColorHarmonyScreenState extends ConsumerState<ColorHarmonyScreen> {
  bool _open = false;

  ControlNotifier _notifier() =>
      ref.read(controlNotifierProvider(widget.lampId).notifier);

  Future<void> _onSurfacesChanged(
    Set<HarmonySurface> previous,
    Set<HarmonySurface> next,
  ) async {
    final n = _notifier();
    for (final surface in HarmonySurface.values) {
      final on = next.contains(surface);
      if (on == previous.contains(surface)) continue;
      try {
        await n.setAutoPick(surface, on);
      } catch (_) {
        if (mounted) {
          AppSnackbar.error(context, "Couldn't update Auto-pick. Try again.");
        }
        return;
      }
    }
  }

  Future<void> _onSchemePicked(int schemeId) async {
    try {
      await _notifier().pickColorScheme(schemeId);
    } catch (_) {
      if (mounted) {
        AppSnackbar.error(context, "Couldn't change the scheme. Try again.");
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final async = ref.watch(controlNotifierProvider(widget.lampId));
    return Scaffold(
      appBar: AppBar(
        leading: const BackButtonLeading(),
        title: const Text('Color harmony'),
      ),
      body: async.when(
        loading: () => const SizedBox.expand(),
        error: (e, _) => FriendlyError.page(
          title: "Couldn't reach your lamp.",
          subtitle:
              "They may have wandered out of range. Bring your phone closer "
              'and try again.',
          rawError: e,
          onRetry: () =>
              ref.invalidate(controlNotifierProvider(widget.lampId)),
        ),
        data: _buildBody,
      ),
    );
  }

  Widget _buildBody(ControlState state) {
    final colorScheme = Theme.of(context).colorScheme;
    final textTheme = Theme.of(context).textTheme;
    final shadeEligible = state.shade.harmonyEligible;
    final baseEligible = state.base.harmonyEligible;
    if (!shadeEligible && !baseEligible) {
      return EmptyStatePane(
        icon: Icon(
          Icons.palette_outlined,
          size: 56, // deliberate dimension, not spacing
          color: colorScheme.onSurfaceVariant,
        ),
        title: 'No Auto-pick on this lamp',
        subtitle: "This lamp's colors are set by its firmware.",
      );
    }

    final shadeOn = shadeEligible && state.shade.autoColors;
    final baseOn = baseEligible && state.base.autoColors;
    final selected = {
      if (shadeOn) HarmonySurface.shade,
      if (baseOn) HarmonySurface.base,
    };
    final selectedId = sharedSchemeId(
      shadeScheme: state.shade.colorScheme,
      baseScheme: state.base.colorScheme,
      shadeEligible: shadeEligible,
      baseEligible: baseEligible,
    );
    final previewShade = shadeOn || (!baseOn && shadeEligible);
    final previewPrimary =
        (previewShade ? state.shade.colors : state.base.colors).lastOrNull;
    List<LampColor> swatchesFor(HarmonyScheme scheme) => previewPrimary == null
        ? const []
        : harmonyColors(previewPrimary, scheme.id, scheme.naturalCount)
            .reversed
            .toList();

    return ListView(
      padding: const EdgeInsets.all(AppSpace.lg),
      children: [
        const InfoPanel(
          child: Text(
            "Auto-pick fills in a surface's other colors from one of them: "
            "the Shade's top color and the Base's bottom color. Pick the "
            'surfaces, then the scheme.',
          ),
        ),
        const SizedBox(height: AppSpace.lg),
        const SectionHeader('Auto-pick colors for'),
        const SizedBox(height: AppSpace.sm),
        SizedBox(
          width: double.infinity,
          child: SegmentedButton<HarmonySurface>(
            key: const Key('auto-pick-surfaces'),
            multiSelectionEnabled: true,
            emptySelectionAllowed: true,
            showSelectedIcon: false,
            style: SegmentedButton.styleFrom(
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(AppRadius.card),
              ),
            ),
            segments: [
              if (shadeEligible)
                const ButtonSegment(
                  value: HarmonySurface.shade,
                  label: Text('Shade'),
                ),
              if (baseEligible)
                const ButtonSegment(
                  value: HarmonySurface.base,
                  label: Text('Base'),
                ),
            ],
            selected: selected,
            onSelectionChanged: (next) => _onSurfacesChanged(selected, next),
          ),
        ),
        const SizedBox(height: AppSpace.xl),
        const SectionHeader('Scheme'),
        const SizedBox(height: AppSpace.sm),
        Row(
          key: const Key('scheme-summary'),
          children: [
            Expanded(
              child: Text(
                selectedId == null
                    ? 'Mixed'
                    : harmonySchemeById(selectedId).name,
                style: textTheme.bodyMedium,
              ),
            ),
            if (selectedId != null)
              _SchemeSwatches(swatchesFor(harmonySchemeById(selectedId))),
            const SizedBox(width: AppSpace.sm),
            TextButton(
              key: const Key('scheme-toggle'),
              onPressed: () => setState(() => _open = !_open),
              child: Text(_open ? 'Hide' : 'Change'),
            ),
          ],
        ),
        if (selectedId == null)
          Text(
            'Shade uses ${harmonySchemeById(state.shade.colorScheme).name}, '
            'Base uses ${harmonySchemeById(state.base.colorScheme).name}. '
            'Picking a scheme sets both.',
            style: textTheme.bodySmall
                ?.copyWith(color: colorScheme.onSurfaceVariant),
          ),
        if (_open)
          for (final scheme in kHarmonySchemes)
            ListTile(
              key: Key('harmony-scheme-${scheme.key}'),
              contentPadding: EdgeInsets.zero,
              title: Text(scheme.name, style: textTheme.bodyMedium),
              subtitle: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    scheme.description,
                    style: textTheme.bodySmall
                        ?.copyWith(color: colorScheme.onSurfaceVariant),
                  ),
                  const SizedBox(height: AppSpace.xs),
                  _SchemeSwatches(swatchesFor(scheme)),
                ],
              ),
              trailing: scheme.id == selectedId
                  ? Icon(Icons.check, color: colorScheme.primary)
                  : null,
              onTap: () => _onSchemePicked(scheme.id),
            ),
      ],
    );
  }
}

class _SchemeSwatches extends StatelessWidget {
  const _SchemeSwatches(this.colors);
  final List<LampColor> colors;

  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisSize: MainAxisSize.min,
      spacing: AppSpace.xs,
      children: [
        for (final c in colors)
          LampColorSwatch(
            color: c,
            size: 16, // deliberate dimension, not spacing
            borderRadius: 2, // deliberate dimension, not spacing
          ),
      ],
    );
  }
}
