// Auto-generated. Do not edit.


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

declare namespace groveswitch {
}

// Auto-generated. Do not edit. Really.
