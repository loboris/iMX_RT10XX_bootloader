# iMX RT10XX bootloader
*Bootloader for mimrxt MicroPython port*

The bootloader must be programed into RT10XX qspi Flash (or Hyperflash, not tested) using one of the usual flashing ways.
Once loaded, it enables loading the firmware into flassh over the CDC/ACM port using the provided **Mflash.py** program loader, written in **Python** and capable of running any **Linux** or **Windows** machine (probably also on OSX, not yet tested).
Bootloader itself occupies the first **64MB** of flash, the rest can be used for user firmware(s).
The firmware must be linked at start address equal or higher than `0x60010000`.

**Main features:**

* works on any i.MX RT series MCU (tested with RT1052 and RT1062)
* capable of loading one of **two** firmwares form Flash, ideal for OTA program upgrade (the new version can be * loaded from application itself)
* if valid application would start, the bootloader can be entered by pressing the user button on board
* LED indication of operation state
* very secure, two copies of the boot configuratin sectors (main and backup) are provided, if the main is corrupted it is restored from backup
* the firmware (user application) is protected and verified on boot by 32-byte SHA256 hash
* very fast communication with the loader program (~500 MB/sec), Flash program opperation is, of course, slower and depend on how much sectors must be erased
* this bootloader was build for use with **MicroPython** firmwares, but any firmware can be used, as long as it was correctly linked for start address of 0x60010000 or higher
* provided (Python) loader program features:
  * programming the firmware to the specific Flash address (taken from the firmware file IVT section)
  * reading any Flash area into file
  * erasing any flash area
  * getting the information about boot configuration
  * bootloader does not permit accidental programming of its own Flash area
  * works wel bot on Linux and Windows, (**_pyserial_** module must be installed)


**Boot loader boot sector structure:**
> | Offset | Size | Name | Description |
> | ---: | ---: | :---: | :---: |
> |      0 | 16 | bootSectID | string ID: `i.MXRT10XX_boot` |
> |     16 | 60 | app1ConfigRecord | Application #1 config record |
> |     76 | 60 | app2ConfigRecord | Application #2 config record |
> |    136 | 4 | bootSectCRC32 | Boot sector CRC32 |

**Application config record structure:**
> | Offset | Size | Name | Description |
> | ---: | ---: | :---: | :---: |
> | 0 | 16 | appName | application name, NULL terminated string |
> | 16 | 4 | appFlashAddress | application address in Flash (min addr 0x600100000) |
> | 20 | 4 | appSize | application size, 24-bil; upper 8-bits are *flags*<br>Application size range is `0x10000` - `0x200000` (64KB - 2MB) |
> | 24 | 4 | appTimestamp | application timestamp written by Loader |
> | 28 | 32 | appSHA256 | application's SHA-256 hash calculated over **appSize** bytes from **appFlashAddress** |

*Bits `24-31` of the* **appSize** *field are used as application* **_flags_**:
| Bit | Comment |
| :---: | :--- |
| `24` | **Active** flag, if set the application will be loaded and executed.<br>If multiple entries have **active** flag set, the first one will be loaded and executed.<br>If no application has active flag set, the applications will be tested for valid address, size and SHA256 hash an the first one which passes will be started. |
| `25` | Not used, reserved for future use |
| `26` | Not used, reserved for future use |
| `27` | Not used, reserved for future use |
| `28` | Not used, reserved for future use |
| `29` | Not used, reserved for future use |
| `30` | Not used, reserved for future use |
| `31` | Not used, reserved for future use |

_Notes_:

*More information will be added soon*
