import serial
import struct
import time
import math
import os

# ---------------------------------------------------------
# Commands & Constants
# ---------------------------------------------------------
PC_CMD_START = 0x07  # Initial command sent to STM32 to start update process
PC_CMD_SENDING = 0x67  # Command indicating data chunk transmission
PC_CMD_FINISHED = 0x99  # Command indicating file transfer completion
PC_CMD_WAIT = 0x20  # Command sent when STM32 buffer is full to trigger flash write

STM_ACK_READY = 0x49  # STM32 is ready to receive next packet
STM_ACK_PAUSE = 0x69  # STM32 buffer is full, pause transmission
STM_ACK_FINISHED = 0x55  # STM32 successfully finished flashing

# Protocol configurations
FIRMWARE_VERSION = 0x67  # Define firmware version variable here
PAYLOAD_SIZE = 1016
PACKET_SIZE = 1024

# Target binary file name
FIRMWARE_FILE = "App.bin"


# ---------------------------------------------------------
# Function to simulate STM32 Hardware CRC32 calculation
# ---------------------------------------------------------
def software_crc32_c_style(data: bytes) -> int:
    crc = 0xFFFFFFFF
    for i in range(0, len(data), 4):
        word = int.from_bytes(data[i : i + 4], byteorder="little")
        crc ^= word
        for j in range(32):
            if crc & 0x80000000:
                crc = ((crc << 1) ^ 0x04C11DB7) & 0xFFFFFFFF
            else:
                crc = (crc << 1) & 0xFFFFFFFF
    return crc


# ---------------------------------------------------------
# Packet Builder Helper (Includes Version Variable)
# ---------------------------------------------------------
def build_packet(command, payload_bytes=b""):
    """Construct a 1024-byte packet: Header(4) + Data(1016) + CRC(4)"""
    # Pack command, version, and packet size into 4-byte header
    header = struct.pack("<B B H", command, FIRMWARE_VERSION, PACKET_SIZE)
    padded_payload = payload_bytes.ljust(PAYLOAD_SIZE, b"\x00")
    data_to_crc = header + padded_payload
    my_crc = software_crc32_c_style(data_to_crc)
    return data_to_crc + struct.pack("<I", my_crc)


# ---------------------------------------------------------
# Function to wait and receive responses from STM32
# ---------------------------------------------------------
def wait_for_stm32(ser, expected_acks, timeout_retries=1000):
    """Function to wait for expected ACKs from STM32 (reads in chunks of 1024 bytes)"""
    retries = 0
    while retries < timeout_retries:
        if ser.in_waiting >= PACKET_SIZE:
            bytes_to_read = (ser.in_waiting // PACKET_SIZE) * PACKET_SIZE
            rx_data = ser.read(bytes_to_read)

            last_packet = rx_data[-PACKET_SIZE:]
            ack_cmd = last_packet[0]

            if ack_cmd in expected_acks:
                return ack_cmd
            else:
                print(f"[!] Unexpected ACK or garbage received: 0x{ack_cmd:02X}")

        time.sleep(0.01)
        retries += 1
    return None


# ---------------------------------------------------------
# Serial Port Configuration & Main Execution
# ---------------------------------------------------------
PORT = "COM12"
BAUDRATE = 115200

# Load binary firmware file into memory
if not os.path.exists(FIRMWARE_FILE):
    print(f"Error: File {FIRMWARE_FILE} not found in directory!")
    exit()

with open(FIRMWARE_FILE, "rb") as f:
    firmware_data = f.read()

total_size = len(firmware_data)

# Automatically calculate total chunks based on actual file size
total_chunks = math.ceil(total_size / PAYLOAD_SIZE)
print(
    f"Successfully loaded {FIRMWARE_FILE}! Total size: {total_size} bytes ({total_chunks} packets)"
)

try:
    ser = serial.Serial(PORT, BAUDRATE, timeout=1)
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    print(f"Successfully connected to {PORT}!")
except Exception as e:
    print(f"Failed to open port {PORT}: {e}")
    exit()

try:
    print("\n--- Starting Firmware Update Process ---")

    # Step 0: Send start command (0x07) with version to wake up STM32
    print(
        f"Sending start command (0x07) with Version : {FIRMWARE_VERSION:02X} to STM32..."
    )
    ser.write(build_packet(PC_CMD_START))

    for i in range(total_chunks):

        # Step 1 & 5: Wait for STM32 to respond with READY (0x49) or PAUSE (0x69)
        ack = wait_for_stm32(ser, [STM_ACK_READY, STM_ACK_PAUSE])

        if ack == STM_ACK_PAUSE:
            print(f"\n    -> [!] STM32 Buffer is full! (Received 0x69)")
            print(
                "    -> Sending WAIT command (0x20) for STM32 to write flash memory..."
            )

            # Step 4: Send WAIT command (0x20) back
            ser.write(build_packet(PC_CMD_WAIT))

            print("    -> Waiting for STM32 to finish flashing...")
            ack = wait_for_stm32(ser, [STM_ACK_READY], timeout_retries=50)
            if ack == STM_ACK_READY:
                print(
                    "    -> STM32 is ready (Received 0x49). Resuming transmission loop!\n"
                )
            else:
                print("    -> Error: STM32 Timeout while waiting for flash completion!")
                break

        if ack == STM_ACK_READY:
            # Step 2 & 6: Slice 1016 bytes from binary file and send with command 0x67
            start_idx = i * PAYLOAD_SIZE
            end_idx = min(start_idx + PAYLOAD_SIZE, total_size)
            chunk_data = firmware_data[start_idx:end_idx]

            print(
                f"[{i+1}/{total_chunks}] Sending chunk {len(chunk_data)} bytes (0x67)..."
            )
            packet = build_packet(PC_CMD_SENDING, chunk_data)
            ser.write(packet)
        else:
            print(f"Timeout! No response received at chunk {i+1}")
            break

    # Step 7: Send FINISHED command (0x99) after all data chunks are sent
    print("\n--- Transfer completed. Waiting for sync to send FINISHED (0x99) ---")
    ack = wait_for_stm32(ser, [STM_ACK_READY], timeout_retries=30)

    if ack == STM_ACK_READY:
        ser.write(build_packet(PC_CMD_FINISHED))

        # Step 8: Wait for final success acknowledgement (0x55) from STM32
        print("Waiting for final flash success confirmation (0x55)... ", end="")
        ack_finish = wait_for_stm32(ser, [STM_ACK_FINISHED], timeout_retries=100)

        if ack_finish == STM_ACK_FINISHED:
            print("Success! (0x55) Firmware successfully updated.")
        else:
            print("Failed to receive final confirmation from STM32.")
    else:
        print("Error: STM32 not ready to accept FINISHED command.")

except KeyboardInterrupt:
    print("\nProcess stopped by user.")
finally:
    if "ser" in locals() and ser.is_open:
        ser.close()
