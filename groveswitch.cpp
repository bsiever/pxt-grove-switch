/**
 * Bill Siever
 * 2026 Initial Version (adapted from the SEN66 driver)
 *
 * Driver for the Seeed Grove 6-Position DIP Switch (I2C).
 * See protocol.md for a full description of the I2C protocol. In brief:
 *   - Default 7-bit address 0x03
 *   - Command based: write a 1-byte command, then (for queries) read the
 *     response in a separate transaction (STOP between, no repeated start)
 *   - 0x00 GET_DEV_ID    -> 4 bytes: PID(LE16), VID(LE16); DIP = 0x0003 / 0x2886
 *   - 0x01 GET_DEV_EVENT -> 4-byte event word + 1 status byte per switch
 *                           (bit 0 of each status byte: 0 = ON, 1 = OFF)
 *   - 0x02 EVENT_DET_MODE / 0x03 BLOCK_DET_MODE (no response)
 *   - 0xC0 SET_ADDR <addr> (no response)
 *   - 0xE2 GET_VER       -> 10-byte ASCII version string, e.g. "BN-6--0.1"
 *
 * This code is released under the [MIT License](http://opensource.org/licenses/MIT).
 * Please review the LICENSE.txt file included with this extension. If you have any questions
 * or concerns with licensing, please contact the author.
 * Distributed as-is; no warranty is given.
 */

#include "pxt.h"
#include "MicroBitI2C.h"
#include "MicroBit.h"

using namespace pxt;

namespace groveswitch {

    // ************** Protocol constants (see protocol.md) **************

    const uint8_t CMD_GET_DEV_ID    = 0x00;  // -> 4 bytes: PID lo, PID hi, VID lo, VID hi
    const uint8_t CMD_GET_DEV_EVENT = 0x01;  // -> 4-byte event word + 1 byte per switch
    const uint8_t CMD_EVENT_MODE    = 0x02;  // enable event detection (no response)
    // const uint8_t CMD_BLOCK_MODE = 0x03;  // disable event detection (unused)
    const uint8_t CMD_SET_ADDR      = 0xC0;  // followed by new 7-bit address (no response)
    const uint8_t CMD_GET_VER       = 0xE2;  // -> 10-byte ASCII version string

    const uint16_t VID_SEEED        = 0x2886;
    const uint16_t PID_DIP_SWITCH   = 0x0003;
    const uint16_t PID_5_WAY_SWITCH = 0x0002;

    const int DEFAULT_ADDRESS = 0x03;        // 7-bit address
    const int SWITCH_COUNT    = 6;
    const int EVENT_HDR_LEN   = 4;           // Size of the event word that precedes switch bytes
    const int VERSION_LEN     = 10;          // Size of the version string response
    const int PROBE_TRIES     = 4;           // Matches Seeed's Arduino library

    const uint8_t STATUS_RAW_BIT = 0x01;     // Per-switch status bit 0: 0 = ON, 1 = OFF

    // ************** State **************

    static Action errorHandler = NULL;
    static String _lastError = PSTR("");

    static int  _address = DEFAULT_ADDRESS;  // 7-bit address currently in use
    static bool _ready = false;              // true after a successful probe
    static bool _errorReported = false;      // suppress repeated errors until the next success
    static char _version[VERSION_LEN + 1];   // Firmware version string ("" if unknown)

    // ************** Private helpers **************

    /*
     * Record an error and call the registered handler (if any).
     * Errors are only reported once until the device is successfully read again,
     * so that a disconnected switch doesn't flood the handler while polling.
     */
    static void groveswitch_error(const char *error = "") {
        if (_errorReported) {
            return;
        }
        _errorReported = true;
        _lastError = PSTR(error);
        if (errorHandler) {
            pxt::runAction0(errorHandler);
        }
    }

    // micro:bit I2C APIs take 8-bit (shifted) addresses
    static inline int busAddress() {
        return _address << 1;
    }

    // Write bytes to the device in a single transaction (ends with STOP).
    static bool writeBytes(uint8_t *data, int length) {
        int status;
#if MICROBIT_CODAL
        status = uBit.i2c.write(busAddress(), data, length, false);
#else
        status = uBit.i2c.write(busAddress(), (char *)data, length, false);
#endif
        return status == MICROBIT_OK;
    }

    // Read bytes from the device in a single transaction.
    static bool readBytes(uint8_t *data, int length) {
        int status;
#if MICROBIT_CODAL
        status = uBit.i2c.read(busAddress(), data, length, false);
#else
        status = uBit.i2c.read(busAddress(), (char *)data, length, false);
#endif
        return status == MICROBIT_OK;
    }

    // Send a single-byte command that has no response (e.g. 0x02 event mode).
    static bool sendCommand(uint8_t command) {
        return writeBytes(&command, 1);
    }

    // Send a single-byte command, then read its response.
    // As in Seeed's library, the write and read are separate transactions (STOP between).
    static bool query(uint8_t command, uint8_t *response, int length) {
        if (!sendCommand(command)) {
            return false;
        }
        return readBytes(response, length);
    }

    /*
     * Probe for the device and set it up (protocol.md §3):
     *   1. GET_DEV_ID (with retries and a 1-byte dummy read to realign the
     *      firmware's transmit buffer, which can be off by one on firmware 0.1)
     *   2. GET_VER to record the firmware version string
     *   3. EVENT_DET_MODE (raw switch positions are reported in either mode,
     *      but this matches Seeed's reference example)
     *
     * Returns true if a DIP switch was found. When `report` is false no
     * errors are sent to the error handler (used by isConnected()).
     */
    static bool probe(bool report) {
        _ready = false;
        _version[0] = 0;

        bool found = false;
        uint16_t pid = 0;
        for (int tries = PROBE_TRIES; tries > 0 && !found; tries--) {
            uint8_t id[4] = {0, 0, 0, 0};
            if (query(CMD_GET_DEV_ID, id, sizeof(id))) {
                uint16_t vid = id[2] | (id[3] << 8);
                pid = id[0] | (id[1] << 8);
                if (vid == VID_SEEED) {
                    found = true;
                    break;
                }
            }
            // Realign the device's transmit buffer and retry
            uint8_t dummy;
            readBytes(&dummy, 1);
        }

        if (!found) {
            if (report) groveswitch_error("Grove DIP switch not found");
            return false;
        }
        if (pid != PID_DIP_SWITCH) {
            if (report) {
                groveswitch_error(pid == PID_5_WAY_SWITCH
                                  ? "Found a Grove 5-Way Switch, not a DIP switch"
                                  : "Unknown Grove switch model");
            }
            return false;
        }

        // Firmware version (best effort; not required for operation)
        uint8_t ver[VERSION_LEN];
        if (query(CMD_GET_VER, ver, sizeof(ver))) {
            memcpy(_version, ver, VERSION_LEN);
            _version[VERSION_LEN] = 0;
        }

        if (!sendCommand(CMD_EVENT_MODE)) {
            if (report) groveswitch_error("Grove DIP switch setup failed");
            return false;
        }

        _ready = true;
        return true;
    }

    // Probe if not already set up. Reports errors.
    static bool ensureReady() {
        return _ready || probe(true);
    }

    //************************ MakeCode Blocks ************************

    //%
    void setErrorHandler(Action a) {
        // Release any prior error handler
        if (errorHandler)
            pxt::decr(errorHandler);
        errorHandler = a;
        if (errorHandler)
            pxt::incr(errorHandler);
    }

    //%
    String lastError() {
        return _lastError;
    }

    /*
     * Read all six switch positions (GET_DEV_EVENT, protocol.md §4).
     * Returns a bit mask where bit 0 is position 1 ... bit 5 is position 6,
     * and a 1 bit means the switch is ON. Returns -1 on error.
     * On a failed read the device will be re-probed on the next call, so
     * unplugging and re-plugging the switch is handled automatically.
     */
    //%
    int readSwitches() {
        if (!ensureReady()) {
            return -1;
        }

        uint8_t data[EVENT_HDR_LEN + SWITCH_COUNT];
        if (!query(CMD_GET_DEV_EVENT, data, sizeof(data))) {
            _ready = false;
            groveswitch_error("Grove DIP switch read failed");
            return -1;
        }

        int mask = 0;
        for (int i = 0; i < SWITCH_COUNT; i++) {
            // Status bit 0 is active-low: 0 = ON
            if ((data[EVENT_HDR_LEN + i] & STATUS_RAW_BIT) == 0) {
                mask |= 1 << i;
            }
        }
        _errorReported = false;  // Successful read: allow future errors to be reported
        return mask;
    }

    /*
     * Re-probe the device. Returns true if a DIP switch responds.
     * Does not call the error handler.
     */
    //%
    bool isConnected() {
        bool ok = probe(false);
        if (ok) {
            _errorReported = false;
        }
        return ok;
    }

    /*
     * Firmware version string reported by the device (e.g. "BN-6--0.1").
     * Returns an empty string on error.
     */
    //%
    String firmwareVersion() {
        if (!ensureReady()) {
            return PSTR("");
        }
        return PSTR(_version);
    }

    /*
     * Talk to a switch at a different 7-bit I2C address (does not change the device).
     */
    //%
    void useAddress(int address) {
        if (address < 0x03 || address > 0x77) {
            groveswitch_error("Invalid I2C address");
            return;
        }
        _address = address;
        _ready = false;
    }

    /*
     * Change the device's I2C address (SET_ADDR, 0xC0) and use the new address.
     */
    //%
    void changeAddress(int address) {
        if (address < 0x03 || address > 0x77) {
            groveswitch_error("Invalid I2C address");
            return;
        }
        if (!ensureReady()) {
            return;
        }
        uint8_t data[2] = {CMD_SET_ADDR, (uint8_t)address};
        if (!writeBytes(data, sizeof(data))) {
            groveswitch_error("Change address failed");
            return;
        }
        _address = address;
        _ready = false;
        uBit.sleep(50);  // Give the device time to switch addresses
    }
}
