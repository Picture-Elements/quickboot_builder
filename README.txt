
To make mcs files for CLIF2F boards from only a silver file, use the command line:

$ ./quickboot_builder --output=CLIF2F_qb4_2A0.mcs --silver=CLIF2F_silver4_2A6.bit --spi
No gold file, using silver file.
Reading gold file: CLIF2F_silver4_2A6.bit
Reading silver file: CLIF2F_silver4_2A6.bit
MULTIBOOT Address: 0x00400000
PROM erase block Size: 4096 bytes
AXSS (gold): 0x474f4c44 (was: 0x53494c56)
Write GOLD image at byte address 0x00002000
Write SILVER image at byte address 0x00400000
Quickboot SPI header
Critical Switch word is aa:99:55:66 at 0x00000ffc (page 0)

The output repeats the names of the input files, reports the multiboot
address that is inferred, and reports what and where the critical
switch word is. The last part may vary depending on input flag
settings. With this incantation, the gold file is generated from the
silver file.

To make mcs fies for ICB2F boards, use the command line:

$ ./quickboot_builder --output=ICB2F_qbN_MMM.mcs --gold=ICB2F_qbgoldN_MMM.bit --silver=ICB2F_silverN_MMM.bit --bpi16

In this case, the --bpi16 selects the bpi mode, and a different
quickboot header will be written to support this chip type.

** NOTE: The --bpi16 mode is still under development.
