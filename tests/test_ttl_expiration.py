#!/usr/bin/env python3
"""Test TTL expiration to ensure no double-free bugs."""

import socket
import struct
import time
import sys

def send_request(sock, *args):
    """Send a request to the server."""
    payload = struct.pack('<I', len(args))
    for arg in args:
        arg_bytes = arg.encode('utf-8')
        payload += struct.pack('<I', len(arg_bytes))
        payload += arg_bytes
    frame = struct.pack('<I', len(payload)) + payload
    sock.sendall(frame)

def recv_response(sock):
    """Receive a response from the server."""
    length_bytes = sock.recv(4)
    if len(length_bytes) < 4:
        return None
    length = struct.unpack('<I', length_bytes)[0]
    payload = b''
    while len(payload) < length:
        chunk = sock.recv(length - len(payload))
        if not chunk:
            break
        payload += chunk
    return payload

def test_ttl_expiration():
    """Test that TTL expiration doesn't cause double-free."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect(('127.0.0.1', 8080))
        
        # Set a key with a short TTL
        send_request(sock, 'set', 'test_key', 'test_value')
        recv_response(sock)
        
        send_request(sock, 'pexpire', 'test_key', '10')
        resp = recv_response(sock)
        # Response should be 1 (success)
        
        # Wait for expiration
        time.sleep(0.015)
        
        # Try to get the expired key - should return nil
        send_request(sock, 'get', 'test_key')
        resp = recv_response(sock)
        assert resp == b'\x00', f"Expected nil for expired key, got {resp}"
        
        # Make sure server is still responsive
        send_request(sock, 'ping')
        resp = recv_response(sock)
        assert resp is not None, "Server should still respond after TTL expiration"
        
        print("✅ TTL expiration test passed")
        return True
    except Exception as e:
        print(f"❌ TTL expiration test failed: {e}")
        return False
    finally:
        sock.close()

if __name__ == '__main__':
    success = test_ttl_expiration()
    sys.exit(0 if success else 1)
