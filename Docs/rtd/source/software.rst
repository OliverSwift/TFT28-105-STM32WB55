Software
========

Software packages and Licenses
------------------------------

+--------------+------------+
| Package      | Version    |
+==============+============+
| STM32CubeIDE | 1.7        |
+--------------+------------+
| STM32CubeMX  | 6.3.0      |
+--------------+------------+
| FW_WB        | 1.12.1     |
+--------------+------------+
| DmTftLibrary | 1.4        |
+--------------+------------+

Major part of the code is covered by ST. Check out respective agreements for proper use:

+-----------------------------+--------------------+-------------------------------------------+
| Component                   | Copyright          | License                                   |
+=============================+====================+===========================================+
| Original application source | STMicroelectronics | `ST License agreement`_                   |
+-----------------------------+--------------------+-------------------------------------------+
| Cortex®-M CMSIS             | ARM Ltd            | `BSD-3-Clause`_ or `Apache License 2`_    |
+-----------------------------+--------------------+-------------------------------------------+
| DisplayModule library       | DisplayModule      | Proprietary                               |
+-----------------------------+--------------------+-------------------------------------------+

The DmTftLibrary has been drastically modified, however, original copyright must be preserved.

.. _ST License agreement: https://www.st.com/content/ccc/resource/legal/legal_agreement/license_agreement/group0/39/50/32/6c/e0/a8/45/2d/DM00218346/files/DM00218346.pdf/jcr:content/translations/en.DM00218346.pdf
.. _BSD-3-Clause: https://opensource.org/licenses/BSD-3-Clause
.. _Apache License 2: https://opensource.org/licenses/Apache-2.0

Development requirements
------------------------

My favorite development environment is Linux but ST tools are available for Mac and Windows as well. The first two are easier to setup, usually no issues or driver problems. Well, real tech savvy's know that already.

Although I'm not a great fan of IDEs, STM32CudeIDE works very well (Eclipse variant though) and ST plugins helped (MX, Software expansions download,...).

The project can easily be open in STM32CudeIDE without dependencies. All code is there no need to install WB from ST. Open  the project and build it (tested on Linux and MacOS).

For the BLE part to work, you need the BLE Stack firmware installed for CPU2. Here, **stm32wb5x_BLE_Stack_full_fw.bin** has been flashed. Every stacks firmware are provided in **Projects/STM32WB_Copro_Wireless_Binaries/STM32WB5x** directories of STM32CubeWB firmware package as well as how to program the boards for this. It needs to be done **once**.

