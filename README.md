# mempatcher
[![Build](https://github.com/aixxe/mempatcher/actions/workflows/build.yml/badge.svg)](https://github.com/aixxe/mempatcher/actions/workflows/build.yml)

Drop-in replacement for [mempatch-hook](https://github.com/djhackersdev/bemanitools/blob/master/doc/tools/mempatch-hook.md) with a few extra features

- Auto-loads any `.mph` files from `autopatch` directory
- Supports file-based offsets by prefixing addresses with `F+`
- Uses [loader hooks](https://aixxe.net/2024/09/dll-memory-patching) to ensure patches are applied before entrypoint call
- Can be loaded ahead of target libraries, will unload after applying patches

### Usage

<details><summary><b>Patch file format</b></summary>

### `patch.mph`

```yaml
# Validate: LDJ-003-2023090500
bm2dx.dll F+170 - F50FEF64
bm2dx.dll F+190 - 54770301

## Enable 1P Premium Free
bm2dx.dll A271FC EB 75

## Skip Monitor Check
bm2dx.dll ABC2F2 8D 8C

## Choose Skip Monitor Check FPS
### 60 FPS
# bm2dx.dll 659B10 48B80000000000004E4066480F6EC0F20F58C8C3 448B91480B0000448BCA4C8BD94181C267010000
### 120 FPS
bm2dx.dll AE55F0 48B80000000000005E4066480F6EC0F20F58C8C3 8954241048894C24084883EC28488B4424308B80

# Omnimix
bm2dx.dll F+557C0E 9090 7407
bm2dx.dll F+902E69 9090 743C
bm2dx.dll F+981202 B001 32C0
bm2dx.dll F+9936AD EB 75
bm2dx.dll F+AF8E0B 90909090909090909090909090909090909090909090909090909090909090909090909090909090909090909090909090909090909090909090909090909090909090909090909090909090909090909090909090 0FB644244085C00F84F800000048837C2448000F84EC000000488B4424480FB6400383F8587539488B4424480FB6400483F858752B488B4424480FB6400583F858751D488B442448C640034A488B442448C6400442
bm2dx.dll F+AF8E68 58 41
bm2dx.dll F+AFD481 FFFF 0807
bm2dx.dll F+AFD485 9090 7F0A
bm2dx.dll F+AFD48B FFFF 0807
bm2dx.dll F+11BD5EF 6F 61
bm2dx.dll F+127492B 6F6D6E69 64617461
```

</details>

<details><summary><b>Expected output log</b></summary>

### `log.txt`

```
[2024/09/25 14:03:04] I:launcher: loading early hook DLL mempatcher.dll
```

### `mempatcher.log`

```
[2024/09/25 14:03:04] mempatcher (v0.1.0.0) loaded at 0x7FFBAB6D0000
[2024/09/25 14:03:04] Compiled at Sep 25 2024 13:54:56 from master@2897d111
[2024/09/25 14:03:04] Command line arguments: -w -iidx -io -modules modules bm2dx.dll -iidxtdj --mempatch patch.mph -z mempatcher.dll
[2024/09/25 14:03:04] Opening memory patch file 'patch.mph'...
[2024/09/25 14:03:04] Parsed check on line 1 at file 'bm2dx.dll'+0x170
[2024/09/25 14:03:04]      expected data [4]: F5 0F EF 64
[2024/09/25 14:03:04] Parsed check on line 2 at file 'bm2dx.dll'+0x190
[2024/09/25 14:03:04]      expected data [4]: 54 77 03 01
[2024/09/25 14:03:04] Parsed patch on line 5 at RVA 'bm2dx.dll'+0xA271FC
[2024/09/25 14:03:04]      expected data [1]: 75
[2024/09/25 14:03:04]   replacement data [1]: EB
[2024/09/25 14:03:04] Parsed patch on line 8 at RVA 'bm2dx.dll'+0xABC2F2
[2024/09/25 14:03:04]      expected data [1]: 8C
[2024/09/25 14:03:04]   replacement data [1]: 8D
[2024/09/25 14:03:04] Parsed patch on line 14 at RVA 'bm2dx.dll'+0xAE55F0
[2024/09/25 14:03:04]      expected data [20]: 89 54 24 10 48 89 4C 24 08 48 83 EC 28 48 8B 44 24 30 8B 80
[2024/09/25 14:03:04]   replacement data [20]: 48 B8 00 00 00 00 00 00 5E 40 66 48 0F 6E C0 F2 0F 58 C8 C3
[2024/09/25 14:03:04] Parsed patch on line 17 at file 'bm2dx.dll'+0x557C0E
[2024/09/25 14:03:04]      expected data [2]: 74 07
[2024/09/25 14:03:04]   replacement data [2]: 90 90
[2024/09/25 14:03:04] Parsed patch on line 18 at file 'bm2dx.dll'+0x902E69
[2024/09/25 14:03:04]      expected data [2]: 74 3C
[2024/09/25 14:03:04]   replacement data [2]: 90 90
[2024/09/25 14:03:04] Parsed patch on line 19 at file 'bm2dx.dll'+0x981202
[2024/09/25 14:03:04]      expected data [2]: 32 C0
[2024/09/25 14:03:04]   replacement data [2]: B0 01
[2024/09/25 14:03:04] Parsed patch on line 20 at file 'bm2dx.dll'+0x9936AD
[2024/09/25 14:03:04]      expected data [1]: 75
[2024/09/25 14:03:04]   replacement data [1]: EB
[2024/09/25 14:03:04] Parsed patch on line 21 at file 'bm2dx.dll'+0xAF8E0B
[2024/09/25 14:03:04]      expected data [85]: 0F B6 44 24 40 85 C0 0F 84 F8 00 00 00 48 83 7C 24 48 00 0F 84 EC 00 00 00 48 8B 44 24 48 0F B6 40 03 83 F8 58 75 39 48 8B 44 24 48 0F B6 40 04 83 F8 58 75 2B 48 8B 44 24 48 0F B6 40 05 83 F8 58 75 1D 48 8B 44 24 48 C6 40 03 4A 48 8B 44 24 48 C6 40 04 42
[2024/09/25 14:03:04]   replacement data [85]: 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90
[2024/09/25 14:03:04] Parsed patch on line 22 at file 'bm2dx.dll'+0xAF8E68
[2024/09/25 14:03:04]      expected data [1]: 41
[2024/09/25 14:03:04]   replacement data [1]: 58
[2024/09/25 14:03:04] Parsed patch on line 23 at file 'bm2dx.dll'+0xAFD481
[2024/09/25 14:03:04]      expected data [2]: 08 07
[2024/09/25 14:03:04]   replacement data [2]: FF FF
[2024/09/25 14:03:04] Parsed patch on line 24 at file 'bm2dx.dll'+0xAFD485
[2024/09/25 14:03:04]      expected data [2]: 7F 0A
[2024/09/25 14:03:04]   replacement data [2]: 90 90
[2024/09/25 14:03:04] Parsed patch on line 25 at file 'bm2dx.dll'+0xAFD48B
[2024/09/25 14:03:04]      expected data [2]: 08 07
[2024/09/25 14:03:04]   replacement data [2]: FF FF
[2024/09/25 14:03:04] Parsed patch on line 26 at file 'bm2dx.dll'+0x11BD5EF
[2024/09/25 14:03:04]      expected data [1]: 61
[2024/09/25 14:03:04]   replacement data [1]: 6F
[2024/09/25 14:03:04] Parsed patch on line 27 at file 'bm2dx.dll'+0x127492B
[2024/09/25 14:03:04]      expected data [4]: 64 61 74 61
[2024/09/25 14:03:04]   replacement data [4]: 6F 6D 6E 69
[2024/09/25 14:03:04] Loaded target file 'bm2dx.dll' at address 0x180000000
[2024/09/25 14:03:04] Validated data on line 1 at file 'bm2dx.dll'+0x170 from 0x180000170
[2024/09/25 14:03:04] Validated data on line 2 at file 'bm2dx.dll'+0x190 from 0x180000190
[2024/09/25 14:03:04] Applied patch on line 5 at RVA 'bm2dx.dll'+0xA271FC to 0x180A271FC
[2024/09/25 14:03:04] Applied patch on line 8 at RVA 'bm2dx.dll'+0xABC2F2 to 0x180ABC2F2
[2024/09/25 14:03:04] Applied patch on line 14 at RVA 'bm2dx.dll'+0xAE55F0 to 0x180AE55F0
[2024/09/25 14:03:04] Applied patch on line 17 at file 'bm2dx.dll'+0x557C0E to 0x18055880E
[2024/09/25 14:03:04] Applied patch on line 18 at file 'bm2dx.dll'+0x902E69 to 0x180903A69
[2024/09/25 14:03:04] Applied patch on line 19 at file 'bm2dx.dll'+0x981202 to 0x180981E02
[2024/09/25 14:03:04] Applied patch on line 20 at file 'bm2dx.dll'+0x9936AD to 0x1809942AD
[2024/09/25 14:03:04] Applied patch on line 21 at file 'bm2dx.dll'+0xAF8E0B to 0x180AF9A0B
[2024/09/25 14:03:04] Applied patch on line 22 at file 'bm2dx.dll'+0xAF8E68 to 0x180AF9A68
[2024/09/25 14:03:04] Applied patch on line 23 at file 'bm2dx.dll'+0xAFD481 to 0x180AFE081
[2024/09/25 14:03:04] Applied patch on line 24 at file 'bm2dx.dll'+0xAFD485 to 0x180AFE085
[2024/09/25 14:03:04] Applied patch on line 25 at file 'bm2dx.dll'+0xAFD48B to 0x180AFE08B
[2024/09/25 14:03:04] Applied patch on line 26 at file 'bm2dx.dll'+0x11BD5EF to 0x1811BE9EF
[2024/09/25 14:03:04] Applied patch on line 27 at file 'bm2dx.dll'+0x127492B to 0x181275D2B
[2024/09/25 14:03:04] All patches applied, unloading from process...
```

</details>

### Installation

- Compile from source or download a pre-built version from the [releases](https://github.com/aixxe/mempatcher/releases/) page
- Copy the appropriate `mempatcher.dll` build to your game directory
- Alter your launch command to load the library during startup

Upon launching the process, a `mempatcher.log` file should be created in the working directory  

#### [spice2x](https://spice2x.github.io)

> [!IMPORTANT]
> The `-z` flag requires version **24-09-21** or newer. It is possible to use `-k` instead, but this will result in patches being applied after the game library has been initialized. In some rare cases [this may cause issues](https://github.com/spice2x/spice2x.github.io/issues/220) 

```
spice64.exe [...] -z mempatcher.dll --mempatch patch.mph
```

#### [Bemanitools](https://github.com/djhackersdev/bemanitools)

```
launcher.exe [...] -B mempatcher.dll --mempatch patch.mph
```

#### Generic

Use any method, such as [**proxyloader**](https://github.com/aixxe/proxyloader), to load `mempatcher.dll` into the process