# LED UI

*A demonstration of Vue.js, Flask, and USB-Serial connected devices*

This is a small full-stack application consisting of three components:
* client - a Vue.js application that enables a user to issue commands to connected microcontrollers
* server - a Python Flask application that provides a RESTful API to the client that handles enumeration and control of microcontrollers over USB-Serial
* device - a preexisting C++ microcontroller application that provides a USB-Serial interface for controlling LEDs and various other peripherials

**NOTE** A demonstration of embedded Rust was provided previously, but I wasn't able to get USB-Serial input working, so the Rust firmware isn't adequate for an end-to-end demonstration.

See the [upper level README.md](../README.md) for instructions for starting and stopping the entire system. 

## Recommended IDE Setup

[VSCode](https://code.visualstudio.com/) + [Volar](https://marketplace.visualstudio.com/items?itemName=Vue.volar) (and disable Vetur).

## Type Support for `.vue` Imports in TS

TypeScript cannot handle type information for `.vue` imports by default, so we replace the `tsc` CLI with `vue-tsc` for type checking. In editors, we need [Volar](https://marketplace.visualstudio.com/items?itemName=Vue.volar) to make the TypeScript language service aware of `.vue` types.

## Customize configuration

See [Vite Configuration Reference](https://vitejs.dev/config/).

## Project Setup

```sh
npm install
```

### Compile and Hot-Reload for Development

```sh
npm run dev
```

### Type-Check, Compile and Minify for Production

```sh
npm run build
```

### Run Unit Tests with [Vitest](https://vitest.dev/)

The default unit test passes, but it's not terribly useful. See the `../script/test` script for practical tests.

```sh
npm run test:unit
```

### Lint with [ESLint](https://eslint.org/)

This doesn't seem to work even after installing [ESLint plugin for Vue.js](https://eslint.vuejs.org/user-guide/). ¯\\\_(ツ)\_/¯

```sh
npm run lint
```
