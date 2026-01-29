.. SPDX-License-Identifier: GPL-2.0+

phyFLEX-i.MX 91/93
==================

U-Boot for the phyFLEX-i.MX 91/93. Please follow the correct steps below.

Please note, as the hardware for the i.MX 91 variant of the SOM is not yet
officially available, the i.MX 91 implementation is not yet finally done.

Quick Start
-----------

- Get and Build the ARM Trusted firmware
- Get the DDR firmware
- Get ahab-container.img
- Build U-Boot

Get and Build the ARM Trusted firmware
--------------------------------------

Note: srctree is U-Boot source directory
Get ATF from: https://github.com/nxp-imx/imx-atf/
branch: lf_v2.12

.. code-block:: bash

   $ unset LDFLAGS
   $ make PLAT=imx93 bl31
   $ cp build/imx93/release/bl31.bin $(srctree)

Get the DDR firmware
--------------------

.. code-block:: bash

   $ wget https://www.nxp.com/lgfiles/NMG/MAD/YOCTO/firmware-imx-8.21.bin
   $ chmod +x firmware-imx-8.21.bin
   $ ./firmware-imx-8.21.bin
   $ cp firmware-imx-8.21/firmware/ddr/synopsys/lpddr4*.bin $(srctree)

Get ahab-container.img
---------------------------------------

.. code-block:: bash

   $ wget  https://www.nxp.com/lgfiles/NMG/MAD/YOCTO/firmware-ele-imx-1.3.0-17945fc.bin
   $ chmod +x firmware-ele-imx-1.3.0-17945fc.bin
   $ ./firmware-ele-imx-1.3.0-17945fc.bin
   $ cp firmware-ele-imx-1.3.0-17945fc/mx93a1-ahab-container.img $(srctree)

Build U-Boot
------------

.. code-block:: bash

   $ make imx93-phyflex_defconfig
   $ make

Burn the flash.bin to MicroSD card offset 32KB:

.. code-block:: bash

   $ dd if=flash.bin of=/dev/sd[x] bs=1024 seek=32 conv=notrunc
