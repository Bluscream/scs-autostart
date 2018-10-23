# SCS Software Autostart plugin

## Description

This plugin allows you to let SCS games run apps for you on certain events. I figured this is much easier than writing actual plugins.

## Installation

1. Download the latest installer from the [releases](https://github.com/Bluscream/scs-autostart/releases) page.
2. Run `scs-autostart-plugin-installer.exe` and set the path to your SCS game (Example: `C:\Program Files (x86)\Steam\Steamapps\common\Euro Truck Simulator 2\`).
3. Start the game once until you accepted the SDK warning to have the config generated.
4. Edit the configuration file located at `<Euro Truck Simulator 2>\bin\win_x64\plugins\autoStart.ini` to your needs.
5. Start the game and enjoy

## Configuration

```ini
[AutoStarts]
after_sdk_warning=plugins/season_changer/Season Changer.exe,C:/test.exe,https://ets2map.com
on_sdk_config=
on_game_pause=
on_game_resume=
on_game_exit=
```

## Known Issues

- Currently only works with ETS 2
- Currently only works on Windows
