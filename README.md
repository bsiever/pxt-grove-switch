
```package
grove-switch=github:bsiever/pxt-grove-switch
```

# Grove DIP Switch

This extension supports the [Seeed Grove 6-Position DIP Switch](https://wiki.seeedstudio.com/Grove-6-Position_DIP_Switch/), an I2C module with six on/off switches. Use it to:

* Check whether each switch is on or off
* Read all six switches as a single binary number (0–63)
* Run code when a switch is flipped

The I2C protocol used by the switch is documented in [protocol.md](protocol.md).

# Parts & Wiring

## Parts

1. A Grove 6-Position DIP Switch (see the [Seeed wiki page](https://wiki.seeedstudio.com/Grove-6-Position_DIP_Switch/))
2. A Grove shield for micro:bit (such as Seeed's Grove Shield for micro:bit) and a Grove cable

## Wiring

Plug the switch into one of the shield's **I2C** Grove ports. No other wiring is needed.

If you are not using a shield, connect the Grove cable to the micro:bit as follows:

| Grove wire | Signal | micro:bit |
|---|---|---|
| Black  | GND | GND |
| Red    | VCC | 3V |
| White  | SDA | P20 (SDA) |
| Yellow | SCL | P19 (SCL) |

The switch's default I2C address is **0x03**.

# Switch is on/off

```sig
groveSwitch.switchIs(position: number, state: GroveSwitchState): boolean
```

Returns `true` if switch `position` (1–6) is in the given state (`on` or `off`). Returns `false` if the switch can't be read.

# Switches as number

```sig
groveSwitch.value(): number
```

Returns all six switches as one number from 0 to 63. Position 1 is the 1s bit, position 2 the 2s bit, position 3 the 4s bit, and so on up to position 6, the 32s bit. A switch that is on contributes its bit. Returns `-1` on error.

This is useful for teaching binary numbers: turn on positions 1 and 3 and the value is 1 + 4 = 5.

# On switch turned on/off

```sig
groveSwitch.onSwitch(position: number, state: GroveSwitchState, handler: () => void): void
```

Runs `handler` when switch `position` (1–6) is turned to the given state.

# On any switch changed

```sig
groveSwitch.onAnySwitchChanged(handler: (position: number, isOn: boolean) => void): void
```

Runs `handler` whenever any switch changes. `position` is the switch that changed (1–6), and `isOn` is its new state.

### ~hint

Event blocks work by checking the switch about 20 times per second in the background. If a switch is flipped and flipped back faster than that, the change may be missed.

### ~

# On Error

```sig
groveSwitch.onError(errCallback: (reason: string) => void): void
```

Responds to errors, for example when the switch is not connected. `reason` describes the error. Each problem is reported once, until the switch is read successfully again. If the switch is unplugged and plugged back in, the extension reconnects automatically.

# Connected (advanced)

```sig
groveSwitch.isConnected(): boolean
```

Returns `true` if a Grove DIP switch is connected and responding.

# Firmware Version (advanced)

```sig
groveSwitch.firmwareVersion(): string
```

Returns the switch's firmware version string (for example `BN-6--0.1`). Returns an empty string on error.

# Use I2C Address (advanced)

```sig
groveSwitch.useAddress(address: number): void
```

Talks to a switch at a different I2C address. This does not change the switch itself. Use it for a switch whose address was previously changed.

# Change I2C Address (advanced)

```sig
groveSwitch.changeAddress(address: number): void
```

Changes the switch's own I2C address, then uses the new address. Write down the new address, because the switch may keep it after it is unplugged. Use `use DIP switch at I2C address` with the new address in later programs.

# Example

The following program shows the switches' value on the LEDs, and logs each change to the serial console.

```blocks
groveSwitch.onError(function (reason) {
    serial.writeLine(reason)
})
groveSwitch.onAnySwitchChanged(function (position, isOn) {
    serial.writeLine("switch " + position + " " + (isOn ? "on" : "off"))
})
basic.forever(function () {
    basic.showNumber(groveSwitch.value())
})
```

# Acknowledgements

The I2C protocol is based on Seeed Studio's [Grove_Multi_Switch](https://github.com/Seeed-Studio/Grove_Multi_Switch) Arduino library (MIT License).

# Miscellaneous

I develop micro:bit extensions in my spare time to support activities I'm enthusiastic about, like summer camps and science curricula.  You are welcome to become a sponsor of my micro:bit work (one time or recurring payments), which helps offset equipment costs: [https://github.com/sponsors/bsiever](https://github.com/sponsors/bsiever). Any support at all is greatly appreciated!

## Supported targets

for PXT/microbit

<script src="https://makecode.com/gh-pages-embed.js"></script>
<script>makeCodeRender("{{ site.makecode.home_url }}", "{{ site.github.owner_name }}/{{ site.github.repository_name }}");</script>
