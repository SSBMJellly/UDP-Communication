import socket
import struct
import time
from pynput import keyboard

LOCAL_IP = "127.0.0.1"
LOCAL_PORT = 9000

# CommandPacket: int32, float, float, float, int32 (20 bytes)
CMD_FORMAT = "<ifffi"
CMD_COORDINATES = 1
CMD_GRIPPER = 2

pressed_keys = set()

def on_press(key):
    pressed_keys.add(key)

def on_release(key):
    pressed_keys.discard(key)

def main():
    listener = keyboard.Listener(on_press=on_press, on_release=on_release)
    listener.start()

    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        client_socket.connect((LOCAL_IP, LOCAL_PORT))
        print("Connected to local Hardware Mover script!")
    except Exception as e:
        print(f"Failed to connect to local Mover script on port {LOCAL_PORT}: {e}")
        print("Ensure receiver_mover.py is running first.")
        return

    print("Controls: Arrows (X/Y), A/D (Z), O/C (Gripper), ESC (Exit)")

    o_was_pressed = False
    c_was_pressed = False

    try:
        while True:
            if keyboard.Key.esc in pressed_keys:
                break

            # Movement Delta Coordinates
            x, y, z = 0.0, 0.0, 0.0
            move_requested = False

            if keyboard.Key.left in pressed_keys:   x -= 1.0; move_requested = True
            if keyboard.Key.right in pressed_keys:  x += 1.0; move_requested = True
            if keyboard.Key.up in pressed_keys:     y += 1.0; move_requested = True
            if keyboard.Key.down in pressed_keys:   y -= 1.0; move_requested = True
            if keyboard.KeyCode.from_char('a') in pressed_keys: z += 1.0; move_requested = True
            if keyboard.KeyCode.from_char('d') in pressed_keys: z -= 1.0; move_requested = True

            if move_requested:
                cmd_data = struct.pack(CMD_FORMAT, CMD_COORDINATES, x, y, z, 0)
                client_socket.sendall(cmd_data)

            # Gripper Triggers
            o_is_pressed = keyboard.KeyCode.from_char('o') in pressed_keys
            c_is_pressed = keyboard.KeyCode.from_char('c') in pressed_keys

            if o_is_pressed and not o_was_pressed:
                cmd_data = struct.pack(CMD_FORMAT, CMD_GRIPPER, 0.0, 0.0, 0.0, 1)  # Open
                client_socket.sendall(cmd_data)
            o_was_pressed = o_is_pressed

            if c_is_pressed and not c_was_pressed:
                cmd_data = struct.pack(CMD_FORMAT, CMD_GRIPPER, 0.0, 0.0, 0.0, 0)  # Close
                client_socket.sendall(cmd_data)
            c_was_pressed = c_is_pressed

            time.sleep(0.05)  # 20 Hz loop rate

    finally:
        listener.stop()
        client_socket.close()

if __name__ == "__main__":
    main()