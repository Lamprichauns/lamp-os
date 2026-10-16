import 'dart:math' as math;

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';

import '../../../../core/theme/app_spacing.dart';
import '../../../../core/widgets/app_sheet.dart';
import '../../../../core/widgets/confirm_discard.dart';
import '../../domain/color_harmony.dart';
import '../../domain/lamp_color.dart';
import 'color_picker_sheet.dart';

const double _kLockedOpacity = 0.4;

/// The surface's Auto-pick is on and uses this scheme. The sheet locks the
/// derived rows and regenerates them from the primary, the last storage
/// index; the flags themselves change only on the Color harmony screen.
class AutoPickConfig {
  const AutoPickConfig({required this.schemeId});
  final int schemeId;
}

/// Reusable bottom sheet for editing a list of color stops: full-width color
/// bar + hex per row, add / remove / (optional) reorder, tap-a-row to edit its
/// color, live-preview via [onChanged] as the user edits, and Save / Cancel
/// with a discard guard. The sheet owns its working list; [onChanged] streams
/// every in-flight change for preview and [onSave] commits.
class ColorStopsSheet extends StatefulWidget {
  const ColorStopsSheet({
    super.key,
    required this.initial,
    required this.title,
    required this.max,
    required this.onSave,
    this.description,
    this.allowEmpty = false,
    this.reorderable = true,
    this.bpp = 4,
    this.topFirst = false,
    this.autoPick,
    this.onChanged,
  });

  final List<LampColor> initial;
  final String title;
  final int max;

  /// Muted help line under the title explaining how these colors are used.
  final String? description;

  /// Empty list allowed (min stays at zero); otherwise the last stop can't be
  /// removed.
  final bool allowEmpty;

  /// Drag-to-reorder rows. False renders a plain list with no drag handles for
  /// callers whose colors are an unordered set.
  final bool reorderable;

  /// Strip depth for the color picker: 4 exposes the warm-white slider, 3 hides
  /// it.
  final int bpp;

  /// Lists the last stored color first and adds new colors at storage index
  /// 0, so a strip whose index 0 is its bottom reads top-down. [initial],
  /// [onChanged] and [onSave] stay in storage order.
  final bool topFirst;

  /// Read once when the sheet opens; later values are ignored.
  final AutoPickConfig? autoPick;

  /// Streams the working list on every edit for live preview. May be null.
  final ValueChanged<List<LampColor>>? onChanged;

  /// Commits the final list. Throws to signal a failed write (the sheet stays
  /// open and surfaces the error).
  final Future<void> Function(List<LampColor>) onSave;

  @override
  State<ColorStopsSheet> createState() => _ColorStopsSheetState();
}

class _ColorStopsSheetState extends State<ColorStopsSheet> {
  // Snapshot once. The parent re-watches lamp state and re-passes the
  // live-updated colors as `initial` on every rebuild, so the dirty check and
  // revert target must key off the colors the sheet opened with, not the prop.
  late final List<LampColor> _original = List.of(widget.initial);
  late List<LampColor> _colors = List.of(widget.initial);
  late final AutoPickConfig? _autoPick = widget.autoPick;
  bool _allowPop = false;

  int get _minCount => widget.allowEmpty ? 0 : 1;

  int get _maxCount => _autoPick == null
      ? widget.max
      : math.min(widget.max, kHarmonyMaxCount);

  bool get _dirty => !listEquals(_colors, _original);

  int _storageIndex(int position) =>
      widget.topFirst ? _colors.length - 1 - position : position;

  void _apply(List<LampColor> next) {
    setState(() => _colors = next);
    widget.onChanged?.call(List.of(next));
  }

  void _regenerate(List<LampColor> colors, int count) =>
      _apply(harmonizePalette(colors, _autoPick!.schemeId, count: count));

  // The generator clamps to kHarmonyMaxCount, so the primary's index can shift
  // during one picker session.
  void _setStop(int i, LampColor color) {
    if (_autoPick != null) {
      _regenerate([..._colors]..[_colors.length - 1] = color, _colors.length);
    } else if (i < _colors.length) {
      _apply([..._colors]..[i] = color);
    }
  }

  void _close({required bool revert}) {
    if (revert && _dirty) widget.onChanged?.call(List.of(_original));
    setState(() => _allowPop = true);
    // Pop after the frame so PopScope picks up _allowPop=true (setState +
    // synchronous Navigator.pop would still see the old canPop=false).
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (mounted) Navigator.of(context).pop();
    });
  }

  Future<void> _save() async {
    try {
      await widget.onSave(List.of(_colors));
    } catch (_) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text("Couldn't save: disconnected")),
      );
      return;
    }
    if (!mounted) return;
    _close(revert: false);
  }

  Future<void> _editStop(int i, {required int position}) async {
    final before = List.of(_colors);
    final picked = await showColorPickerSheet(
      context,
      initial: _colors[i],
      title: 'Stop ${position + 1}',
      bpp: widget.bpp,
      onLive: (live) => _setStop(i, live),
    );
    if (picked == null) {
      _apply(before);
    } else {
      _setStop(i, picked);
    }
  }

  void _removeStop(int i) {
    if (_colors.length <= _minCount) return;
    if (_autoPick == null) {
      _apply([..._colors]..removeAt(i));
    } else {
      _regenerate(_colors, _colors.length - 1);
    }
  }

  void _addStop() {
    if (_colors.length >= _maxCount) return;
    if (_autoPick != null) {
      _regenerate(_colors, _colors.length + 1);
      return;
    }
    // Seed from the neighbouring stop so the new one starts near a color the
    // user can tweak from.
    if (_colors.isEmpty) {
      _apply(const [LampColor(r: 0xFF, g: 0xFF, b: 0xFF, w: 0)]);
    } else if (widget.topFirst) {
      _apply([_colors.first, ..._colors]);
    } else {
      _apply([..._colors, _colors.last]);
    }
  }

  void _reorder(int oldIndex, int newIndex) {
    // The list's semantics actions still offer Move up / down on locked rows.
    if (_autoPick != null) return;
    final next = widget.topFirst ? _colors.reversed.toList() : [..._colors];
    next.insert(newIndex, next.removeAt(oldIndex));
    _apply(widget.topFirst ? next.reversed.toList() : next);
  }

  Widget _row(BuildContext ctx, int position) {
    final i = _storageIndex(position);
    final stop = _colors[i];
    final cs = Theme.of(ctx).colorScheme;
    final atMin = _colors.length <= _minCount;
    final locked = _autoPick != null;
    final isPrimary = i == _colors.length - 1;
    final derived = locked && !isPrimary;
    final dimmed = cs.onSurfaceVariant.withValues(alpha: _kLockedOpacity);
    final key = ValueKey('color-stop-$position');
    final tile = ListTile(
      key: derived ? null : key,
      onTap: derived ? null : () => _editStop(i, position: position),
      leading: widget.reorderable
          ? ReorderableDragStartListener(
              index: position,
              enabled: !locked,
              child: Icon(
                Icons.drag_indicator,
                color: locked ? dimmed : cs.onSurfaceVariant,
              ),
            )
          : null,
      title: Row(
        children: [
          Expanded(
            child: Container(
              height: 36, // deliberate dimension, not spacing
              decoration: BoxDecoration(
                color: stop.toSwatch(),
                borderRadius: BorderRadius.circular(AppRadius.swatch),
                border: Border.all(
                  color: cs.outlineVariant,
                  width: 1, // deliberate dimension, not spacing
                ),
              ),
            ),
          ),
          const SizedBox(width: AppSpace.md),
          Text(
            '#${stop.toHex().substring(1, 7)}',
            style: Theme.of(ctx).textTheme.bodySmall?.copyWith(
                  color: derived ? dimmed : cs.onSurfaceVariant,
                  fontFamily: 'monospace',
                ),
          ),
        ],
      ),
      trailing: locked && isPrimary
          ? const SizedBox(width: kMinInteractiveDimension)
          : Tooltip(
              message: atMin
                  ? (widget.allowEmpty
                      ? 'Remove color'
                      : 'Keep at least one stop')
                  : 'Remove stop',
              child: IconButton(
                icon: Icon(Icons.close, color: cs.onSurfaceVariant),
                onPressed: atMin ? null : () => _removeStop(i),
              ),
            ),
    );
    if (!derived) return tile;
    final lockedLabel = 'Auto-picked from the '
        '${widget.topFirst ? 'top' : 'bottom'} color. '
        'Turn off Auto-pick in Setup to edit.';
    return Semantics(
      key: key,
      label: lockedLabel,
      child: Tooltip(
        message: lockedLabel,
        excludeFromSemantics: true,
        child: tile,
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return PopScope(
      canPop: _allowPop || !_dirty,
      onPopInvokedWithResult: (didPop, _) async {
        if (didPop) {
          // A swipe-down pops without consulting canPop, so revert here.
          if (!_allowPop && _dirty) widget.onChanged?.call(List.of(_original));
          return;
        }
        final discard = await confirmDiscard(context);
        if (discard) _close(revert: true);
      },
      child: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(AppSpace.lg),
          child: Column(
            children: [
              Row(
                children: [
                  Expanded(
                    child: Text(
                      widget.title,
                      style: Theme.of(context).textTheme.titleMedium,
                    ),
                  ),
                  if (_colors.length < _maxCount)
                    TextButton.icon(
                      onPressed: _addStop,
                      icon: const Icon(Icons.add, size: 18),
                      label: const Text('Add Color'),
                    ),
                ],
              ),
              if (widget.description != null) ...[
                const SizedBox(height: AppSpace.xs),
                Align(
                  alignment: Alignment.centerLeft,
                  child: Text(
                    widget.description!,
                    style: Theme.of(context).textTheme.bodySmall?.copyWith(
                          color: Theme.of(context).colorScheme.onSurfaceVariant,
                        ),
                  ),
                ),
              ],
              const SizedBox(height: AppSpace.md),
              Expanded(
                child: widget.reorderable
                    ? ReorderableListView.builder(
                        itemCount: _colors.length,
                        onReorderItem: _reorder,
                        buildDefaultDragHandles: false,
                        itemBuilder: _row,
                      )
                    : ListView.builder(
                        itemCount: _colors.length,
                        itemBuilder: _row,
                      ),
              ),
              const SizedBox(height: AppSpace.sm),
              Row(
                children: [
                  TextButton(
                    onPressed: () => _close(revert: true),
                    child: const Text('Cancel'),
                  ),
                  const Spacer(),
                  FilledButton.icon(
                    icon: const Icon(Icons.check, size: 18),
                    label: const Text('Save'),
                    onPressed: _save,
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }
}

/// Opens [ColorStopsSheet] as a modal bottom sheet. Mirrors
/// [showColorPickerSheet].
Future<void> showColorStopsSheet(
  BuildContext context, {
  required List<LampColor> initial,
  required String title,
  required int max,
  required Future<void> Function(List<LampColor>) onSave,
  String? description,
  bool allowEmpty = false,
  bool reorderable = true,
  int bpp = 4,
  ValueChanged<List<LampColor>>? onChanged,
}) {
  return showAppSheet<void>(
    context,
    builder: (ctx) => FractionallySizedBox(
      heightFactor: 0.6,
      child: ColorStopsSheet(
        initial: initial,
        title: title,
        max: max,
        onSave: onSave,
        description: description,
        allowEmpty: allowEmpty,
        reorderable: reorderable,
        bpp: bpp,
        onChanged: onChanged,
      ),
    ),
  );
}
