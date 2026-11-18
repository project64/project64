# Project64 GameShark / Action Replay Cartridge Support

- Author: Nekokabu <nekokabu@gmail.com>
- Date: 2026-11-10 (last updated 2026-11-11)
- Target: Project64 (develop branch) / N64 GameShark Pro v3.30 firmware
  (internal name `(C) MUSHROOM &NU`, build `09:54 Mar 27`, 256KB)

## 1. Overview

This change adds emulation of the N64 GameShark Pro / Pro Action Replay pass-through
cartridge to Project64. When a firmware ROM is opened, Project64 asks for the game ROM
that is plugged in to the top of the cartridge. The GameShark menu then boots the same
way as on a console: the IPL3 of the game checks the key code of the GameShark and
starts its firmware. "Start Game" in the menu starts the game with the code engine and
trainer resident, and the GS button opens the in-game menu. Writes the firmware makes
to its flash are not written back to the firmware file; they are saved as an IPS patch.

## 2. Sources

The hardware model is based only on the following sources.

1. Static analysis of the firmware `gsv33.n64`, disassembled for this work.
   `shell.bin` and `trainer.bin` were analysed after LZARI decompression. Addresses in
   the "Evidence" column below are load addresses after decompression
   (boot loader 0x80040000, shell 0x80200400, trainer 0x80780800).
2. Measurements on a real cartridge, recorded in `gameshark-n64-analysis.md`
   (Nekokabu, 2026-11-03): reads past 0x20000 return zero during boot, the structure of
   the key code, booting with a 5101 key, and the WatchLo behaviour in Baku Bomberman 2.
3. Console measurements of reads from unpopulated RDRAM with the test ROM
   rdram_unmapped v1.1, recorded in `rdram-hw-report.md` (Nekokabu, 2026-11-13).
   Used only for the change in section 4.4.

RWeick's REF1329 and documents derived from it were not used.

One piece of outside knowledge was used: the checksum the game's IPL3 calculates
(6102/6103/6105/6106). This belongs to the game cartridge, not to the GameShark, and
the commonly known algorithm was implemented. The results for 6102/6103/6106 were
confirmed to match the CRCs in the key table of the firmware (0x2FC00). For 6105
there is no test against the key table without the game's IPL3; a 6105 game was
confirmed to work on the Windows build. The calculation is only used for the
warning shown before boot; whether the game actually boots is decided by the game's
IPL3 running in the emulator.

## 3. Hardware Analysis

### 3.1 Address map

X is the value of the address latch (0x10 after power on, 0x1E while the menu runs).

| Range | Function | Evidence |
|---|---|---|
| 0xX0000000-0xX000003F | Firmware header (key code) | IPL3 reads the CRCs and entry point. Measured |
| 0xX0000040-0xX0000FFF | Not answered (the game's IPL3 shows through) | Firmware is all zero here. Measured |
| 0xX0001000-0xX001FFFF | Firmware | Measured. Key table CRCs match over this range |
| 0xX0020000-0xX03FFFFF | Reads as zero | Measured (needed for the checksums to match) |
| 0xX0400000 (read) | Status. bit 10 = GS button (0 = pressed), bit 4 / bits 3-0 = PC link | trainer 0x80793870, shell 0x802176F4 |
| 0xX0400400 (write) | Address latch (0x10 \| low 4 bits) | 0x80040600 / 0x80217CD0 |
| 0xX0400600 (write) | PC link control | 0x802176B4 |
| 0xX0400800 (write) | 7-segment LED shift register (bit 8 data, bit 9 clock) | 0x8004066C, 0x80206770 |
| 0xX0500000-0xX05FFFFF | PC link output latches | 0x80206238, 0x80217694 |
| 0xX0600000-0xX07FFFFF | Register writes here do not move the cartridge | Section 3.3 |
| 0xX0C00000-0xX0DFFFFF | Flash, 256KB, read | Boot loader ROM 0x1080, 0x80218040 |
| 0xX0E00000-0xX0FFFFFF | Flash, 16-bit word access (commands / programming) | 0x80217DE0, 0x80217E48 |

Everything else is answered by the game cartridge plugged in to the top.

### 3.2 Boot sequence

1. At power on the GameShark sits at 0x10000000 and hides the game. The PIF runs the
   game's IPL3 (0x40-0xFFF).
2. The IPL3 loads 1MB from 0x10001000 to the entry point in the GameShark header and
   compares its checksum with CRC1/CRC2 in the header (the key code). It stops on a
   mismatch.
3. The position-independent boot loader copies 0x3F000 bytes from 0xB0C01000 to
   0x80040000, continues there, and decompresses `shell.bin` to 0x80200400 and
   `trainer.bin` to 0x80780800.
4. The shell first writes 0x1E to the latch (0x80200424). From then on the GameShark is
   at 0x1E000000 and the game is visible at 0x10000000.

### 3.3 Writes to the 0x600000 block

During initialisation the shell writes 0x12 to 0x1E600400, reads 0x12C00000 and
compares it with 0x80371240, then writes 0x1E to 0x12600400 (0x802078D0). When the
compare matches, the ROM flasher code (0x80207CE8) keeps writing the flash at
0x1EE00000 after the same writes. Writes through the 0x600000 block are therefore taken
as not moving the GameShark itself, and are ignored. What they actually do (possibly
switching something on the cartridge above) is not known.

### 3.4 Flash

- 16 bits wide. Write address is `0xE00000 | (word bit 0 << 20) | ((word & ~1) << 1)`;
  the data is the upper 16 bits of the 32-bit write.
- Commands: 0x5555=0xAAAA, 0x2AAA=0x5555, then 0x5555=0xA0A0 (page write, 128 words)
  or 0x5555=0x8080 ... 0x5555=0x1010 (chip erase).
- After a write, word 0 is read twice until bit 6 stops toggling (0x80217EB8).

### 3.5 Game start and resident code

- 0x80200488: writes the code list pointer to 0xA0000200 and osMemSize to 0xA0000318,
  and installs the exception hook.
- 0x802005DC: places the start stub (0x8021840C) at the game entry point + 1MB (or
  0x80000400). The stub copies 1MB from 0xB0001000 to the entry point, applies the
  F0/F1 codes and jumps to the game.
- 0x80218190: copies the hook to 0x80000180 and sets WatchLo = 0x181 (watch for writes
  to physical 0x180).
- Hook (0x802181F0): on a watch exception it jumps to 0xBEC2F000 (in the GameShark
  flash), on an SI interrupt to the code engine, otherwise to the game's handler at
  0x80000120.
- 0xBEC2F000: in the saved registers, replaces any value of 0x80000180 with 0xA0000120
  and returns. When the game installs its exception vector it is written to
  0x80000120 instead, and the GameShark hook stays in place.
- Trainer (0x80793870): on every SI interrupt it reads bit 10 of the status register;
  when it is 0 it waits for it to return to 1 and enters the in-game menu.

## 4. Implementation in Project64

### 4.1 Files added and changed

| File | Change |
|---|---|
| `Project64-core/N64System/GameSharkCart.h/.cpp` (new) | The cartridge: address decoding, registers, flash, key code check, IPS |
| `N64System/SystemGlobals.h/.cpp`, `AppInit.cpp` | `g_GameShark`, creation and cleanup |
| `MemoryHandler/RomMemoryHandler.cpp` | Cartridge bus reads and writes go to the GameShark first |
| `MemoryHandler/PeripheralInterfaceHandler.cpp` | PI DMA reads go to the GameShark first |
| `MemoryHandler/SerialInterfaceHandler.cpp` | 0x04800014, which the boot loader polls instead of SI status, reads as 0 (no debugger break) |
| `Mips/MemoryVirtualMem.h/.cpp` | Opcode fetch from the GameShark flash, write watch exception, LW from unpopulated RDRAM reads 0 |
| `Recompiler/x86/x86RecompilerOps.cpp` | Same LW rule for constant addresses |
| `Mips/Register.cpp` | Writing WatchLo updates the watch point |
| `N64System/N64System.h/.cpp` | Firmware detection, game ROM selection, key check, reset, GS button, CPU core selection, recent ROM entry |
| `Notification.h`, UI / Android `Notification.h/.cpp` | Game ROM selection dialog |
| `Multilanguage.h`, `Language.cpp`, `Lang/English.pj.Lang`, `Lang/Japanese.pj.Lang` | Messages 2069-2073 |
| `Project64-core.vcxproj(.filters)`, Android `CMakeLists.txt` | New source files |

### 4.2 Behaviour

- Detection: a file of 256KB or less whose internal name is `(C) MUSHROOM &NU` and
  whose bytes 0x40-0xFFF are all zero is treated as GameShark firmware (z64, v64 or n64
  byte order; files inside zip/7z are not detected).
- Every way of opening a ROM (menu, ROM browser, drag and drop, command line) goes
  through `CN64System::RunFileImage`, where the detection is done.
- For firmware, a message shows the active key code and states that only a game with
  the matching CIC can start; then the game ROM selection dialog opens. The key code is
  shown as `name / CIC-xxxx (CRC1 CRC2 check digit)`, e.g.
  `Zelda / CIC-6105 (A9245352 5F737737 7D)`:
  - Name and entry point come from the key table entry matching the header CRCs.
  - CIC: 6102/6103/6106 by calculating the checksum; otherwise by the entry point of
    the key table entry (0x80190000 = 6105, 0x80201000 = 5101 once 6103 is ruled out).
  - Check digit: low byte of CRC1 + CRC2 + entry point + the four header words at
    0x30-0x3C. This matches all four entries of the v3.30 key table. The entry point in
    the header itself is not used; it is the same for every key.
- The game is loaded as `g_Rom` as usual, so its settings (save type etc.) are used.
  The GameShark is inserted on the bus as `g_GameShark`. The game's own IPL3 runs
  unchanged.
- The checksum for the game's CIC is calculated; when it is known not to match the key
  code, the user is asked whether to continue (it would not start on a console). CICs
  that can not be checked (5101 etc.) start without asking.
- Both the game ROM and the firmware file are added to the recent ROM list, the
  firmware on top. Choosing the firmware from the list starts again with the game ROM
  selection; choosing the game ROM starts the game without the GameShark. When the
  start is cancelled at the key code warning, only the firmware is added.
- Reset puts the GameShark back in its power-on state (latch 0x10).
- The existing GS button in the System menu also presses the button on the cartridge.

### 4.3 IPS

- Saved next to the firmware with the same name and an `.ips` extension
  (`gsv33.n64` -> `gsv33.ips`).
- The dialog shown when the firmware is opened does not mention the IPS file; the
  text was shortened to stay well below the 800-byte limit of a string in the language
  files (`MAX_STRING_LEN` in `CLanguage::GetNextLangString`). A longer string is cut off,
  which can split a UTF-8 character.
- The patch is made against the firmware file as stored on disk (its byte order), so
  it can be applied to the original file with common IPS tools.
- An existing IPS is applied when the firmware is loaded. When the flash is back to
  the content of the file, the IPS file is deleted.
- Saved on the first flash read after a write, on a latch change, on reset and when
  emulation ends.

### 4.4 Key code dialog colour

When the key code is changed, the firmware draws the "Are you sure you have this game?"
box with the routine at 0x8021126C. The caller (0x80207028) passes 0x00280028 where the
routine expects an index into a 12-byte table at 0x80227C18, so the colours are read
from 0x82027DF8 (physical 0x02027DF8), an address with no
RDRAM behind it. Project64 returned an address pattern (0x7DF87DF8 ...) for 32-bit
loads there and the box came out yellow; on a console it is black. With zero returned
the test environment draws the box black, matching the console. Byte, halfword and
doubleword loads already returned zero in this range; 32-bit loads from the RDRAM
address space beyond the installed RDRAM (up to 0x03EFFFFF) now do the same.

This was confirmed on a console with the test ROM rdram_unmapped v1.1 (Nekokabu,
2026-11-13, NTSC console with 8 MiB): loads of every width from the unpopulated RDRAM
range return zero, as do cached reads and RSP DMA; writes there are ignored and there
is no mirroring. In 0.2-1.6% of reads the console returned values that look like RDRAM
device registers instead, at a rate that depends on bus activity; this is not
reproduced.

### 4.5 Design decisions

- **The CPU core is forced to the interpreter while a GameShark is attached.** The
  firmware depends on the WatchLo write watch exception. The recompiler writes some
  stores (for example to constant addresses) without going through the memory maps, so
  it can not raise the exception reliably. It is only emulated accurately in the
  interpreter.
- Only the write watch (WatchLo bit 0) is implemented, and only while a GameShark is
  attached. Games without a GameShark behave as before.
- The GameShark state is not part of save states. After loading a state the latch is
  assumed to be 0x1E.

## 5. Verification

1. **Model check against the real firmware.** `GameSharkCart.cpp` was built unchanged
   as a shared library and `gsv33.n64` was run in a minimal N64 environment on Unicorn
   (a CPU emulator), with Baku Bomberman 2 as the game. The IPL3 step (1MB load) was
   done by the test script.
   - Boot loader -> shell -> main menu shown (screen checked).
   - "Start Game With Selected Codes" -> reaches the game entry point 0x80000400.
     The hook at 0x80000180, the code list at 0x80000200 and the game code from
     0x80000400 were checked.
   - Options: "Scroll Background" set to Off, then Exit -> a 14-byte IPS
     (offset 0x2FB04: 01 -> 00) is written; the firmware file is unchanged.
   - Restart with the IPS present -> "Scroll Background - Off" is shown.
   - Key check: 6101/6102 = match, 6103/6105/6106 = mismatch, 5101 = can not tell.
   - All console registers accessed from boot to game start were listed; this found
     the boot loader's read of 0x04800014.
2. **Formatting and syntax.** All changed C++ files pass clang-format 12
   (`--dry-run -Werror`, as used by `Source/Script/clang.cmd`) and a g++ syntax check.
3. **Windows build (Nekokabu, 2026-11-11).** Builds with Visual Studio. Confirmed:
   - firmware boot to the menu and the firmware detection message,
   - starting a game, and starting a game after changing the key code,
   - the in-game trainer (GS button),
   - the IPS file is written and the firmware file is not modified,
   - the confirmation message for a game whose CIC does not match the key code,
   - the firmware and the game ROM in the recent ROM list,
   - a game with CIC 6105,
   - Pro Action Replay v3.20 firmware boots (only v3.30 was analysed).

## 6. Open points and limitations


- Only v3.30 (09:54 Mar 27) was analysed. Pro Action Replay v3.20 has been seen to
  boot, but other versions were not analysed and may use a different register layout.
- With the interpreter forced, demanding games may run slower.
- PC link (parallel port) and the LED display are not implemented (writes are accepted
  and ignored).
- The meaning of status register bits other than bit 10, bit 4 and bits 3-0 is not
  known; they read as 0.
- What 0x04800014 returns on a console has not been measured; it reads as 0.
- As on a console, the hook is lost when a game clears WatchLo
  (Baku Bomberman 2 needs `F1071F34 2400`).

## 7. Test procedure (Windows build)

1. Build and open `gsv33.n64` -> the message and the game ROM selection dialog appear.
2. Select Baku Bomberman 2 (CIC 6102) -> after the LED countdown the menu appears.
3. Change an option and exit -> `gsv33.ips` is created and the time stamp of
   `gsv33.n64` does not change.
4. Start Game -> the game starts and the codes work.
5. With the code generator enabled (8MB), press the GS button in the System menu during
   the game -> the in-game menu appears.
6. Select a game with a different CIC (e.g. 6103) -> a confirmation message appears.
7. Check that `gsv33.n64`, not the game ROM, is at the top of the recent ROM list.
