# M2000 - Philips P2000 home computer emulator
Version 0.8-SNAPSHOT

![P2000T](/img/P2000T.png)
                                     
## Supported platforms

* Windows
* Linux
* macOS

## Downloads

For downloading the latest release, please see the [M2000 releases](https://github.com/p2000t/M2000/releases) page.\
Get additional cassette- and cartridge dumps from here: https://github.com/p2000t/software/

### Installation

Installation of M2000 depends on your platform:
* **Windows** (64 bit) \
  Unzip the downloaded release package and double click the `M2000-installer.exe`, which guides you through installation. After installation, "M2000 - Philips P2000 Emulator" will be added to your Windows apps.
* **macOS** (version 10.13 or higher) \
  Unzip the downloaded release package (usually this is done by just double-clicking it) and drag the resulting "M2000" app into the Applications folder. Now you can start M2000 from your applications - probably after allowing M2000 to run in the security settings.
* **Linux** (Debian/Ubuntu/Linux Mint) \
  Unzip the downloaded release package and double-click `M2000_amd64.deb` to start the package installer(\*). After installation is done, type `M2000` in a terminal to start the emulator. \
  (\*) If double-clicking doesn't open a package installer, then open a terminal to the unzipped .deb file and do:
  ```
  sudo apt -f install M2000_amd64.deb
  ```

## What's emulated

-  P2000T or P2000M model (P2000M emulation is experimental)
-  Support for a single ROM cartridge
-  User-definable amount of RAM
-  One tape drive
-  Sound
-  SAA5050 character rounding

## Control Keys
```
Ctrl-I           -  Insert cassette dialog
Ctrl-O           -  Open/Boot cassette dialog
Ctrl-E           -  Eject current cassette
Ctrl-R           -  Reset P2000

Ctrl-Enter       -  Toggle fullscreen on/off (not supported on Linux)
Ctrl-L           -  Toggle scanlines on/off

Ctrl-C           -  Save current state to file (without dialog)
Ctrl-V           -  Load previously saved state (without dialog)
Ctrl-S           -  Save screenshot picture to file (without dialog)
Ctrl-R           -  Save visible video RAM to file (without dialog)

Ctrl-1           -  ZOEK key (show cassette index)
Ctrl-3           -  START key (start loaded program)
Ctrl-.           -  STOP key (pause/halt program)
Ctrl-7           -  WIS key (clear cassette dialog)

Ctrl-P           -  Toggle pause on/off
Ctrl-M           -  Mute/unmute sound

Ctrl-Q           -  Quit emulator
```

## Command line options
```
M2000 [filename]       Optional cassette (.cas) or cartridge (.bin) to preload
                       When a cassette (.cas) is provided, BASIC will try to boot it
```

### Configuration file

After starting M2000 for the first time, a configuration file named `M2000.cfg` will be created in the root of the M2000 folder inside the user's Documents folder. This is a plain text file which can be edited by the user.

## Keyboard emulation

There are two keyboard mappings available in M2000:

- **Symbolic** key mapping (default), in which typing a key on your keyboard will - as far as possible - show the actual character/symbol written on the keycap. So that means that typing Shift-2 will show the @ symbol in the emulator.

- **Positional** key mapping (optional), in which the keys are mapped to the same relative positions as they would be on a real P2000 keyboard. So that means that typing Shift-2 on your keyboard will show the double-quote (") character, because that matches a real P2000 when you type Shift-2.

## How to compile M2000 from the sources

If you want to compile the M2000 sources yourself, then the instructions below will get you on your way. Note that M2000 is depending on the Allegro 5 library, which might not be available as pre-compiled package on every platform.

### Linux
* Using your Linux disto's package manager, install the essential build tools and Allegro 5 libs. For Debian/Ubuntu/Linux Mint you can use the `apt` package manager:
  ```
  sudo apt update && sudo apt install git build-essential liballegro5-dev
  ```
* Clone the M2000 repo:
  ```
  git clone https://github.com/p2000t/M2000.git
  ```
* Go into the M2000 directory and run make
  ```
  cd M2000 && make
  ```
* After successfull building, you can run M2000:
  ```
  ./M2000
  ```

### macOS
Make sure you have both the `Xcode command line tools` and `brew` installed.
* Now install the Allegro 5 libs using brew:
  ```
  brew install allegro
  ```
* Clone the M2000 repo:
  ```
  git clone https://github.com/p2000t/M2000.git
  ```
* Go into the M2000 directory and run make
  ```
  cd M2000 && make
  ```
* After successfull building, you can run M2000:
  ```
  ./M2000
  ```

### Windows
The easiest way to build M2000 on a Windows machine is by using the [MSYS2](https://www.msys2.org/) toolkit.


* After you've installed MSYS2, open its MINGW64 shell environment and install the required tools and libraries:
  ```
  pacman -S base-devel git mingw-w64-x86_64-gcc mingw-w64-x86_64-allegro
  ```
* Clone the M2000 repo:
  ```
  git clone https://github.com/p2000t/M2000.git
  ```
* Go into the M2000 directory and run make:
  ```
  cd M2000 && make
  ```
* After successfull building, you can run M2000:
  ```
  ./M2000
  ```

## More information on the P2000

### P2000 documentation
* A large collection of (scanned) P2000 documents, P2000gg and Nat.Lab. newsletters and editions of TRON magazine can be found on: https://github.com/p2000t/documentation
* The [P2000T community on Retroforum](https://www.retroforum.nl/topic/3914-philips-p2000t/
) can help out with questions. They're nice people :-)

### P2000 software
* Many P2000 cartridge images and cassette dumps (games, utilities, etc.) can be found on: https://github.com/p2000t/software

## Credits

* Thanks to Marcel de Kogel for originally creating M2000 back in 1996-1997. His [M2000 distribution site](https://www.komkon.org/~dekogel/m2000.html), which contains the original source code and MS-DOS binaries, is still up and running.

* Thanks to Stafano Bodrato for creating the initial Allegro driver for M2000 in 2013.

* Thanks to the M2000 maintainers and contributors for its continued development: Ivo Filot, Martijn Koch and Dion Olsthoorn.

## License

M2000 is distributed under the terms of the GNU General Public License v3 as described in [LICENSE](LICENSE).

