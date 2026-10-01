# Grove 6-Position DIP Switch — I2C Protocol

This document describes the I2C protocol used by the Seeed **Grove 6-Position DIP Switch**, as implemented by the Seeed Arduino library in `Grove_Multi_Switch/` (`src/Grove_Multi_Switch.cpp`, `src/Grove_Multi_Switch.h`). The same firmware and protocol also serve the Grove 5-Way Tactile Switch; differences are noted where they matter.

There is no official datasheet for this protocol. Everything here is derived from the library source, its example sketch, and the sample output in its README. Items marked **(unverified)** are command codes defined in the library but never exercised by it, so their payloads and responses are not confirmed.

---

## 1. Bus basics

| Item | Value |
|---|---|
| Default 7-bit address | `0x03` |
| Vendor ID (VID) | `0x2886` (Seeed) |
| Product ID (PID), DIP switch | `0x0003` |
| Product ID (PID), 5-way tactile | `0x0002` |
| Switch count | 6 (DIP), 5 (tactile) |
| Multi-byte integer byte order | Little-endian |

**Address note:** `0x03` falls inside the I2C reserved range (`0x00`–`0x07`). Some I2C masters or bus scanners skip or reject these addresses, so confirm that your platform allows it.

### 1.1 Transaction pattern

The device is **command-based**: the host writes a single command byte, and for commands that return data, it then reads the response.

The library does this as **two separate transactions with a STOP between them**, not with a repeated start:

```
[S] addr+W  CMD  [P]                 ← write command (endTransmission() sends STOP)
[S] addr+R  D0 D1 ... Dn-1  [P]      ← read n response bytes
```

Commands that take an argument send it in the same write transaction:

```
[S] addr+W  CMD  ARG  [P]
```

The library tolerates the device returning more bytes than requested and discards the extras.

---

## 2. Command summary

| Code | Name | Direction | Write payload | Response | Used by library |
|---|---|---|---|---|---|
| `0x00` | `GET_DEV_ID` | write, then read | — | 4 bytes: PID, VID | Yes (`probeDevID`) |
| `0x01` | `GET_DEV_EVENT` | write, then read | — | 4 + N bytes (N = 6 for DIP) | Yes (`getEvent`) |
| `0x02` | `EVENT_DET_MODE` | write only | — | none | Yes (`setEventMode(true)`) |
| `0x03` | `BLOCK_DET_MODE` | write only | — | none | Yes (`setEventMode(false)`) |
| `0xB2` | `AUTO_SLEEP_ON` | write only | — | none | No **(unverified)** |
| `0xB3` | `AUTO_SLEEP_OFF` | write only | — | none | No **(unverified)**; the source comment says this is the default |
| `0xC0` | `SET_ADDR` | write only | 1 byte: new 7-bit address | none | Yes (`setDevAddr`) |
| `0xC1` | `RST_ADDR` | write only | — | none | No **(unverified)** |
| `0xE0` | `TEST_TX_RX_ON` | write only | — | none | No **(unverified)**; factory test |
| `0xE1` | `TEST_TX_RX_OFF` | write only | — | none | No **(unverified)**; factory test |
| `0xE2` | `TEST_GET_VER` | write, then read | — | 10 bytes: ASCII version string | Yes (`getDevVer`) |
| `0xF1` | `GET_DEVICE_UID` | write, then read | — | unknown length | No **(unverified)** |

---

## 3. Setup sequence

The library's `begin()` plus the example's `deviceDetect()` perform this sequence:

1. **Probe the device ID** (`0x00`). Retry with buffer realignment if needed (§3.1).
2. **Read the firmware version** (`0xE2`) and keep it, because it decides whether a workaround is needed (§4.3).
3. **Get the switch count** from the PID. No I2C traffic is needed: PID `0x0003` means 6 switches.
4. **Enable event detection** (`0x02`). This is optional; see §3.3.

### 3.1 `0x00` GET_DEV_ID — probe

```
Write: 00
Read:  4 bytes
```

| Byte | Meaning |
|---|---|
| 0 | PID low byte |
| 1 | PID high byte |
| 2 | VID low byte |
| 3 | VID high byte |

The bytes are read into a little-endian `uint32_t`, so `VID = id >> 16` and `PID = id & 0xFFFF`.

Expected bytes for the DIP switch: **`03 00 86 28`**, which gives `id = 0x28860003`.

**Probe retry and realignment.** The original firmware (the "i2c data buffer shift error" fixed in commit `16a1313`) can return the response shifted by one byte. The library handles this as follows:

```
repeat up to 4 times:
    write 00, read 4 bytes → id
    if (id >> 16) == 0x2886: success
    else: read 1 extra byte and discard it   ← realigns the device's TX buffer
```

The README's sample output shows two failed probes before success on a DIP switch, so expect to need this retry in practice.

### 3.2 `0xE2` TEST_GET_VER — firmware version

```
Write: E2
Read:  10 bytes (ASCII, NUL-terminated)
```

Example strings from the README:

| Device | String | Bytes 0–8 |
|---|---|---|
| 6-pos DIP | `BN-6--0.1` | `B N - 6 - - 0 . 1` + `\0` |
| 5-way tactile | `BN-5E-0.1` | `B N - 5 E - 0 . 1` + `\0` |

The library derives a numeric version from characters 6 and 8:

```
version = (byte[6] - '0') * 10 + (byte[8] - '0')     // "0.1" → 1
```

A `version` of `1` (firmware 0.1) turns on the level-change workaround in §4.3.

### 3.3 `0x02` / `0x03` — detection mode

```
Enable event detection:   write 02
Disable (block mode):     write 03
```

Each is a single-byte write with no response.

- **Event mode (`0x02`):** the firmware reports events. For DIP switches the only event is *level changed* (bit 4, §4.2). On the tactile switch it also reports click, double-click, and long-press.
- **Block mode (`0x03`):** event detection is off.

**Raw switch position (bit 0) is reported in both modes.** The library never sets a mode in `begin()`, and the power-on default mode is not documented. The example sketch always sends `0x02` after probing, so it is safest to set the mode explicitly.

### 3.4 `0xC0` SET_ADDR — change I2C address

```
Write: C0 <new_addr>
```

`new_addr` is the new 7-bit address. Notes:

- The library does **not** update its own stored address after this command, so the host must talk to `new_addr` from then on.
- The source does not say whether the new address survives a power cycle.
- `0xC1` (RST_ADDR) presumably restores `0x03`. It is **(unverified)**: probably a bare one-byte write.

---

## 4. Data retrieval

### 4.1 `0x01` GET_DEV_EVENT

```
Write: 01
Read:  4 + N bytes      (N = 6 for the DIP switch → 10 bytes total)
```

| Bytes | Field | Meaning |
|---|---|---|
| 0–3 | `event` (uint32, little-endian) | Bit 31 (`0x80` in byte 3) = `HAS_EVENT`: at least one switch has an event. Other bits are not used by the library. |
| 4 | `button[0]` | Switch position 1 |
| 5 | `button[1]` | Switch position 2 |
| 6 | `button[2]` | Switch position 3 |
| 7 | `button[3]` | Switch position 4 |
| 8 | `button[4]` | Switch position 5 |
| 9 | `button[5]` | Switch position 6 |

The library sizes the read as `4 + switchCount`. For the tactile switch that is 9 bytes.

### 4.2 Per-switch status byte

| Bit | Mask | Name | DIP switch meaning |
|---|---|---|---|
| 0 | `0x01` | `RAW_STATUS` | **0 = ON**, 1 = OFF (active-low) |
| 1 | `0x02` | `SINGLE_CLICK` | Tactile only |
| 2 | `0x04` | `DOUBLE_CLICK` | Tactile only |
| 3 | `0x08` | `LONG_PRESS` | Tactile only |
| 4 | `0x10` | `LEVEL_CHANGED` | Position changed since the last report (event mode) |
| 5–7 | — | — | Unused |

The active-low sense comes from the header (`#define RAW_DIP_SWITCH_ON 0`) and the README output (`POS 1: RAW - LOW ON`).

To build a 6-bit "switches ON" value from a response `r[]`:

```
on_mask = 0
for i in 0..5:
    if (r[4 + i] & 0x01) == 0: on_mask |= (1 << i)
```

### 4.3 Firmware 0.1 level-change workaround

Firmware v0.1 can **drop `LEVEL_CHANGED` events if the host polls frequently**. This suggests that events clear when read, but that is an inference from the comment, not stated in the source. The library handles it as follows:

- If `version > 1`, the device response is trusted as-is.
- If `version <= 1`, the library ignores the device's bit 4 and creates it on the host side:

```
for each switch i:
    button[i] &= ~0x10
    if (button[i] ^ previous[i]) & 0x01:      // raw position changed
        button[i] |= 0x10
        event     |= 0x80000000               // set HAS_EVENT
previous = current
```

On the first call, `previous` starts out equal to the current reading, so no event is reported. A host that only needs switch positions can ignore events entirely, read bit 0, and compare readings itself. That works the same on every firmware version.

### 4.4 Polling

The example sketch polls `GET_DEV_EVENT` about every 1 ms. A failed read means the device was removed, and the sketch then repeats the full setup sequence (§3) about once per second. That is how hot-plugging is supported.

---

## 5. Complete example transaction log (DIP switch)

```
# --- setup ---
W 03: 00                  ; GET_DEV_ID
R 03: 03 00 86 28         ; PID=0x0003, VID=0x2886  (retry + 1-byte dummy read if VID wrong)
W 03: E2                  ; GET_VER
R 03: 42 4E 2D 36 2D 2D 30 2E 31 00   ; "BN-6--0.1"
W 03: 02                  ; EVENT_DET_MODE

# --- poll ---
W 03: 01                  ; GET_DEV_EVENT
R 03: 00 00 00 00 00 00 00 00 00 01   ; no event; pos 1–5 ON, pos 6 OFF
...
W 03: 01
R 03: 00 00 00 80 00 00 00 00 11 01   ; HAS_EVENT; pos 5 turned OFF (RAW=1 + LEVEL_CHANGED)
```

In the second poll, the event word bytes `00 00 00 80` are little-endian, so bit 31 is set. These response bytes are illustrative and were built from the field definitions above, not captured from a device.

---

## 6. MakeCode (micro:bit) sketch

```ts
const ADDR = 0x03

function cmd(c: number) {
    pins.i2cWriteNumber(ADDR, c, NumberFormat.UInt8LE, false)   // STOP after write
}

function probe(): boolean {
    for (let t = 0; t < 4; t++) {
        cmd(0x00)
        const id = pins.i2cReadBuffer(ADDR, 4)
        if (id[2] == 0x86 && id[3] == 0x28) return id[0] == 0x03   // PID 0x0003 = DIP
        pins.i2cReadBuffer(ADDR, 1)                                 // realign
    }
    return false
}

function readSwitches(): number {        // bit i set = position i+1 ON
    cmd(0x01)
    const r = pins.i2cReadBuffer(ADDR, 10)
    let m = 0
    for (let i = 0; i < 6; i++) if (!(r[4 + i] & 0x01)) m |= 1 << i
    return m
}
```

---

## 7. Open questions

These need confirmation on real hardware:

- The power-on default detection mode (event or block).
- Whether `LEVEL_CHANGED` and `HAS_EVENT` clear when `0x01` is read on firmware 0.2 and later.
- The meaning of event-word bits 0–30.
- Whether `SET_ADDR` persists across power cycles, and the exact behavior of `RST_ADDR`.
- The payloads and responses of `0xB2`/`0xB3` (auto-sleep) and `0xF1` (UID).
