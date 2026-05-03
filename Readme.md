# Backup-Srl
BUP memory module - full replacement for sega_bup.h
Note: this is an "experimental" build.  It is functional, but there may be bugs or imcomplete features.
Currently, it does not support the FDD or folders.

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
SRL::Backup;
```
See the example in `/Samples/Backup Memory` for use
