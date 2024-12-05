#!/usr/bin/python3

import socket
import time

# Replace with your server's IP and port
HOST = 'localhost'  # or '127.0.0.1' if it's on the same machine
PORT = 8081         # replace with the port your server is listening on

try:
    # Create a socket and connect to the server
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((HOST, PORT))

    # Define message parts to simulate partial sends
    message_parts = [
        b"GET / HTTP/1.1\r\n",
        b"key: localhost\r\n",
        b"Content-length: 10\r\n",
        b"Host: localhost \r\n",
        b"key2: localhost\n \r\n",
        b"key2: otherhost\r\n",
        b"key4: localhost\r\n",
        b"\r\n"
    ]

    # Send each part with a delay in between
    for part in message_parts:
        client_socket.send(part)
        print(f"Sent part: {part.decode()}")
        time.sleep(1)  # Pause for 1 second between sends

    # Close the socket after sending
    client_socket.close()
    print("Connection closed.")

except Exception as e:
    print(f"Error: {e}")
