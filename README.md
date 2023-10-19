# M2000: Emulator for the Philips P2000 home computer
Version 0.6.1-SNAPSHOT
                                     
## Supported platforms

* MS-DOS
* Unix/X
* Windows (experimental, using the Allegro 5 libraries)

## Download

Coming soon on the [M2000 releases](https://github.com/p2000t/M2000/releases) page.\
Until then, get the M2000 emulator and games from here: https://github.com/p2000t/software/

### Files included in the release packages
```
m2000          The emulator
README.md      This readme file
Default.fnt    SAA5050 font
P2000ROM.bin   P2000 ROM image
BASIC.bin      BASIC cartridge ROM image (v. 1.1NL)
CWSDPMI.EXE    (MS-DOS version only) A DPMI server required by M2000
allegro*.dll   (Windows version only) Allegro libraries required by M2000
```

## What's emulated

-  P2000T or P2000M model (P2000M emulation is buggy)
-  Support for 1 ROM cartridge
-  User-definable amount of RAM
-  One tape drive
-  Sound
-  SAA5050 character rounding emulated in high resolution mode

## Key Mappings

![keyboard mappings](/img/toetsenbord.png)

```
Cursor Keys, Alt/Ctrl -  Movement
Delete                -  <
Shift-Delete          -  >
` ~                   -  CODE
```

### Special Keys
```
F3           -  Dump video RAM to file (P2000T mode only)
F4           -  Toggle tracing on/off (Debugging version only)
F5           -  Toggle sound on/off
F11          -  Decrease sound volume
F12          -  Increase sound volume
F6           -  Change options
F7           -  Make screen shot (Not implemented in the Unix/X version)
F8           -  Pause & Blank screen
F9           -  Pause
ESC/F10      -  Quit emulator
```

## Command line options
```
-trap <address>        Trap execution when PC reaches specified address [-1]
                       (Debugging version only)
-help                  Print a help page describing all available command
                       line options
-verbose <level>       Select debugging messages [1]
                       0 - Silent           1 - Startup messages
                       4 - Tape
-ifreq <frequency>     Select interrupt frequency [50 Hz]
-cpuspeed <speed>      Set Z80 CPU speed [100%]
-sync <value>          Sync/Do not sync emulation [1]
                       0 - Do not sync   1 - Sync
                       Emulation is faster if sync is turned off
-ram <value>           Select amount of RAM installed [32KB]
-uperiod <period>      Number of interrupts per screen update [1]
                       Try -uperiod 2 or -uperiod 3 if emulation is a bit
                       slow
-t / -m                Select P2000 model [-t]
-video <mode>          Select video mode/window size [0]
                       0 - 500x300 (Unix/X)
                           320x240 (Linux/SVGALib, T-model emulation only)
                           256x240 (MS-DOS, T-model emulation only)
                       1 - 520x490 (Unix/X)
                           640x480 (Linux/SVGALib and MS-DOS)
-printer <filename>    Select file for printer output
                       Default is PRN for the MS-DOS version, stdout for
                       the Unix versions
-printertype <value>   Select printer type [0]
                       0 - Daisy wheel   1 - Matrix
-romfile <file>        Select P2000 ROM dump file [P2000ROM.bin]
-tape <filename>       Select tape image to use [P2000.cas]
-boot <value>          Allow/Don't allow BASIC to boot from tape [0]
                       0 - Don't allow booting
                       1 - Allow booting
-font <filename>       Select font to use [Default.fnt]
-sound <mode>          Select sound mode [255]
                       0 - No sound
                       1 - PC Speaker (MS-DOS) or /dev/dsp (Unix)
                       2 - SoundBlaster (MS-DOS)
                       255 - Detect
-volume <value>        Select initial volume
                       0 - Silent   15 - Maximum
-joystick <mode>       Select joystick mode [1]
                       0 - No joystick support
                       1 - Joystick support
                       When joystick support is on, moving the joystick
                       emulates cursorkey presses, pressing a joystick
                       button emulates pressing the spacebar
-shm <mode>            Use/Do not use MIT SHM extensions for X [1] (Unix/X
                       version only)
                       0 - Don't use SHM   1 - Use SHM
```

## Configuration files

The emulator loads two configuration files (if present) before it loads a cartridge ROM: 
* M2000.cfg located in the emulator's directory and
* CART.cfg (i.e. BASIC.cfg by default) located in the cartridge dump's directory.
  
These are plain text files containing optional command line options. \
Options can be separated with spaces, tabs or returns. \
Please note that for the Unix versions, the configuration files should be present in the current working directory.

## How to compile M2000 from the sources

If you want to compile for an alternative OS or help us with fixing bugs, you'll first need to open a terminal (or command prompt) and clone this M2000 repo (or your fork!) into a local folder: \
`git clone https://github.com/p2000t/M2000.git`

### MS-DOS:
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
* Now run DOSBox. When you type `dir` in the command prompt, it should show you the content of your cloned M2000 repo
* Go into the src folder (`cd src`) and type: `make clean` and then `make dos`. Wait for the compiler to finish...
* Go back to the parent folder (`cd ..`) and notice `m2000.exe` is there. You can now run `m2000.exe` and test it. 

### Unix/X:
* Make sure you have installed: make, gcc and the X11 libs:
  ```
  sudo apt-get update
  sudo apt install make
  sudo apt install gcc
  sudo apt install libx11-dev
  sudo apt-get install libxext-dev
  ``` 
* Now go into the src folder of the cloned M2000 repo and type: `make x`. \
The resulting `m2000` will be copied into the root of your cloned M2000 repo, where you can now run it.
* If you're missing sound on Ubuntu, please try installing also-oss (`sudo apt-get install alsa-oss`) and run m2000 through the alsa-oss wrapper: \
  `aoss ./m2000`

### Windows:
* Make sure to have WinGW (the Windows port of gcc) installed on your machine. \
A good distribution is [TDM-GCC](https://jmeubank.github.io/tdm-gcc/download/). Select either the 32 or 64 bits version, which by default will install WinGW in either `C:\TDM-GCC-32` or `C:\TDM-GCC-64` and then automatically adds the `bin` folder to your PATH environment variable. \
You can test a correct installation by opening a command prompt and typing `gcc --version`
* Download the static [Allegro 5 libraries v5.2.8.0]([https://github.com/liballeg/allegro5/releases](https://github.com/liballeg/allegro5/releases/tag/5.2.8.0)) and pick the version that matches your WinGW architecture. So `i686-w64` (dwarf-static) for 32-bits or `x86-w64` (seh-static) for 64-bits. Copy the content of the downloaded zip (i.e., folders `bin`, `include` and `lib`) into the root of your WinGW folder.
* Open a command prompt into the src folder of your cloned M2000 repo and type: `mingw32-make allegro`. The resulting `m2000.exe` will be copied into the root of your cloned M2000 repo, where you can now run it. \
Note: when distributing m2000.exe, don't forget to include these Allegro 5 libraries: \
(*replace * with the version of the Allegro 5 libraries you installed*)
  * allegro-5.*.dll
  * allegro_primitives-5.*.dll
  * allegro_image-5.*.dll
  * allegro_audio-5.*.dll

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

* You are done.

## More information on the P2000

### P2000 documentation
* A large collection of (scanned) P2000 documents, P2000gg and Nat.Lab. newsletters and editions of TRON magazine can be found on: https://github.com/p2000t/documentation
* The [P2000T community on Retroforum](https://www.retroforum.nl/topic/3914-philips-p2000t/
) can help out with questions. They're nice people :-)

### P2000 software
* Many P2000 cartridge images and cassette dumps (games, utilities, etc.) can be found on: https://github.com/p2000t/software

## Credits

- Marcel de Kogel for creating the M2000 emulator, as found on his [M2000 distribution site](https://www.komkon.org/~dekogel/m2000.html)
- Stafano Bodrato (@zx70) for creating the Allegro version for M2000
- Hans Bus provided lots of technical information on the P2000
- Marat Fayzullin provided invaluable help improving the Unix/X version

## License

Unknown, but probably GNU GPLv3, as most of the original source files contain the following header:
```
Copyright (C) Marcel de Kogel 1996,1997
You are not allowed to distribute this software commercially
Please, notify me, if you make any changes to this file
```
