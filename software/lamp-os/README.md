# Lamp OS

---
This is the main software aboard the lamps. It's a small LED driver focused on slow animations and social behaviors

## Contributing

This project runs on pioarduino, a VS Code plugin for all platforms. Pioarduino can be found in the VS Code marketplace

## Prerequisites

You'll need the following software packages

* <https://www.python.org/downloads/> (At least version 3.10)
* Python 3.10+ added to your path
* <https://code.visualstudio.com/>
* pioarduino (accessed from VS Code's extensions tab)

## Opening the project

 Clone this branch to a local folder. Find the folder in the Pioarduino tab on the sidebar of VS Code

## Setting up your editor

We're using Google's code style for this application. All lamp specific code will be added to a flat `lamp` namespace.

You can auto format your code by changing the Vscode settings (ctrl + shift + p > Open user settings (json)):

```json
    "cSpell.diagnosticLevel": "Hint",
    "editor.formatOnSave": true,
    "editor.formatOnSaveMode": "file",
    "C_Cpp.clang_format_fallbackStyle": "{ BasedOnStyle: Google, IndentWidth: 2, ColumnLimit: 0}"
```

## Flashing

 Currently the flash process is a few steps:

 If you've got a completely new board, it's prudent to run erase flash to completely clear your device:

* Pioarduino in the VSCode sidebar -> Project Tasks -> Platform -> Erase Flash

To push the web app in the data dir, you can run

* Pioarduino in the VSCode sidebar -> Project Tasks -> Platform -> build filesystem image
* Pioarduino in the VSCode sidebar -> Project Tasks -> Platform -> upload filesystem image

To upload the factory firmware image:

* Pioarduino in the VSCode sidebar -> Project Tasks -> General -> upload

 Note that depending on the board, you may need to hold the boot button in order to flash the board.

## Updating the web app for development

Plug your lamp board into a usb port

In VSCode, navigate to `Pioarduino > Quick Access > Miscellaneous > pioarduino core CLI`

This will bring up a window in the correct environment to upload

```bash
cd ../lamp-ui
npm ci
npm run build:upload
```

This process will build a new .spiffs partition and replace it onboard your esp32.

## Distribution

While you can program boards through the IDE, Lamp OS intends to be deployed on the web so anyone can format and maintain their own firmware

## OTA Updating

Join your lamp's wifi and navigate to <http://lamp.local/update> and update the following two files in pairs:

Lamp's main firmware:

* file: firmware.bin
* OTA Mode: Firmware

Lamp's mobile UI:

* file: spiffs.bin
* OTA Mode: LittleFS/Spiffs
