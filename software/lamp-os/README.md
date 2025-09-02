# Lamp OS - C/Arduino Port

---
This is an unstable branch for development of a new C++ focused port of the lamp os project. It retains all the same hardware specs.

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

We're using google's code style for this application and for now all lamp specific code will be added to a flat `lamp` namespace.

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

copy the file `secrets_sample.hpp` to `secrets.hpp` and add any artnet credentials you want. otherwise leave it blank

## Flashing

 Currently the flash process is a few steps:
  - Pioarduino in the VSCode sidebar -> project tasks -> platform -> build filesystem image
  - Pioarduino in the VSCode sidebar -> project tasks -> platform -> upload filesystem image
  - Pioarduino in the VSCode sidebar -> project tasks -> general -> upload

 Note that depending on the board, you may need to hold the boot button in order to flash the board.
