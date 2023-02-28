# MinUI

MinUI is a focused custom launcher for the Anbernic RG35XX (and possibly others to come).

<img src="github/minui-main.png" width=320 /> <img src="github/minui-menu-gbc.png" width=320 />  
See [more screenshots](github/).

## Features

- Simple launcher, simple SD card
- No settings or configuration
- No boxart, themes, or distractions
- Automatically hides extension 
  and region/version cruft in 
  display names
- Consistent in-emulator menu with
  quick access to save states, disc
  changing, and emulator options
- Automatically sleeps after 30 seconds 
  or press POWER to sleep (and wake)
- Automatically powers off while asleep
  after two minutes or hold POWER for
  one second
- Automatically resumes right where
  you left off if powered off while
  in-game, manually or while asleep
- Resume from manually created, last 
  used save state by pressing X in 
  the launcher instead of A
- Streamlined emulator frontend 
  (minarch + libretro cores)
  
## Install

1. Copy "dmenu.bin" to the root
of the MISC partition of the SD card
that goes in the TF1 slot.

2. Where you copy "MinUI.zip"
will depend on if you are using one or
two SD cards (using two SD cards is
recommended). If using one SD card,
copy "MinUI.zip" to the root of the
ROMS partition of the SD card that
goes in the TF1 slot. Otherwise copy
"MinUI.zip" to the root of the SD card
that goes in the TF2 slot.

During installation (but not on
subsequent updates) MinUI will
install a custom boot logo. It
will save a backup of your existing
"boot_logo.bmp.gz" in a folder named
"bak" at the root of the ROMS partition
of the SD card that goes into the
TF1 slot.

## Update

Follow step 2 of the install instructions.

## Releases

You can [grab the latest version here](https://github.com/shauninman/union-minui/releases).
