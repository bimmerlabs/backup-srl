# Backup-Srl
BUP memory module - full replacement for sega_bup.h

New: Now supports the FDD (including partitions) and file searching!
(Unfortunately, I don't have a FDD, so I was only able to test in an emulator)
Also fixed some bugs

Also requires the SMPC module:
* https://github.com/bimmerlabs/smpc-SRL

## Use
Copy modules_extra to your SRL folder

To include a module in your project, add this line to your project’s Makefile:
```
MODULES_EXTRA = smpc backup
```
Include it in your code:
```
#include <backup.hpp>
using namespace SRL::Backup;
```
See the example in `/Samples/Backup Memory` for use
