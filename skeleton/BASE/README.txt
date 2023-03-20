MinUI is a minimal launcher

Source: https://github.com/shauninman/union-minui

----------------------------------------
Features

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

----------------------------------------
Following Instructions 101

  Please read every step of the 
  relevant instructions before 
  undertaking the first step. A 
  little context goes a long way 
  towards understanding.

----------------------------------------
Install

MinUI is meant to be installed over a working stock TF1. The official Anbernic image requires a 64GB SD card: 

  https://drive.google.com/drive/folders/1HfGCxkratM_zYiYfGWCrHZ1AynO3hIwU 
  
Or you can download a reduced image that can be flashed to a 4GB SD card:

  https://github.com/shauninman/union-minui/releases/tag/stock-tf1-20230309 

If you are using two SD cards (recommended), the second one should be formatted FAT32.

Copy "dmenu.bin" to the root of the MISC partition of the SD card that goes in the TF1 slot. 

Where you copy "MinUI.zip" (and the Bios, Roms, and Saves folders) will depend on if you are using one or two SD cards (again, using two SD cards is recommended). If using one SD card, copy them to the root of the ROMS partition of the SD card that goes in the TF1 slot. Otherwise copy them to the root of the SD card that goes in the TF2 slot. 

During installation (but not on subsequent updates) MinUI will install a custom boot logo. It will save a backup of your existing "boot_logo.bmp.gz" in a folder named "bak" at the root of the ROMS partition of the SD card that goes into the TF1 slot. 

----------------------------------------
Update

If using one SD card, copy "MinUI.zip" to the root of the ROMS partition of the SD card that goes in the TF1 slot. Otherwise copy "MinUI.zip" to the root of the SD card that goes in the TF2 slot. 

----------------------------------------
Shortcuts

Reduce/increase brightness:

MENU + VOLUME UP or VOLUME DOWN

----------------------------------------
Roms

Included in this zip is a Roms folder containing folders for each console MinUI currently supports. You can rename these folders but you must keep the uppercase tag name in parentheses in order to retain the mapping to the correct emulator (eg. "Nintendo Entertainment System (FC)" could be renamed to "Nintendo (FC)", "NES (FC)", or "Famicom (FC)"). 

You can (and probably should) preload these folders with roms and copy each one to the Roms folder on your SD card before installing.

When one or more folder share the same display name (eg. "Game Boy Advance (GBA)" and "Game Boy Advance (MGBA)") they will be combined into a single menu item containing the roms from both folders (continuing the previous example, "Game Boy Advance"). This allows opening specific roms with an alternate pak.

----------------------------------------
Bios

Some emulators require or perform much better with official bios. MinUI is strictly BYOB. Place the bios for each system in a folder that matches the tag in the corresponding Roms folder name (eg. bios for "Sony PlayStation (PS)" roms goes in "/Bios/PS/"),

Bios file names are case-sensitive:

   FC: disksys.rom
   GB: gb_bios.bin
  GBA: gba_bios.bin
  GBC: gbc_bios.bin
   PS: psxonpsp660.bin
	
----------------------------------------
Disc-based games

To streamline launching multi-file disc-based games with MinUI place your bin/cue (and/or iso/wav files) in a folder with the same name as the cue file. MinUI will automatically launch the cue file instead of navigating into the folder when selected, eg. 

  Harmful Park (English v1.0)/
    Harmful Park (English v1.0).bin
    Harmful Park (English v1.0).cue

For multi-disc games, put all the files for all the discs in a single folder and create an m3u file (just a text file containing the relative path to each disc's cue file on a separate line) with the same name as the folder. Instead of showing the entire messy contents of the folder, MinUI will launch the appropriate cue file, eg. For a Policenauts folder structured like this:

  Policenauts (English v1.0)/
    Policenauts (English v1.0).m3u
    Policenauts (Japan) (Disc 1).bin
    Policenauts (Japan) (Disc 1).cue
    Policenauts (Japan) (Disc 2).bin
    Policenauts (Japan) (Disc 2).cue

The m3u file would contain just:

  Policenauts (Japan) (Disc 1).cue
  Policenauts (Japan) (Disc 2).cue

MinUI also supports chd files and official pbp files (multi-disc pbp files larger than 2GB are not supported). Regardless of the multi-disc file format used, every disc of the same game share the same memory card and save state slots.

----------------------------------------
Collections

A collection is just a text file containing an ordered list of full paths to rom, cue, or m3u files. These text files live in the Collections folder at the root of your SD card, eg. "/Collections/Metroid series.txt" might look like this:

  /Roms/GBA/Metroid Zero Mission.gba
  /Roms/GB/Metroid II.gb
  /Roms/SNES (SFC)/Super Metroid.sfc
  /Roms/GBA/Metroid Fusion.gba

----------------------------------------
Advanced

MinUI can automatically run a user-authored shell script on boot. Just place a file named "auto.sh" in "/.userdata/".

----------------------------------------
Thanks

To BlackSeraph, for introducing me to chroot and for sharing his modified uImage which provides overclocking (and underclocking) and increases the available framebuffer memory.

To eggs, for his NEON scalers, years of top-notch example code, and patience in the face of my endless questions.

Check out eggs' RG35XX releases: https://www.dropbox.com/sh/3av70t99ffdgzk1/AAAKPQ4y0kBtTsO3e_Xlrhqha

To neonloop, for putting together the Trimui toolchain from which I learned everything I know about docker and buildroot and is the basis for every toolchain I've put together since, and for picoarch, the inspiration for minarch.

Check out neonloop's repos: https://git.crowdedwood.com

And to Jim Gray, for commiserating during development, for early alpha testing, and for providing the soundtrack for much of MinUI's development.

Check out Jim's music: https://ourghosts.bandcamp.com/music
