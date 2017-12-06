
To make mcs files for CLIF2F boards, use the command line:

$ ./quickboot_builder --output=CLIF2F_qb4_2A0.mcs --gold=CLIF2F_qbgold4_2A0.bit --silver=CLIF2F_silver4_2A0.bit --spi
  Reading gold file: CLIF2F_qbgold4_2A0.bit
  Reading silver file: CLIF2F_silver4_2A0.bit
  Extracted WBSTAR register value: 0x40400000
  Extracted multiboot BYTE address: 0x00400000
  MULTIBOOT Address: 0x00400000
  PROM Page Size: 4096 bytes
  Quickboot SPI header
  Critical Switch word is aa:99:55:66 at 0x00000ffc (page 0)

The output repeats the names of the input files, reports the WBSTAR
value found in the gold file (and the multiboot address inferred) and
reports what and where the critical switch word is. The last part may
vary depending on input flag settings.

To make mcs fies for ICB2F boards, use the command line:

$ ./quickboot_builder --output=ICB2F_qbN_MMM.mcs --gold=ICB2F_qbgoldN_MMM.bit --silver=ICB2F_silverN_MMM.bit --bpi16

In this case, the --bpi16 selects the bpi mode, and a different
quickboot header will be written to support this chip type.

** NOTE: The --bpi16 mode is still under development.
