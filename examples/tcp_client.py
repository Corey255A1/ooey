#!/usr/bin/env python3
import socket
import time
import sys

def main():
    host = '127.0.0.1'
    port = 12345

    print(f"Connecting to {host}:{port}...")
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((host, port))
        print("Connected successfully to OOEY Network Server!")
    except Exception as e:
        print(f"Error connecting: {e}")
        print("Ensure the C++ hello_network executable is running first.")
        sys.exit(1)

    try:
        # 1. Send some text
        print("Sending command: TEXT Loading wizard assets...")
        s.sendall(b"TEXT Loading wizard assets...\n")
        time.sleep(2.0)

        # 2. Cycle selection in the listbox forwards
        for i in range(10):
            print(f"Sending command: SELECT {i}")
            s.sendall(f"SELECT {i}\n".encode('utf-8'))
            time.sleep(0.8)

        # 3. Modify text input
        print("Sending command: TEXT Network drive successful!")
        s.sendall(b"TEXT Network drive successful!\n")
        time.sleep(2.0)

        # 4. Cycle selection in the listbox backwards
        for i in range(9, -1, -1):
            print(f"Sending command: SELECT {i}")
            s.sendall(f"SELECT {i}\n".encode('utf-8'))
            time.sleep(0.5)

        # 5. Send close/exit command
        print("Sending command: EXIT")
        s.sendall(b"EXIT\n")
        time.sleep(0.5)

    except KeyboardInterrupt:
        print("Exiting client.")
    finally:
        s.close()
        print("Connection closed.")

if __name__ == '__main__':
    main()
