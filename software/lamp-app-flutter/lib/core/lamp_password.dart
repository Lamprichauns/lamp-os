/// The lamp keys its setup WiFi network with the control password, and WPA2
/// needs an 8..63 character key, so anything shorter leaves the lamp's web
/// config reachable by anyone nearby. The 16 upper bound matches the web
/// config UI's own field.
const int kLampPasswordMinLength = 8;
const int kLampPasswordMaxLength = 16;

/// Printable ASCII only. A WPA2 key is PBKDF2 over the raw passphrase bytes,
/// and two devices can disagree on the Unicode normalization of the same
/// typed accent, which would leave the owner unable to join their own lamp.
final _printableAscii = RegExp(r'^[ -~]+$');

/// Null when [password] is usable, otherwise the reason to show inline.
String? lampPasswordError(String password) {
  if (password.length < kLampPasswordMinLength) {
    return 'Must be at least $kLampPasswordMinLength characters';
  }
  if (password.length > kLampPasswordMaxLength) {
    return 'Must be at most $kLampPasswordMaxLength characters';
  }
  if (!_printableAscii.hasMatch(password)) {
    return 'Letters, numbers and symbols only';
  }
  return null;
}
