#!/usr/bin/env python3
"""R36 STORE independent gamepad-to-keyboard mapper for PortMaster .gptk profiles."""
import fcntl
import os
import re
import select
import signal
import struct
import sys
import time

from evdev import InputDevice, ecodes, list_devices

RUNNING = True
THRESHOLD = 700
UI_SET_EVBIT = 0x40045564
UI_SET_KEYBIT = 0x40045565
UI_DEV_CREATE = 0x5501
UI_DEV_DESTROY = 0x5502

KEY_NAMES = {
    "esc": "KEY_ESC", "escape": "KEY_ESC",
    "enter": "KEY_ENTER", "return": "KEY_ENTER",
    "backspace": "KEY_BACKSPACE", "space": "KEY_SPACE",
    "tab": "KEY_TAB", "shift": "KEY_LEFTSHIFT",
    "ctrl": "KEY_LEFTCTRL", "add_ctrl": "KEY_LEFTCTRL",
    "alt": "KEY_LEFTALT", "add_alt": "KEY_LEFTALT",
    "up": "KEY_UP", "down": "KEY_DOWN",
    "left": "KEY_LEFT", "right": "KEY_RIGHT",
    "home": "KEY_HOME", "end": "KEY_END",
    "pageup": "KEY_PAGEUP", "pagedown": "KEY_PAGEDOWN",
    "insert": "KEY_INSERT", "delete": "KEY_DELETE",
    "backtab": "KEY_BACKTAB", "minus": "KEY_MINUS",
    "plus": "KEY_KPPLUS", "equal": "KEY_EQUAL",
    "=": "KEY_EQUAL", "-": "KEY_MINUS", ".": "KEY_DOT",
    ",": "KEY_COMMA", "/": "KEY_SLASH", "'": "KEY_APOSTROPHE",
    ";": "KEY_SEMICOLON", "[": "KEY_LEFTBRACE",
    "]": "KEY_RIGHTBRACE", "\\": "KEY_BACKSLASH",
}
for letter in "abcdefghijklmnopqrstuvwxyz":
    KEY_NAMES[letter] = "KEY_" + letter.upper()
for digit in "0123456789":
    KEY_NAMES[digit] = "KEY_" + digit
for n in range(1, 13):
    KEY_NAMES["f" + str(n)] = "KEY_F" + str(n)


class LegacyUInput:
    """Linux 4.4-compatible uinput device using the legacy user_dev ABI."""
    def __init__(self, codes):
        self.fd = os.open("/dev/uinput", os.O_WRONLY | os.O_NONBLOCK)
        try:
            fcntl.ioctl(self.fd, UI_SET_EVBIT, ecodes.EV_KEY)
            for code in sorted(set(codes)):
                fcntl.ioctl(self.fd, UI_SET_KEYBIT, code)
            name = b"R36 STORE Virtual Keyboard"
            payload = struct.pack("@80sHHHHI" + "i" * 256,
                                  name, 0x03, 0x1209, 0x0036, 1, 0,
                                  *([0] * 256))
            os.write(self.fd, payload)
            fcntl.ioctl(self.fd, UI_DEV_CREATE)
            time.sleep(0.12)
        except Exception:
            os.close(self.fd)
            raise

    def write(self, event_type, code, value):
        now = time.time()
        event = struct.pack("@llHHi", int(now), int((now % 1) * 1000000),
                            event_type, code, value)
        os.write(self.fd, event)

    def syn(self):
        self.write(ecodes.EV_SYN, ecodes.SYN_REPORT, 0)

    def close(self):
        try:
            fcntl.ioctl(self.fd, UI_DEV_DESTROY)
        finally:
            os.close(self.fd)


def stop_signal(_signum, _frame):
    global RUNNING
    RUNNING = False


def keycode(name):
    name = name.strip().lower()
    if not name or name in ('"', "''", "none", "disabled", "repeat"):
        return None
    code_name = KEY_NAMES.get(name, "KEY_" + name.upper())
    return getattr(ecodes, code_name, None)


def load_profile(path):
    mapping = {}
    repeats = set()
    with open(path, "r", encoding="utf-8", errors="replace") as source:
        for raw in source:
            line = raw.split("#", 1)[0].strip()
            if not line or "=" not in line:
                continue
            left, right = [part.strip().lower() for part in line.split("=", 1)]
            if right == "repeat":
                repeats.add(left)
                continue
            if right in ('"', "''", ""):
                continue
            code = keycode(right)
            if code is not None:
                mapping.setdefault(left, []).append(code)
    return mapping, repeats


def find_gamepad():
    for path in list_devices():
        try:
            dev = InputDevice(path)
            caps = dev.capabilities()
            keys = set(caps.get(ecodes.EV_KEY, []))
            axes = {entry[0] if isinstance(entry, tuple) else entry
                    for entry in caps.get(ecodes.EV_ABS, [])}
            if ecodes.BTN_SOUTH in keys and ecodes.BTN_EAST in keys and ecodes.ABS_X in axes:
                return dev
            dev.close()
        except Exception:
            pass
    raise RuntimeError("Controle fisico nao encontrado")


def main():
    if len(sys.argv) != 2:
        print("Uso: keymapper.py perfil.gptk", file=sys.stderr)
        return 2
    mapping, repeats = load_profile(sys.argv[1])
    if not mapping:
        print("Perfil sem teclas mapeadas", file=sys.stderr)
        return 2

    dev = find_gamepad()
    key_codes = sorted({code for values in mapping.values() for code in values})
    ui = LegacyUInput(key_codes)
    source_keys = {}
    key_counts = {}
    repeat_sources = set()
    axis_sources = {}

    button_sources = {
        ecodes.BTN_SOUTH: "a", ecodes.BTN_EAST: "b",
        ecodes.BTN_NORTH: "x", ecodes.BTN_WEST: "y",
        ecodes.BTN_TL: "l1", ecodes.BTN_TL2: "l2",
        ecodes.BTN_TR: "r1", ecodes.BTN_TR2: "r2",
        ecodes.BTN_TRIGGER_HAPPY1: "back",
        ecodes.BTN_TRIGGER_HAPPY2: "start",
        ecodes.BTN_TRIGGER_HAPPY3: "l3",
        ecodes.BTN_TRIGGER_HAPPY4: "r3",
    }
    dpad_sources = {
        ecodes.BTN_DPAD_UP: "up", ecodes.BTN_DPAD_DOWN: "down",
        ecodes.BTN_DPAD_LEFT: "left", ecodes.BTN_DPAD_RIGHT: "right",
    }
    axis_map = {
        ecodes.ABS_X: ("left_analog_left", "left_analog_right"),
        ecodes.ABS_Y: ("left_analog_up", "left_analog_down"),
        ecodes.ABS_RX: ("right_analog_left", "right_analog_right"),
        ecodes.ABS_RY: ("right_analog_up", "right_analog_down"),
    }

    def send(code, value):
        ui.write(ecodes.EV_KEY, code, value)
        ui.syn()

    def set_source(source, pressed):
        values = mapping.get(source, [])
        was = source in source_keys
        if pressed and not was:
            source_keys[source] = values
            for code in values:
                count = key_counts.get(code, 0)
                key_counts[code] = count + 1
                if count == 0:
                    send(code, 1)
            if source in repeats:
                repeat_sources.add(source)
        elif not pressed and was:
            repeat_sources.discard(source)
            for code in source_keys.pop(source):
                count = key_counts.get(code, 1) - 1
                if count <= 0:
                    key_counts.pop(code, None)
                    send(code, 0)
                else:
                    key_counts[code] = count

    def set_axis(axis, value):
        directions = axis_map.get(axis)
        if not directions:
            return
        info = dev.absinfo(axis)
        threshold = max(THRESHOLD, int((info.max - info.min) * 0.22))
        if value <= -threshold:
            next_source = directions[0]
        elif value >= threshold:
            next_source = directions[1]
        else:
            next_source = None
        old_source = axis_sources.get(axis)
        if old_source != next_source:
            if old_source:
                set_source(old_source, False)
            if next_source:
                set_source(next_source, True)
                axis_sources[axis] = next_source
            else:
                axis_sources.pop(axis, None)

    signal.signal(signal.SIGTERM, stop_signal)
    signal.signal(signal.SIGINT, stop_signal)
    try:
        dev.grab()
        while RUNNING:
            ready, _, _ = select.select([dev.fd], [], [], 0.08)
            if ready:
                for event in dev.read():
                    if event.type == ecodes.EV_KEY:
                        source = button_sources.get(event.code) or dpad_sources.get(event.code)
                        if source and event.value in (0, 1):
                            set_source(source, event.value == 1)
                    elif event.type == ecodes.EV_ABS:
                        set_axis(event.code, event.value)
            now = time.monotonic()
            if repeat_sources and now >= getattr(main, "next_repeat", 0):
                for source in list(repeat_sources):
                    for code in source_keys.get(source, []):
                        ui.write(ecodes.EV_KEY, code, 2)
                ui.syn()
                main.next_repeat = now + 0.085
    finally:
        for code in list(key_counts):
            try:
                send(code, 0)
            except Exception:
                pass
        try:
            dev.ungrab()
        except Exception:
            pass
        dev.close()
        ui.close()
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("R36 KEYMAPPER:", error, file=sys.stderr, flush=True)
        raise SystemExit(1)

[executed on device: gilbert (f7f1466f-4c0b-4a9b-8c86-4d8bdba1ba83)]