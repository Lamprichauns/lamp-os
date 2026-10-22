import 'package:riverpod_annotation/riverpod_annotation.dart';

part 'clock.g.dart';

/// Wall-clock source for time-based UI. Tests override it to step time
/// without real waits.
@Riverpod(keepAlive: true, name: 'clockProvider')
DateTime Function() clock(Ref ref) => DateTime.now;
