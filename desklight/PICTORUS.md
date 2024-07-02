In the Pictorus Embedded Rust Challenge, I didn't manage to get
USB-Serial input to work, so for this challenge, instead of creating a
mock device, I used an existing device programmed in C++ with a little
adapter code to translate from the Desk Light serial interface to the
Pictorus challenge API.

[Git Lab Desk Light](https://gitlab.com/pictographer/desklight)
has the documentation and source for Desk Light.

Desk Light uses an Arm Cortex-M0 development board from PJRC called a
Teensy LC.