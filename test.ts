// Test program for the Grove DIP switch extension.
// Button A: print every switch's state and the combined value.
// Button B: print connection status and firmware version.
// Changes to any switch are reported as they happen.

groveSwitch.onError(function (reason) {
    serial.writeLine("Error: " + reason)
})

groveSwitch.onAnySwitchChanged(function (position, isOn) {
    serial.writeLine("Switch " + position + (isOn ? " ON" : " OFF"))
})

groveSwitch.onSwitch(1, GroveSwitchState.On, function () {
    basic.showIcon(IconNames.Yes)
})

groveSwitch.onSwitch(1, GroveSwitchState.Off, function () {
    basic.showIcon(IconNames.No)
})

input.onButtonPressed(Button.A, function () {
    for (let p = 1; p <= 6; p++) {
        serial.writeLine("pos " + p + ": " + (groveSwitch.switchIs(p, GroveSwitchState.On) ? "on" : "off"))
    }
    serial.writeValue("value", groveSwitch.value())
    basic.showNumber(groveSwitch.value())
})

input.onButtonPressed(Button.B, function () {
    serial.writeLine("connected: " + groveSwitch.isConnected())
    serial.writeLine("firmware: " + groveSwitch.firmwareVersion())
})

basic.showIcon(IconNames.Heart)
