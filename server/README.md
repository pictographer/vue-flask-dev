# Server

This directory contains a simple flask application with a single API endpoint that returns a list of devices. The relevant code is located in `app.py`. The end goal of the server is to create an endpoint that allows the user to proxy a command via serial connection from the client to the device.

## Routes

The routes which are essential to the demonstration are listed below. Several additional routes are provided for convenience. The homepage of the server lists the routes with their documentation.

```
/api/devices
/api/<id>/state
/api/<id>/state/<state>
/api/<id>/state/blinking/<Hz>

state: { on, off, blinking }
0.1 <= Hz <= 240
```

## Tests

Unit tests are located in the `tests/` directory, and can be run with the `pytest` command.

See `../script/test` for end-to-end testing.