# Test the LED UI client.
#
# Since the client needs information from the server to render the
# initial client page, use Selenium and Chrome to fetch the html
# for the page, and then look for signs of success.
#
# The options to control an LED can only be generated
# if all the following
#    * The client can fetch the list of devices from the server
#    * The server can enumerate the devices and respond to an API request
#    * The client cna fetch the list of LED states from the server
#
# This does not test that the client can successfully update the state
# of a device. Without external hardware, the best we could hope to do
# would be to test that the command to set the state of an LED was sent.
#
# The test involves looking for at least two menu options:
#    * On
#    * Off
#
# This could go wrong if there were several devices but the server
# returned an empty or otherwise incorrect list of LED states. Testing
# the server API directly can easily cover this.
#
# The code below is very similar to code found on Stack Overflow.
# https://stackoverflow.com/questions/74742348/how-can-i-get-html-of-a-website-as-seen-on-browser

from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from sys import exit


def test_option_menu():
    # Client URL
    url = "http://localhost:5173/"

    # Adding the option for headless browser
    options = webdriver.ChromeOptions()
    options.add_argument("headless")
    driver = webdriver.Chrome(options=options)

    # Create a new instance of the Chrome webdriver
    driver = webdriver.Chrome()

    driver.get(url)

    # Get  HTML 
    html = driver.page_source
    driver.close()

    assert html.count('<option ') > 1



