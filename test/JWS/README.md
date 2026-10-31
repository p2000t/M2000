# JWS floppy boot fixtures

The cartridge and system floppy are the pair used to validate the emulator's
FDC implementation. An empty JWS disk is also included for testing file writes
on a working copy. Both floppies are raw 320 KiB JWS images: 40 cylinders,
two heads, and 16 sectors of 256 bytes per track.

## Source

Copied without modification from the [P2000T software preservation archive](https://github.com/p2000t/software),
commit `53654def997bee2a8784e18b433a3b4ccc2f39e3`. The upstream spelling `jws-sytem.dsk` is retained.
These are third-party software assets; the upstream archive supplies no license
information for these files.

| File (upstream link) | Bytes | SHA-256 |
| --- | ---: | --- |
| [JWSBasic.bin](https://github.com/p2000t/software/blob/53654def997bee2a8784e18b433a3b4ccc2f39e3/cartridges/JWSBasic.bin) | 16384 | `a2dd996e03cb365170b96caa0a4c19404c28f52bb58ced86324961ba1ed0d409` |
| [jws-sytem.dsk](https://github.com/p2000t/software/blob/53654def997bee2a8784e18b433a3b4ccc2f39e3/disks/jws-sytem.dsk) | 327680 | `3fa390c36f01dc85bc718085771155de59795f44c5ac18a5cf1414620d925e67` |
| [empty-jws.dsk](https://github.com/p2000t/software/blob/53654def997bee2a8784e18b433a3b4ccc2f39e3/disks/empty-jws.dsk) | 327680 | `1babaaaf1de4cd03c0ba7b3c0292ef47ccbb50006f255588e7cf0080ae0c3448` |

## Boot test

From the repository root, use a temporary disk copy because the emulator writes
to mounted images in place:

```sh
jws_test_dir=$(mktemp -d)
cp test/JWS/jws-sytem.dsk "$jws_test_dir/jws-sytem.dsk"
./M2000 ./test/JWS/JWSBasic.bin --floppy "$jws_test_dir/jws-sytem.dsk"
```

For a menu-driven test, enable **Hardware → Floppy Controller**, select
`JWSBasic.bin` using **File → Insert Cartridge**, and select a working copy of
`jws-sytem.dsk` using **File → Insert Floppy Image**.

Expected behavior:

- The monitor reads the two system tracks into bank 1 at `0xE000–0xFFFF`.
  Before JWS starts modifying RAM, these 8192 bytes match disk offsets
  `0x0000–0x0FFF` followed by `0x2000–0x2FFF` (head 0 of cylinders 1 and 2).
- The screen displays **JWS DISK SYSTEM**, **versie 5.0NL**.
- The disk automatically loads **Utilities II** and displays its disk-number prompt.
- Cold and warm resets can boot the same image again.

To test writes on an empty disk, copy `empty-jws.dsk` to a working directory
and select that copy using **File → Insert Floppy Image**.

Keep the checked-in images unchanged so future tests use the same input bytes.
