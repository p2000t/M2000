# M2000 - Philips P2000 home computer emulator
Version 0.7-SNAPSHOT

![P2000T](/img/P2000T.png)
                                     
## Supported platforms

* Windows
* Linux
* macOS
* MS-DOS

## Downloads

Coming soon on the [M2000 releases](https://github.com/p2000t/M2000/releases) page.\
Until then, get the M2000 emulator and games from here: https://github.com/p2000t/software/



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
F6               -  Change options (MS-DOS version only)
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
                       When a cassette (.cas) is given, by default BASIC will try to boot it
-trap <address>        Trap execution when PC reaches specified address [-1]
                       (Debugging version only)
-help                  Print a help page describing all available command
                       line options
-verbose <level>       Select debugging messages [1]
                       0 - Silent           1 - Startup messages
                       4 - Tape messages
-ifreq <frequency>     Select interrupt frequency [50] Hz
-cpuspeed <speed>      Set Z80 CPU speed [100]%
-sync <mode>           Sync/Do not sync emulation [1]
                       0 - Do not sync   1 - Sync
                       Emulation is faster if sync is turned off
-ram <value>           Select amount of RAM installed [32KB]
-uperiod <period>      Number of interrupts per screen update [1]
                       Try -uperiod 2 or -uperiod 3 if emulation is a bit slow
-t / -m                Select P2000 model [-t]
-keymap <value>        Select keyboard mapping [1]
                       0 - Positional mapping
                       1 - Symbolic mapping (not supported in MS-DOS)
-video <mode>          Select video mode/window size [0]
                       0  - Autodetect best window size
                            256x240 (only for MS-DOS)
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
                       1 - Show scanlines (not supported in MS-DOS)
-printer <filename>    Select file for printer output [Printer.out]
-printertype <value>   Select printer type [0]
                       0 - Daisy wheel   1 - Matrix
-romfile <file>        Select P2000 ROM dump file [P2000ROM.bin]
-tape <filename>       Select tape image to use [Default.cas]
-boot <value>          Allow/Don't allow BASIC to boot from tape
                       0 - Don't allow booting (default when no [filename] is given)
                       1 - Allow booting (default when a .cas [filename] is given)
-font <filename>       Select font to use [Default.fnt]
-sound <mode>          Select sound mode [255]
                       0 - No sound
                       1 - PC Speaker (MS-DOS)
                       2 - SoundBlaster (MS-DOS)
                       255 - Detect
-volume <value>        Select initial volume
                       0 - Silent   15 - Maximum
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
- **Positional** key mapping, in which the keys are mapped to the same relative positions as they would be on a real P2000 keyboard. So that means that typing Shift-2 on your keyboard will show the double-quote (") character, because that matches a real P2000 when you type Shift-2. \
***Positional key mapping is the default and only avialable option for the MS-DOS version of M2000.***

- **Symbolic** key mapping, in which typing a key on your keyboard will - as far as possible - show the actual character/symbol written on the keycap. So that means that typing Shift-2 will show the @ symbol in the emulator. \
***Symbolic key mapping is the default option for M2000, but can be changed to positional key mapping by passing `-keymap 0` as command line argument.***

### Positional key mapping overview

```
Delete        -  <
Shift-Delete  -  >
` ~           -  CODE

For the other key mappings, see picture below.
```

![keyboard mappings](/img/toetsenbord.png)

## How to compile M2000 from the sources

If you want to compile for an alternative OS or help us with fixing bugs, you'll first need to open a terminal (or command prompt) and clone this M2000 repo (or your fork!) into a local folder: \
`git clone https://github.com/p2000t/M2000.git`

### Linux
* Install the Allegro 5 libs, depending on your Linux distro
  * For Debian/Ubuntu/Linux Mint:
    ```
    sudo apt install -y build-essential liballegro5-dev
    ```
  * For Fedora:
    ```
    sudo dnf install allegro5*
    ```
* Go into the M2000 directory and run make
  ```
  cd M2000
  make
  ```
  The resulting `M2000` executable will be available in the M2000 folder.

### macOS
* Make sure you have `brew` installed before you install the Allegro 5 libs:
  ```
  brew install allegro
  ```
* Go into the M2000 directory and run make
  ```
  cd M2000
  make
  ```
  The resulting `M2000` executable will be available in the M2000 folder. \
  Tested for macOS 10.13 (High Sierra).

### Windows
* Make sure to have MinGW (the Windows port of gcc) installed on your machine. \
A good distribution is [TDM-GCC](https://jmeubank.github.io/tdm-gcc/download/). Select either the 32 or 64 bits version, which by default will install MinGW in either `C:\TDM-GCC-32` or `C:\TDM-GCC-64` and then automatically adds the `bin` folder to your PATH environment variable. \
You can test a correct installation by opening a command prompt and typing `gcc --version`
* Download the static [Allegro 5 libraries v5.2.8.0](https://github.com/liballeg/allegro5/releases/tag/5.2.8.0) and pick the version that matches your MinGW architecture. So `i686-w64` (dwarf-static) for 32-bits or `x86-w64` (seh-static) for 64-bits. Copy the content of the downloaded zip (i.e., folders `bin`, `include` and `lib`) into the root of your MinGW folder.
* Open a command prompt into the (cloned) M2000 folder and type: `mingw32-make`. The resulting `M2000.exe` will be copied into the M2000 folder, where you can now run it.

### Windows (WSL Ubuntu cross-compilation)
Alternatively, you can build the Windows version on WSL (Windows Subsystem for Linux).
* Ensure you have [Windows Subsystem for Linux](https://learn.microsoft.com/en-us/windows/wsl/install) installed 
  and have downloaded the latest Ubuntu image from the Windows store.
* Install the required following packages in Ubuntu
  ```bash
  sudo apt install mingw-w64 build-essential cmake zip curl
  ```
* Clone this repository, create a `build` folder in its root directory, go to the `build` folder and compile
  a Makefile using `cmake`
  ```bash
  mkdir build && cd build && cmake ../src
  ```
* Compile the program by running
  ```bash
  make -j
  ```
* The executable is placed in the `build` folder, including its dependencies. You can either directly use this executable, or use
  the `.zip` file found in the same folder and deploy the emulator in another folder.

### MS-DOS
* Download and install [DOSBox](https://www.dosbox.com/)
* Go to the folder which contains your cloned M2000 repo and open the subfolder `djgpp`
* Extract all files from the 5 zips (`djdev202.zip`, `bnu281b.zip`, `gcc281b.zip`, `mak377b.zip` and `csdpmi4b.zip`) directly into `djgpp`, so this folder will get subfolders `bin`, `gnu`, `include`, etc.
* Open the DOSBox options file and copy/paste these lines at the bottom under [autoexec]: \
  ***note: replace "C:\path-to-your-clone-of-M2000" with the actual path***
  ```
  mount c "C:\path-to-your-clone-of-M2000"
  c:
  set PATH=C:\DJGPP\BIN;%PATH%
  set DJGPP=C:\DJGPP\DJGPP.ENV
  #optional: reduce sound volume to 20%
  MIXER MASTER 20
  MIXER SPKR 20
  MIXER SB 20
  MIXER FM 20
  ```
  Note: if you have a joystick attached, pleas make sure to set `timed=false` in the DOSBox options.
* Now run DOSBox. When you type `dir` in the command prompt, it should show you the content of your cloned M2000 repo
* Go into the src folder (`cd src`) and type: `make clean` and then `make dos`. Wait for the compiler to finish...
* Go back to the parent folder (`cd ..`) and notice `m2000.exe` is there. You can now run `m2000.exe` and test it. 

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
