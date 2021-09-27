Demos
=====

This is just to demonstrate genaral capabilities of the TFT28-105.
It can be retrieved from git repository with the `BasicDemo tag`_.

For enhanced DmTftLibrary it is recommended to stick with *master* branch and
use the DmTft and DmTouch source directories.

.. _BasicDemo tag: https://github.com/OliverSwift/TFT28-105-STM32WB55/tree/BasicDemo

Basic demo
----------

The demo application shows the display possibilities:

.. video:: _static/demo.mp4
    :width: 300

You can see that screen clearing and image drawing is decently fast.

Let's add the BLE scanner function since we seat on a WB55.

BLE scanner
-----------

The basic BLE code to install stack and make it run is MX generated. Then, a simple scan is scheduled by calling **aci_gap_start_general_discovery_proc**.
The code waits for **le_advertising_event** to collect RSSI, visible name and remote addresses and reports them to the application that eventually draws the information
onto the screen.

.. image:: images/ble_scanner.jpg
