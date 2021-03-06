
*** To make mcs files for CLIF2F boards using quickboot_builder2, use
*** the command line:

$ ./quickboot_builder --output=CLIF2F-32-4-6_3209.mcs \
           --clif32-4=CLIF2F-silver4_3209.bit \
           --clif32-6=CLIF2F-silver6_3209.bit
Reading CLIF32-4 silver file: CLIF2F_silver4_3209.bit
Reading CLIF32-6 silver file: CLIF2F_silver6_3209.bit
Processing CLIF32-4 design...
... AXSS (gold): 0x474f4c44 (was: 0x53494c56)
... BSPI (gold): 0x0000000c (was: 0x0000000b)
... BSPI (silver): 0x0000000c (was: 0x0000000b)
... Write GOLD image at byte address 0x00020000
... Write SILVER image at byte address 0x00400000
... Critical Switch word is aa:99:55:66 at 0x0000fffc
Processing CLIF32-6 design...
... AXSS (gold): 0x474f4c44 (was: 0x53494c56)
... BSPI (gold): 0x0000000c (was: 0x0000000b)
... BSPI (silver): 0x0000000c (was: 0x0000000b)
... Write GOLD image at byte address 0x00820000
... Write SILVER image at byte address 0x00c00000
... Critical Switch word is aa:99:55:66 at 0x0080fffc
Done processing designs, writing mcs file.
MCS target device size >= 0x01000000

The output gives the important details for each design, including
where the critical switch word for that design lives. The design
collection can be up to four designs. If multiple designs are
specified, then the MCS file will contain the contiguous image that
writes all the designs to the flash. If only a single design is given,
then the MCS file will contain the image, properly offset in the
target flash, for only the target flash.


*** To make mcs files for CLIF2F boards from only a silver file, use the command line:

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

*** To make mcs files for ICB2F boards, use the command line:

$ ./quickboot_builder --output=ICB2F_qbN_MMM.mcs --silver=ICB2F_silverN_MMM.bit --bpi16
No gold file, using silver file.
Reading gold file: ICB2F_silver4_441.bit
Reading silver file: ICB2F_silver4_441.bit
MULTIBOOT Address: 0x00800000
PROM erase block Size: 262144 bytes
AXSS (gold): 0x474f4c44 (was: 0x53494c56)
COR0 (gold): 0x062055dc (was: 0x066055dc)
COR1 (gold): 0x0000000a (was: 0x00000000)
Write GOLD image at byte address 0x00080000
Write SILVER image at byte address 0x00800000
Quickboot BPI header
Critical Switch word is 00:00:00:bb 11:22:00:44 aa:99:44:66 at 0x0003fff4 (page 0)
WBSTAR (quickboot header): 0x60400000
MCS target device size >= 0x00e61edc

In this case, the --bpi16 selects the bpi mode, and a different
quickboot header will be written to support this chip type.

*** To make a CLIF gold image (a .bit file) from a silver image:

$  ./quickboot_gold --output=CLIF2F_gold6_2A6.bit --silver=CLIF2F_silver6_2A6.bit --spi
Reading silver file: CLIF2F_silver6_2A6.bit
AXSS (gold): 0x474f4c44 (was: 0x53494c56)

*** To make a ICB gold image (a .bit file) from a silver image:

$  ./quickboot_gold --output=CLIF2F_gold6_2A6.bit --silver=CLIF2F_silver6_2A6.bit --bpi16
Reading silver file: CLIF2F_silver6_2A6.bit
AXSS (gold): 0x474f4c44 (was: 0x53494c56)
