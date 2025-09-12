# ArtNet Repeater

---
This is an unstable branch for development of an artnet repeater to control multiple lamps from one unicast ArtNet address with support for a single universe. This project uses wifi on an Seeed Xiao ESP32C6 board with a remote antenna. There will be more packet loss on chip antennas and more noticeable lag in animations as a result

## Contributing

This project runs on pioarduino, a VS Code plugin for all platforms. Pioarduino can be found in the VS Code marketplace.

## Prerequisites

You'll need the following software packages

* <https://www.python.org/downloads/> (At least version 3.10)
* Python 3.10+ added to your path
* <https://code.visualstudio.com/>
* pioarduino (accessed from VS Code's extensions tab)

## Opening the project

 Clone this branch to a local folder. Find the folder in the Pioarduino tab on the sidebar of VS Code

## Setting up your editor

We're using google's code style for this application

You can auto format your code by changing the vscode settings (ctrl + shift + p > Open user settings (json)):

```json
    "cSpell.diagnosticLevel": "Hint",
    "editor.formatOnSave": true,
    "editor.formatOnSaveMode": "file",
    "C_Cpp.vcFormat.newLine.beforeOpenBrace.namespace": "sameLine",
    "C_Cpp.vcFormat.newLine.beforeOpenBrace.type": "sameLine",
    "C_Cpp.vcFormat.newLine.scopeBracesOnSeparateLines": false,
    "C_Cpp.vcFormat.newLine.beforeOpenBrace.block": "sameLine",
    "C_Cpp.vcFormat.newLine.beforeOpenBrace.function": "sameLine",
    "C_Cpp.clang_format_fallbackStyle": "{ BasedOnStyle: Google, IndentWidth: 2 }"
```

## Project Setup

Copy the file `secrets_sample.hpp` to `secrets.hpp` and add any WiFi credentials you want. Otherwise leave it blank

## Flashing

Find and run the following task once Pioarduino has finished setup:

* Pioarduino in the VSCode sidebar -> project tasks -> general -> upload

 Note that depending on the board, you may need to hold the boot button in order to flash the board.
