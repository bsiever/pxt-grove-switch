
/**
 * Switch states used by the Grove DIP switch blocks.
 */
enum GroveSwitchState {
    //% block="on"
    On = 1,
    //% block="off"
    Off = 0
}

/**
 * Support for the Seeed Grove 6-Position DIP Switch (I2C).
 * Plug the switch into an I2C Grove port (default address 0x03).
 */
//% color=#3C8D2F
//% icon=""
//% block="DIP Switch"
//% groups='["Switches", "Events", "Errors"]'
namespace groveSwitch {

    // Number of positions on the switch
    const SWITCH_COUNT = 6;
    // How often (ms) the switch is polled when event handlers are registered
    const POLL_INTERVAL = 50;

    class SwitchHandler {
        constructor(public position: number, public on: boolean, public handler: () => void) { }
    }

    //% whenUsed
    let errorHandler: Action = null;
    //% whenUsed
    let _lastError: string = null;
    //% whenUsed
    let _handlers: SwitchHandler[] = [];
    //% whenUsed
    let _anyHandler: (position: number, isOn: boolean) => void = null;
    //% whenUsed
    let _polling = false;
    //% whenUsed
    let _simSwitches = 0;   // Simulator: all switches off

    // ************** Private helpers (implemented in groveswitch.cpp) **************

    //% shim=groveswitch::setErrorHandler
    function setErrorHandler(a: Action) {
        errorHandler = a;
    }

    //% shim=groveswitch::lastError
    function lastError(): string {
        return _lastError;
    }

    //% shim=groveswitch::readSwitches
    function readSwitches(): number {
        return _simSwitches;
    }

    // Validate a 1-based position. Returns the bit mask for it, or 0 if invalid.
    function positionMask(position: number): number {
        position = Math.round(position);
        if (position < 1 || position > SWITCH_COUNT) return 0;
        return 1 << (position - 1);
    }

    /*
     * Start the background poller (once). It reads the switch every POLL_INTERVAL ms
     * and calls handlers for any switch whose position changed. Polling also acts as
     * a simple debounce for the mechanical switches.
     */
    function startPolling() {
        if (_polling) return;
        _polling = true;
        control.inBackground(function () {
            let last = -1;
            while (true) {
                const now = readSwitches();
                if (now >= 0) {
                    if (last >= 0 && now != last) {
                        const changed = now ^ last;
                        for (let i = 0; i < SWITCH_COUNT; i++) {
                            if (changed & (1 << i)) {
                                const isOn = (now & (1 << i)) != 0;
                                for (let j = 0; j < _handlers.length; j++) {
                                    const h = _handlers[j];
                                    if (h.position == i + 1 && h.on == isOn) h.handler();
                                }
                                if (_anyHandler) _anyHandler(i + 1, isOn);
                            }
                        }
                    }
                    last = now;
                }
                basic.pause(POLL_INTERVAL);
            }
        });
    }

    // ************** Switches **************

    /**
     * Check whether a switch position is on (or off).
     * Returns false if the switch can't be read.
     * @param position the switch position, 1 to 6
     * @param state the state to check for
     */
    //% block="switch $position is $state"
    //% position.min=1 position.max=6 position.defl=1
    //% state.defl=GroveSwitchState.On
    //% group="Switches"
    //% weight=1000
    export function switchIs(position: number, state: GroveSwitchState): boolean {
        const mask = positionMask(position);
        const value = readSwitches();
        if (mask == 0 || value < 0) return false;
        const isOn = (value & mask) != 0;
        return state == GroveSwitchState.On ? isOn : !isOn;
    }

    /**
     * Get all six switches as a single number (0 to 63).
     * Position 1 is the 1s bit, position 2 the 2s bit, ... position 6 the 32s bit.
     * A switch that is on contributes its bit. Returns -1 on error.
     */
    //% block="switches as number"
    //% group="Switches"
    //% weight=900
    export function value(): number {
        return readSwitches();
    }

    // ************** Events **************

    /**
     * Run code when a switch is turned on or off.
     * @param position the switch position, 1 to 6
     * @param state the new state that triggers the handler
     * @param handler the code to run
     */
    //% block="on switch $position turned $state"
    //% position.min=1 position.max=6 position.defl=1
    //% state.defl=GroveSwitchState.On
    //% group="Events"
    //% weight=800
    export function onSwitch(position: number, state: GroveSwitchState, handler: () => void) {
        if (!handler || positionMask(position) == 0) return;
        _handlers.push(new SwitchHandler(Math.round(position), state == GroveSwitchState.On, handler));
        startPolling();
    }

    /**
     * Run code when any switch changes.
     * @param handler code to run; `position` is the switch that changed (1 to 6) and `isOn` its new state
     */
    //% block="on any switch changed"
    //% draggableParameters="reporter"
    //% group="Events"
    //% weight=700
    export function onAnySwitchChanged(handler: (position: number, isOn: boolean) => void) {
        _anyHandler = handler;
        startPolling();
    }

    // ************** Errors **************

    /**
     * Set a handler for errors (for example, the switch is not connected).
     * Each problem is reported once, until the switch is read successfully again.
     * @param errCallback The error handler
     */
    //% block="DIP switch error"
    //% draggableParameters="reporter"
    //% group="Errors"
    //% weight=100
    export function onError(errCallback: (reason: string) => void) {
        if (errCallback) {
            errorHandler = () => {
                let le = lastError();
                errCallback(le);
            };
        } else {
            errorHandler = null;
        }
        setErrorHandler(errorHandler);
    }

    // ************** Advanced **************

    /**
     * Check whether a Grove DIP switch is connected and responding.
     */
    //% block="DIP switch connected" advanced=true
    //% shim=groveswitch::isConnected
    //% weight=900
    export function isConnected(): boolean {
        return true;
    }

    /**
     * Get the switch's firmware version string (for example "BN-6--0.1").
     * Returns an empty string on error.
     */
    //% block="firmware version" advanced=true
    //% shim=groveswitch::firmwareVersion
    //% weight=800
    export function firmwareVersion(): string {
        return "BN-6--0.1 (SIM)";
    }

    /**
     * Talk to a switch at a different I2C address. This does not change the switch itself.
     * Use this if a switch's address was previously changed.
     * @param address the 7-bit I2C address, eg: 3
     */
    //% block="use DIP switch at I2C address $address" advanced=true
    //% address.min=3 address.max=119 address.defl=3
    //% shim=groveswitch::useAddress
    //% weight=700
    export function useAddress(address: number) {
        0;
    }

    /**
     * Permanently change the switch's I2C address, then use the new address.
     * Lets more than one switch share the bus. Note the new address so you can
     * use it later; it may persist after power is removed.
     * @param address the new 7-bit I2C address, eg: 4
     */
    //% block="change DIP switch I2C address to $address" advanced=true
    //% address.min=3 address.max=119 address.defl=4
    //% shim=groveswitch::changeAddress
    //% weight=600
    export function changeAddress(address: number) {
        0;
    }
}
