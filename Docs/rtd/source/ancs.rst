iPhone Notification Client Demo
===============================

For those who have an iPhone, this demo shows how to use this module as an ANCS client.
This allows to be notified about incoming calls, messages and being able to accept or reject calls.

The `Apple Notification Client Service`_ is a GATT service hosted on the iPhone. The iPhone role
is central so you cannot connect to it. The iPhone has to connect to your ANCS device which is a
peripheral. So yes, the central runs a GATT server and the peripheral is the GATT client. Most of
the time it's the other way around and developers tend to forget that master/slave (central/peripheral)
and GATT server/client are interchangeable roles.

So, how to make the iPhone connect to your peripheral? There is a special AD type called Service
Sollicitation that has to be added when the device advertises. It becomes visible in the iPhone
devices list in the Bluetooth setiings. Once connected, authorized and bonded, the device is added
in the list and the iPhone will automatically connects to it whenever reachable.

Upon connection, the peripheral starts ANCS services and characteristics discovery and then starts
listening to notifications from the iPhone. Actions can be taken on some notifications. The TFT module
will show up notifications and also gives the user the opportunity to accept/reject incoming calls.

Here is a demo:

.. video:: _static/TFT-ANCS.mp4
    :width: 700

.. _Apple Notification Client Service: https://developer.apple.com/library/archive/documentation/CoreBluetooth/Reference/AppleNotificationCenterServiceSpecification/Introduction/Introduction.html
