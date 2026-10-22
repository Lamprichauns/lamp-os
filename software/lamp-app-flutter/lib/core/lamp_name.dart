import 'package:flutter/services.dart';

/// A lamp's name composes its setup WiFi SSID, its BLE advert name and its
/// mDNS host, so whitespace in one leaves the owner hunting for a network
/// that does not match the name the app shows. The firmware strips it on
/// write; these keep it out of the field to begin with.
final lampNameFormatters = <TextInputFormatter>[
  FilteringTextInputFormatter.deny(RegExp(r'\s')),
];
