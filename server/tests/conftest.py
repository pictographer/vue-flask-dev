import requests
import sys
from tests.test_server import test_version

try:
    test_version()
except Exception as e:
    print(e.__class__)
    print("Use the script instead.")
    sys.exit(1)
