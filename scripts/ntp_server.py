import argparse
import socket
import struct
import time


def create_ntp_packet(stratum, leap, mode=4):
    # Byte 0: LI (bits 6-7), VN (3-5), Mode (0-2)
    first_byte = (leap << 6) | (4 << 3) | mode
    packet = bytearray(48)
    packet[0] = first_byte
    packet[1] = stratum
    return packet


def main():
    parser = argparse.ArgumentParser(description="Inject custom NTP packets to test Pico firmware.")
    parser.add_argument("--host", default="0.0.0.0", help="Listen address")
    parser.add_argument("--port", type=int, default=123, help="Listen port")
    parser.add_argument("--mode", choices=["kod", "leap", "success"], required=True, help="Protocol state to inject")

    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((args.host, args.port))
    print(f"NTP Injector listening on {args.host}:{args.port}...")

    while True:
        data, addr = sock.recvfrom(1024)
        print(f"Received request from {addr}")

        if args.mode == "kod":
            # Stratum 0, LI 3 (KoD)
            packet = create_ntp_packet(0, 3)
            print("Injecting: KOD (Stratum 0)")
        elif args.mode == "leap":
            # Stratum 2, LI 3 (Leap Second Warning)
            packet = create_ntp_packet(2, 3)
            print("Injecting: Leap Second Warning")
        else:
            # Normal Success
            packet = create_ntp_packet(2, 0)
            # Add dummy timestamp
            packet[40:44] = struct.pack("!I", int(time.time() + 2208988800))
            print("Injecting: Valid Success Packet")

        sock.sendto(packet, addr)


if __name__ == "__main__":
    main()
