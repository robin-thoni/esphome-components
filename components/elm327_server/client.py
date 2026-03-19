#!/usr/bin/env python3
import sys
import socket
import argparse
import threading
import select

def receive_loop(sock):
    """Thread to receive and display server replies."""
    while True:
        ready, _, _ = select.select([sock], [], [], 0.1)
        if ready:
            data = sock.recv(4096)
            if not data:
                break
            sys.stdout.write(data.decode(errors='replace'))
            sys.stdout.flush()
        else:
            # Check if stdin closed
            if sys.stdin.closed:
                break

def main():
    parser = argparse.ArgumentParser(description='TCP client: send stdin lines with \\r to server, display replies')
    parser.add_argument('host', help='Server hostname/IP')
    parser.add_argument('port', type=int, help='Server port')
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((args.host, args.port))
    print(f"Connected to {args.host}:{args.port}. Type lines to send (Ctrl+D to exit).", file=sys.stderr)

    # Start receive thread
    recv_thread = threading.Thread(target=receive_loop, args=(sock,), daemon=True)
    recv_thread.start()

    # Send stdin lines
    for line in sys.stdin:
        sock.sendall((line.rstrip('\r\n') + '\r').encode())

    sock.close()

if __name__ == '__main__':
    main()
