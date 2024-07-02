#!/usr/bin/env python3

# TODO remove unused imports
from flask import Flask, render_template, request
from flask_cors import CORS
import glob
import html
import json
import re
import serial
import time

# TODO Add a menu and text box to generate URLs for routes with parameters
# TODO Set the status code for error responses

__version__ = "0.1"
DEVICE_TTY_PATH = '/dev/tty'
DEVICE_PATH_PATTERN = '/dev/ttyACM*'
LED_STATES = ["on", "off", "blinking"]

# Create the application. We need this to define the routes.
app = Flask(__name__)

# WARNING - This disables access control on cross origin requests.
# DO NOT USE THIS IN PRODUCTION!
CORS(app)

# Device-dependent
#
# The two functions below enable me to use my exising desk light
# as the device portion of this demonstration.
#
# Desk Light accepts commands over USB-Serial. It has a strip of
# eight APA102 RGB LEDs, a PIR sensor, a photodiode, a piezo speaker,
# a push button, and a touch sensor, but for this demonstration,
# we're only using four commands:
#    colors - reports the RGB values for each of the LEDs
#    white - sets all the LEDs to white
#    black - turns all the LEDs off
#    alarm - flashes the LEDs red and beeps three times
#
# Desk Light does not support blink frequency.
#
# The response to the colors command looks like this:
#    0: rgb(  40,   1, 215) 
#    1: rgb(  61,   1, 194) 
#    2: rgb(  83,   1, 172) 
#    3: rgb( 103,   1, 153) 
#    4: rgb( 125,   1, 131) 
#    5: rgb( 146,   1, 110) 
#    6: rgb( 168,   1,  88) 
#    7: rgb( 188,   1,  67)
#
# We're using the first LED's state as the overall device state.

def translate_device_state(response):
    """Convert my desklight response into the challenge API"""
    if response.startswith(" 0: rgb"):
        if response.startswith(" 0: rgb(   0,   0,   0)"):
            state = "off"
        else:
            state = "on"
    else:
        state = "error"
    return state

def translate_device_command(command):
    return { "on": "white", "off": "black", "blinking": "alarm" }[command]


# Helpers

def href(url):
    """Generate an href tag from a URL."""

    return "<a href='{url}'>{url}</a>".format(url=html.escape(url))

def is_valid_hz(hz):
    """Is hz a positive fixed point number between 0.1 and 240.0?"""
    
    return (re.fullmatch(r"[0-9.]+", hz)
        and hz.count('.') <= 1
        and float(hz) >= 0.1
        and float(hz) <= 240.0)

def is_valid_led_state(state):
    """Is state valid for an LED?"""

    return state in LED_STATES

def send_device_command(id, cmd):
    """Send a command and return the response"""

    response = ''

    try:
        s = serial.Serial("/dev/tty" + id, timeout=3.0)
    except ValueError as e:
        return "pySerial Error: " + e
    except serial.SerialException as e:
        return str(e)

    # Toss any output leftover from previous output, e.g. status messages.
    count = 16
    while s.inWaiting() > 0 and count >= 0:
        --count
        s.read(s.inWaiting())

    s.write(cmd.encode() + '\n'.encode())
    s.flush()

    # Give the krufty old Linux serial device driver a moment to catch up.
    # TODO find out why this helps and do better.
    time.sleep(1)

    while s.inWaiting() > 0:
        response += s.read(s.inWaiting()).decode()

    return response

def read_state(id):
    """Report the state of device <ID>'s LED"""

    response = send_device_command(id, 'colors')
    state = translate_device_state(response)
    if state == "error":
        content = "device {id} is in state {state}\n{msg}".format(
            id=id, state=state, msg=response)
        return render_template("index.html", content=content)
    else:
        return json.dumps(state)

def write_state(id, state):
    """Set the state of device <ID>'s LED to <STATE>."""

    if is_valid_led_state(state):
        content = "device {id} is in state {state}".format(id=id, state=state)
        command = translate_device_command(state)
        send_device_command(id, command)
        return json.dumps(state)
    else:
        content = "Error: Expecting a state in {on, off, blinking}."
        return render_template("index.html", content=content)


# Required Routes

@app.route("/api/devices")
def list_devices():
    """List the devices currently installed"""

    device_array = list(map(
        lambda d: d.removeprefix(DEVICE_TTY_PATH),
        glob.glob(DEVICE_PATH_PATTERN)))
    device_array.sort()
    return json.dumps(device_array)

@app.route("/api/id/<id>/state/")
@app.route("/api/id/<id>/state/<state>")
def read_write_state(id, state=None):
    """Read or write the state of device <ID>'s LED."""

    if state is None:
        return read_state(id)
    else:
        return write_state(id, state)

@app.route("/api/id/<id>/state/blinking/<hz>")
def write_state_blinking(id, hz):
    """Blink device <ID>'s LED at frequency <HZ>.

    Not implemented."""

    if is_valid_hz(hz):
        content = "device {id} is blinking at {hz} Hz".format(id=id, hz=hz)
        write_state(id, "blinking")
    else:
        content = "Expecting a frequency in 0.1 Hz <= f <= 240 Hz."
    return render_template("index.html", content=content)


# Auxiliary Routes
#
# These routes are not part of the specification, but are helpful.
# Every API should be at least a little discoverable.
# Every API should have a version identifier.
# The devcmd route provides access to the full range of my desk light commands.

@app.route("/")
def index():
    """List the routes of the Device Server"""

    routes = {
        '/': index.__doc__,
        href('/api/devices'): list_devices.__doc__,
        href('/api/states'): list_states.__doc__,
        html.escape('/api/id/<ID>/state'): read_state.__doc__,
        html.escape('/api/id/<ID>/state/<STATE>'): write_state.__doc__,
        html.escape('/api/id/<ID>/state/blinking/<HZ>'): write_state_blinking.__doc__,
        href('/api/version'): version.__doc__,
        html.escape('/devcmd/<ID>/<COMMAND>'): devcmd.__doc__
        }
    return render_template("routes.html", routes=routes)

@app.route("/api/states")
def list_states():
    """List the states for an LED"""

    return json.dumps(LED_STATES)

@app.route("/api/version")
def version():
    """Device Server version"""

    return json.dumps(__version__)

@app.route("/devcmd/<id>/<cmd>")
def devcmd(id, cmd):
    """See https://gitlab.com/pictographer/desklight"""

    result = '<pre>' + send_device_command(id, cmd) + '</pre>'
    return render_template("index.html", content=result)


# The Application

if __name__ == "__main__":
    app.run()
