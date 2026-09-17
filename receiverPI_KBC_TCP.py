import socket
import time
import struct

# --- NETWORK CONFIGURATION ---
HOST = '127.0.0.1'  # Local loopback for single-device Pi 5 setup
PORT = 9000         # Matched to local sender script port

# --- STRUCT FORMATS (Matching C++ / Sender Packing) ---
# CommandPacket: int32, float, float, float, int32 -> '<ifffi' (20 bytes)
COMMAND_PACKET_FMT = '<ifffi'
COMMAND_PACKET_SIZE = struct.calcsize(COMMAND_PACKET_FMT)

# ResponsePacket: 64-byte char array, int32 -> '<64si' (68 bytes)
RESPONSE_PACKET_FMT = '<64si'

# --- STATE VARIABLES & BOUNDS ---
currentX, currentY, currentZ = 0.0, 0.0, 0.0
LIMIT_MIN = -5.0
LIMIT_MAX = 5.0
currentGripperState = -1  # -1 = Uninitialized, 0 = Closed, 1 = Open


def pad_status_message(msg_bytes: bytes) -> bytes:
    """Safely pads or truncates byte messages to exactly 64 bytes for struct packing."""
    return msg_bytes[:64].ljust(64, b'\x00')


def processMovement(packet_x, packet_y, packet_z):
    global currentX, currentY, currentZ
    limit_reached = 0
    status_message = b""

    # Evaluate X Axis
    if packet_x != 0.0:
        nextX = currentX + packet_x
        if nextX < LIMIT_MIN:
            status_message = b"Limit reached: -X limit reached!"
            limit_reached = 1
            # LIMIT REACHED
        elif nextX > LIMIT_MAX:
            status_message = b"Limit reached: +X limit reached!"
            limit_reached = 1
            # LIMIT REACHED
        else:
            currentX = nextX
            status_message = b"Moving in +X direction" if packet_x > 0 else b"Moving in -X direction"
            # MOVEMENT HERE

    # Evaluate Y Axis
    elif packet_y != 0.0:
        nextY = currentY + packet_y
        if nextY < LIMIT_MIN:
            status_message = b"Limit reached: -Y limit reached!"
            limit_reached = 1
            # LIMIT REACHED
        elif nextY > LIMIT_MAX:
            status_message = b"Limit reached: +Y limit reached!"
            limit_reached = 1
            # LIMIT REACHED
        else:
            currentY = nextY
            status_message = b"Moving in +Y direction" if packet_y > 0 else b"Moving in -Y direction"
            # MOVEMENT HERE

    # Evaluate Z Axis
    elif packet_z != 0.0:
        nextZ = currentZ + packet_z
        if nextZ < LIMIT_MIN:
            status_message = b"Limit reached: -Z limit reached (Descend)!"
            limit_reached = 1
            # LIMIT REACHED
        elif nextZ > LIMIT_MAX:
            status_message = b"Limit reached: +Z limit reached (Ascend)!"
            limit_reached = 1
            # LIMIT REACHED
        else:
            currentZ = nextZ
            status_message = b"Moving in +Z direction (Ascend)" if packet_z > 0 else b"Moving in -Z direction (Descend)"
            # MOVEMENT HERE

    return status_message, limit_reached


def processGripper(gripper_action):
    global currentGripperState
    limit_reached = 0
    status_message = b""

    if gripper_action == 1:  # Requesting Open
        if currentGripperState == 1:
            status_message = b"Gripper status: Gripper already open!"
        else:
            currentGripperState = 1
            status_message = b"Gripper action: Opening gripper..."
            # OPEN GRIPPER
    elif gripper_action == 0:  # Requesting Close
        if currentGripperState == 0:
            status_message = b"Gripper status: Gripper already closed!"
        else:
            currentGripperState = 0
            status_message = b"Gripper action: Closing gripper..."
            # CLOSE GRIPPER

    return status_message, limit_reached


def main():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((HOST, PORT))
    server_socket.listen(3)

    print(f"Hardware Mover listening on {HOST}:{PORT}...")

    while True:
        client_socket, client_address = server_socket.accept()
        print(f"Client connected from {client_address}.")

        client_socket.setblocking(False)
        rx_buffer = bytearray()
        wasMovingLastTick = False
        lastPacketTime = time.time()

        while True:
            try:
                # Accumulate data into buffer to handle TCP fragmentation
                data = client_socket.recv(1024)
                if not data:
                    print("Client disconnected gracefully.")
                    break
                rx_buffer.extend(data)

                # Process all fully received command packets
                while len(rx_buffer) >= COMMAND_PACKET_SIZE:
                    packet_bytes = rx_buffer[:COMMAND_PACKET_SIZE]
                    del rx_buffer[:COMMAND_PACKET_SIZE]

                    lastPacketTime = time.time()  # Refresh timeout clock

                    p_type, p_x, p_y, p_z, p_gripper = struct.unpack(COMMAND_PACKET_FMT, packet_bytes)

                    status_message = b""
                    limit_reached = 0

                    if p_type == 1:  # COORDINATES
                        status_message, limit_reached = processMovement(p_x, p_y, p_z)
                        wasMovingLastTick = True
                    elif p_type == 2:  # GRIPPER
                        status_message, limit_reached = processGripper(p_gripper)

                    # Pack and send 68-byte response
                    padded_msg = pad_status_message(status_message)
                    response = struct.pack(RESPONSE_PACKET_FMT, padded_msg, limit_reached)
                    client_socket.sendall(response)

            except BlockingIOError:
                pass  # No socket data available to read right now
            except (ConnectionResetError, BrokenPipeError):
                print("Client disconnected unexpectedly.")
                break

            # Continuous key monitoring (90 ms idle timeout check)
            elapsed_time_ms = (time.time() - lastPacketTime) * 1000
            if wasMovingLastTick and (elapsed_time_ms > 90):
                try:
                    idle_msg = pad_status_message(b"Status: Idle")
                    idle_response = struct.pack(RESPONSE_PACKET_FMT, idle_msg, 0)
                    client_socket.sendall(idle_response)
                except (ConnectionResetError, BrokenPipeError):
                    break
                wasMovingLastTick = False  # Prevent spamming idle message

            time.sleep(0.005)  # 5 ms loop padding

        client_socket.close()


if __name__ == '__main__':
    main()