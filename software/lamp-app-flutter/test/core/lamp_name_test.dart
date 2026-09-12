import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:lamp_app/core/lamp_name.dart';

/// Runs the name field's formatters over a typed value the way a TextField
/// would, so the rule is asserted rather than assumed from the widget tree.
String _typed(String text) {
  var value = TextEditingValue.empty;
  for (final f in lampNameFormatters) {
    value = f.formatEditUpdate(
      TextEditingValue.empty,
      TextEditingValue(text: text, selection: TextSelection.collapsed(offset: text.length)),
    );
  }
  return value.text;
}

void main() {
  test('a typed space never reaches the name', () {
    expect(_typed('gramp '), 'gramp');
    expect(_typed(' gramp'), 'gramp');
    expect(_typed('big jim'), 'bigjim');
  });

  test('a clean name is untouched', () {
    expect(_typed('gramp'), 'gramp');
    expect(_typed('lamp2'), 'lamp2');
  });

  test('tabs and newlines are whitespace too', () {
    expect(_typed('big\tjim'), 'bigjim');
    expect(_typed('big\njim'), 'bigjim');
  });
}
