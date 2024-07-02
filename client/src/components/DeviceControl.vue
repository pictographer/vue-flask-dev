// Presents a drop-down menu for each device found. The menu items 
// control the state of the LED of the respective devices.
<script setup lang="ts">
import { ref, watchEffect } from 'vue'
import Item from './Item.vue'
import LedIcon from './icons/IconLed.vue'

const DEVICES_URL = `http://127.0.0.1:5000/api/devices`
const STATES_URL = `http://127.0.0.1:5000/api/states`
const STATE_URL = 'http://127.0.0.1:5000/api/id/{device}/state/{state}'

const devices = ref([])
const selected = ref([])
const states = ref([])
const err = ref('')

// Fetch the devices and the supported LED states from the device server.
// BUG Code doesn't ask if device supports this protocol
// BUG Code doesn't interrogate the device initial state to set the selection.
watchEffect(async () => {
  // Get the devices currently connected according to the device server.
  let response = new Response()
  try {
    response = await fetch(DEVICES_URL)
  } catch (e) {
    console.log(e)
    err.value = 'Unable to retrieve ' + DEVICES_URL
  }
  if (!err.value.length) {
    try {
      devices.value = await (response).json()
      if (!devices.value.length) {
        err.value = 'No connected devices found.'
      }
    } catch (e) {
      console.log(e)
      err.value = 'Unable to parse device list.'
    }
  }
  if (!err.value.length) {
    // Get the defined states from the device server to make it easier to
    // define new states.
    try {
      states.value = await (await fetch(STATES_URL)).json()
    } catch (e) {
      console.log(e)
    }
  }
})

// Relay a message through the device server to the device.
async function commandDevice(device: string, state: string) {
  let state_lc = state.toLowerCase()
  const url = STATE_URL.replace('{device}', device).replace('{state}', state_lc)
  let result = await fetch(url)
}

// Capitalize in TypeScript because the option tag does not honor css text-transform.
function capitalize(s: string) {
  return s.charAt(0).toUpperCase() + s.slice(1)
}
</script>

<template>
  <Item>
    <template #icon>
      <LedIcon />
    </template>
    <template #heading>Devices</template>

    <h3 v-if="!err.length">USB-Serial ACM Devices</h3>
    <h3 v-if="err.length" class="red">{{ err }}</h3>
    <ul v-if="devices.length">
      <li v-for="device in devices" :key="device">
        {{ device }}
        <select v-model="selected[device]" @change="commandDevice(device, selected[device])">
          <option v-for="state of states.map(capitalize)" :key="state" :value="state">
            {{ state }}
          </option>
        </select>
      </li>
    </ul>
  </Item>
</template>
