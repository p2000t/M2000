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
  Unzip the downloaded release package and double-click `M2000_0.7_amd64.deb` to start the package installer. After installation is done, type `M2000` in a terminal to start the emulator. \
  If double-clicking doesn't open a package installer, then open a terminal to the unzipped .deb file and do:
  ```
  sudo apt-get -f install M2000_0.7_amd64.deb
  ```

## What's emulated

-  P2000T or P2000M model (P2000M emulation is experimental)
-  Support for 1 ROM cartridge
-  User-definable amount of RAM
-  One tape drive
-  Sound
-  SAA5050 character rounding emulated in high resolution mode

## Function Keys
```
F1               -  ZOEK key (show cassette index in BASIC)
F2               -  START key (start loaded program in BASIC)
Shift-F2         -  STOP key (pause/halt program in BASIC)
F5               -  Reset P2000
F7               -  Save screenshot to file (quietly)
F8               -  Save visible video RAM to file (quietly)
F9               -  Toggle pause on/off
F10              -  Toggle sound on/off
F11              -  Toggle fullscreen on/off (use Shift-F11 for macOS)
Alt-F4 / Ctrl-Q  -  Quit emulator
```

## Command line options
```
[filename]             Optional cassette (.cas) or cartridge (.bin) to preload
                       When a cassette (.cas) is provided, BASIC will try to boot it
-trap <address>        Trap execution when PC reaches specified address [-1]
                       (Debugging version only)
-verbose <level>       Select debugging messages [1]
                       0 - Silent           
                       1 - Startup messages
                       4 - Tape messages
-ifreq <frequency>     Select interrupt frequency [50] Hz
-cpuspeed <speed>      Set Z80 CPU speed [100]%
-sync <mode>           Sync/Do not sync emulation [1]
                       0 - Do not sync   
                       1 - Sync
                       Emulation will be too fast if sync is turned off
-ram <value>           Select amount of RAM installed [32KB]
-uperiod <period>      Number of interrupts per screen update [1]
                       Try -uperiod 2 or -uperiod 3 if emulation is a bit slow
-t / -m                Select P2000 model [-t]
-keymap <mode>         Select keyboard mapping [1]
                       0 - Positional mapping
                       1 - Symbolic mapping
-video <mode>          Select video mode/window size [0]
                       0  - Autodetect best window size
                       1  - 640x480
                       2  - 800x600
                       2  - 960x720
                       3  - 1280x960
                       4  - 1440x1080
                       5  - 1600x1200
                       6  - 1920x1440
                       99 - Full Screen (not supported on Linux)
-scanlines <mode>      Show/Do not show scanlines [0]
                       0 - Do not show scanlines
                       1 - Show scanlines
-smoothing <mode>      Use display smoothing [1]
                       0 - Do not use smoothing  
                       1 - Use smoothing
-printer <filename>    Select file for printer output [Printer.out]
-printertype <mode>    Select printer type [0]
                       0 - Daisy wheel   
                       1 - Matrix
-romfile <file>        Select P2000 ROM dump file [P2000ROM.bin]
-tape <filename>       Select tape image to use [Default.cas]
-boot <mode>           Allow/Don't allow BASIC to boot from tape
                       0 - Don't allow booting (default when no [filename] is given)
                       1 - Allow booting (default when a .cas [filename] is given)
-font <filename>       Select font to use [Default.fnt]
-sound <mode>          Select sound mode [1]
                       0 - No sound  
                       1 - Sound on
-volume <value>        Select initial volume
                       0 - Silent
                       ...
                       15 - Maximum
-joystick <mode>       Select joystick mode [1]
                       0 - No joystick support
                       1 - Joystick support
-joymap <mode>         Select joystick mapping [0]
                       0 - Moving the joystick emulates cursorkey presses
                           The main button emulates pressing the spacebar
                       1 - Fraxxon mode: Left/right emulates cursorkeys left/up
                           The main button emulates pressing the spacebar
```

### Configuration file (optional)

If present in the emulator's directory, the `M2000.cfg` configuration file will be loaded and parsed. \
This is a plain text file containing optional command line options. Options can be separated with spaces, tabs or returns.

## Keyboard emulation

There are two keyboard mappings available in M2000:

- **Symbolic** key mapping (default), in which typing a key on your keyboard will - as far as possible - show the actual character/symbol written on the keycap. So that means that typing Shift-2 will show the @ symbol in the emulator.

- **Positional** key mapping (optional), in which the keys are mapped to the same relative positions as they would be on a real P2000 keyboard. So that means that typing Shift-2 on your keyboard will show the double-quote (") character, because that matches a real P2000 when you type Shift-2.

## How to compile M2000 from the sources

If you want to compile the M2000 sources yourself, then the instructions below will get you on your way. Note that M2000 is depending on the Allegro 5 library, which might not be available as pre-compiled package on every platform.

### Linux
* Using your Linux disto's package manager, install the essential build tools and Allegro 5 libs. For Debian/Ubuntu/Linux Mint you can use the `apt` package manager:
  ```
  $ sudo apt update && sudo apt install git build-essential liballegro5-dev
  ```
* Clone the M2000 repo:
  ```
  $ git clone https://github.com/p2000t/M2000.git
  ```
* Go into the M2000 directory and run make
  ```
  $ cd M2000 && make
  ```
* After successfull building, you can run M2000:
  ```
  $ ./M2000
  ```

### macOS
Make sure you have both the `Xcode command line tools` and `brew` installed.
* Now install the Allegro 5 libs using brew:
  ```
  $ brew install allegro
  ```
* Clone the M2000 repo:
  ```
  $ git clone https://github.com/p2000t/M2000.git
  ```
* Go into the M2000 directory and run make
  ```
  $ cd M2000 && make
  ```
* After successfull building, you can run M2000:
  ```
  $ ./M2000
  ```

### Windows
The easiest way to build M2000 on a Windows machine is by using the [MSYS2](https://www.msys2.org/) toolkit.


* After you've installed MSYS2, open its MINGW64 shell environment and install the required tools and libraries:
  ```
  $ pacman -S base-devel git mingw-w64-x86_64-gcc mingw-w64-x86_64-allegro
  ```
* Clone the M2000 repo:
  ```
  $ git clone https://github.com/p2000t/M2000.git
  ```
* Go into the M2000 directory and run make:
  ```
  $ cd M2000 && make
  ```
* After successfull building, you can run M2000:
  ```
  $ ./M2000
  ```

## More information on the P2000

### P2000 documentation
* A large collection of (scanned) P2000 documents, P2000gg and Nat.Lab. newsletters and editions of TRON magazine can be found on: https://github.com/p2000t/documentation
* The [P2000T community on Retroforum](https://www.retroforum.nl/topic/3914-philips-p2000t/
) can help out with questions. They're nice people :-)

### P2000 software
* Many P2000 cartridge images and cassette dumps (games, utilities, etc.) can be found on: https://github.com/p2000t/software

## Credits

- Marcel de Kogel for creating the M2000 emulator, as found on his [M2000 distribution site](https://www.komkon.org/~dekogel/m2000.html)
- Stafano Bodrato (@zx70) for creating the initial Allegro version for M2000

## License

Unknown, but probably GNU GPLv3, as most of the original source files contain the following header:
```
Copyright (C) Marcel de Kogel 1996,1997
You are not allowed to distribute this software commercially
Please, notify me, if you make any changes to this file
```
