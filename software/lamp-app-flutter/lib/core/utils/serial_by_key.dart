/// Runs operations one at a time per key, in the order [run] is called. An
/// operation takes its place during the [run] call itself. A failure reaches
/// only its own caller; the key's next operation still runs.
class SerialByKey {
  final Map<Object, Future<void>> _tails = {};

  Future<T> run<T>(Object key, Future<T> Function() op) {
    final result = (_tails[key] ?? Future<void>.value()).then((_) => op());
    _tails[key] = result.then((_) {}, onError: (_) {});
    return result;
  }
}
