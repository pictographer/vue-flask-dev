import json
import requests

# Test that the server responds

def fetch_json(url):
    """Fetch a JSON expression from a URL"""

    response = requests.get(url)
    if response.ok:
        return json.loads(response.text)
    else:
        print("fetch_json::ERROR {}".format(response.status_code))
        print(response.text)
        return None

def test_version():
    """Test that the version can be fetched"""

    version = fetch_json('http://localhost:5000/api/version')
    assert version

def test_get_devices():
    """Test that at least one device is available"""

    devices = fetch_json('http://localhost:5000/api/devices')
    assert devices

def test_get_states():
    """Test that at least two states are fetched"""

    states = fetch_json('http://localhost:5000/api/states')
    assert len(states) > 1

def test_get_state_by_id():
    """Test that the state of the first device can be fetched"""

    devices = fetch_json('http://localhost:5000/api/devices')
    assert devices
    states = fetch_json('http://localhost:5000/api/states')
    assert len(states) > 1
    url = 'http://localhost:5000/api/id/' + devices[0] + '/state'
    state = fetch_json(url)
    assert state in states

def test_set_state_by_id():
    """Test that the state of the first device can be updated"""

    # The first few assertions are redundant, but for such simple
    # tests, let's not bother with shared setup.
    devices = fetch_json('http://localhost:5000/api/devices')
    assert devices
    states = fetch_json('http://localhost:5000/api/states')
    assert len(states) > 1
    url0 = 'http://localhost:5000/api/id/' + devices[0] + '/state'
    state0 = fetch_json(url0)
    assert state0 in states

    # Should be okay to set the state to the current state.
    url0 = 'http://localhost:5000/api/id/' + devices[0] + '/state/' + state0
    assert state0 == fetch_json(url0)

    # Get a new state and change to it.
    state1 = states[1] if state0 == states[0] else states[0]
    url1 = 'http://localhost:5000/api/id/' + devices[0] + '/state/' + state1
    assert state1 == fetch_json(url1)
