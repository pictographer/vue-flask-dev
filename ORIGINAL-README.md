# Pictorus Fullstack + Embedded Developer Challenge

This challenge consists of a monorepo defining a very basic fullstack application with the following services (see individual READMEs for more details):

- [Client](./client/README.md): A VueJs application that allows a user to connect with a target device.
  - If you are not familiar with VueJs, you can use any other frontend framework you are comfortable with.
- [Server](./server/README.md): A Python flask server that facilitates interactions between the client and the device.
- [Device](./device/README.md): A Rust application that will run on a target embedded device.
  - You may use any embedded device you have available to you. If you don't have a physical device to test with you can use an emulator like QEMU, or just target a STM32 dev board of your choice and do your best come up with a solution that we can test on our end.

The goal of this challenge is to get the client to connect to the server, and the server to connect to the device. The device should be able to receive commands from the server and send responses back to the server and, in turn, the client.

## Getting Started

In order to get started you will need a recent version of npm, Python, and Rust installed on your machine. You can then run `./script/setup` to install the basic dependencies for each project. There is also a `./script/start` script that will start each service as a separate process.

Feel free to modify these scripts, ignore them, or set up your own build/run process.

## Requirements

Note: Ideally, this exercise shouldn't take more than a couple of hours. If you find yourself spending much more time than that, feel free to stop and submit what you have. We will assess based on the quality of the code submitted, and it's fine not to complete every aspect.

1. The client must allow the user to select from a list of devices returned from the /api/devices endpoint of the server (fine to use hardcoded data on the server).
2. The client should show the state of the device's LED as indicated by the device state in the API.
3. The client should allow the user to toggle an LED on the device by sending a request to a new API endpoint you define on the server. The updated LED state should be reflected in the UI.
   - The newly defined endpoint should communicate with the device over a serial connection to toggle the LED state. The device should respond with the new state of the LED, which should in turn be returned to the client.
   - The device application must communicate over a serial connection on the development board, and should change the state of one of a GPIO on the board.
     - If you're using a STM32 dev board please use the ST-Link serial COM port for communication, and one of the "User LEDs" on the board (LD1, LD2, or LD3).

### Bonus

1. Move the embedded serial handling into separate task(s)/thread(s) so it does not block the main execution loop.
2. The demo client is super ugly. Make it look less bad!
3. Determine the available device(s) dynamically in the API. This only needs to work with debug probes, so you can use something like probe-rs to scan for devices.
4. Add an additional control to the UI that allows the user to set a blink rate for the button. This can be a text input, toggle, slider, etc. The device should then blink the LED at the specified rate.

## Assesment Criteria

The goal of this exercise is to show us your ability to write production code in various parts of the stack. It's more important to write good, clean code than to complete all the requirements. Things we will be looking for:

- Well-structured, clean code
- Helpful comments and documentation
- Tests where appropriate

## Submission

Please email your response back as a zip file containing the entire monorepo. Any additional documentation/notes should be included in a markdown/text file in the root of the repo.

Note: An easy way to do this is to commit all of your changes, and then run `git archive -o challenge.zip HEAD` from the root directory.
