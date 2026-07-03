import socket
import time
import struct

# Network Configuration
PORT = 8080

# Match C++ Structure Packing Layout
# '<i f f f i' means: Little-endian, int, float, float, float, int (Total 20 bytes)
COMMAND_PACKET_FMT = '<ifffi'
COMMAND_PACKET_SIZE = struct.calcsize(COMMAND_PACKET_FMT)

# '<64s i' means: Little-endian, 64-byte char array, int (Total 68 bytes)
RESPONSE_PACKET_FMT = '<64si'

# Artificial Bounds & State Variables
currentX, currentY, currentZ = 0.0, 0.0, 0.0
LIMIT_MIN = -5.0
LIMIT_MAX = 5.0
currentGripperState = -1  # -1 = Uninitialized, 0 = Closed, 1 = Open

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
    # Set up TCP Server
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(('0.0.0.0', PORT))  # Listens on all interfaces (Wi-Fi/Ethernet)
    server_socket.listen(3)

    print(f"Server listening on port {PORT}...")

    while True:
        client_socket, client_address = server_socket.accept()
        print(f"Client Connected from {client_address}.")
        
        client_socket.setblocking(False)  # Non-blocking mode matching MSG_DONTWAIT
        wasMovingLastTick = False
        lastPacketTime = time.time()

        while True:
            try:
                # Attempt to read data from the non-blocking socket
                data = client_socket.recv(COMMAND_PACKET_SIZE)
                
                if len(data) == COMMAND_PACKET_SIZE:
                    lastPacketTime = time.time()  # Refresh timeout clock
                    
                    # Unpack bytes into Python variables
                    p_type, p_x, p_y, p_z, p_gripper = struct.unpack(COMMAND_PACKET_FMT, data)
                    
                    status_message = b""
                    limit_reached = 0

                    if p_type == 1:  # COORDINATES
                        status_message, limit_reached = processMovement(p_x, p_y, p_z)
                        wasMovingLastTick = True
                    elif p_type == 2:  # GRIPPER
                        status_message, limit_reached = processGripper(p_gripper)

                    # Pack and send the binary response back
                    response = struct.pack(RESPONSE_PACKET_FMT, status_message, limit_reached)
                    client_socket.sendall(response)
                    
                elif len(data) == 0:
                    # Client disconnected gracefully
                    print("Client Disconnected.")
                    break

            except BlockingIOError:
                # No data available to read right now; continue down to idle checks
                pass
            except ConnectionResetError:
                print("Client Disconnected unexpectedly.")
                break

            # Continuous key monitoring (90ms idle timeout)
            elapsed_time = (time.time() - lastPacketTime) * 1000  # Convert to milliseconds
            if wasMovingLastTick and (elapsed_time > 90):
                # Pack and send idle message
                idle_response = struct.pack(RESPONSE_PACKET_FMT, b"Status: Idle", 0)
                client_socket.sendall(idle_response)
                wasMovingLastTick = False  # Prevent spamming idle message

            time.sleep(0.005)  # 5ms loop padding

        client_socket.close()

if __name__ == '__main__':
    main()