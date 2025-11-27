#!/usr/bin/env python3
import subprocess
import time

def run_client(cmds):
    p = subprocess.Popen(['../bin/client'],
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE,
                         text=True)
    out, err = p.communicate("\n".join(cmds) + "\nexit\n", timeout=5)
    return out, err, p.returncode

def expect(line, needle):
    if needle not in line:
        raise SystemExit(f"Expected '{needle}' in '{line.strip()}'")

def main():
    # Ensure key starts blank
    out, err, rc = run_client(["del ttl_demo_key"])

    # Set key and verify no TTL
    out, err, rc = run_client(["set ttl_demo_key v", "pttl ttl_demo_key"])
    lines = [l for l in out.splitlines() if l.strip()]
    # Expected: nil (from set) then -1 from pttl
    expect(lines[0], "nil")
    expect(lines[1], "-1")

    # Set TTL 1000ms and read it back
    out, err, rc = run_client(["pexpire ttl_demo_key 1000", "pttl ttl_demo_key"])
    lines = [l for l in out.splitlines() if l.strip()]
    expect(lines[0], "1")
    # Some positive milliseconds
    ttl_ms = int(lines[1])
    if ttl_ms <= 0:
        raise SystemExit(f"Expected positive TTL, got {ttl_ms}")

    # Wait past expiry
    time.sleep(1.2)
    out, err, rc = run_client(["pttl ttl_demo_key", "get ttl_demo_key"])
    lines = [l for l in out.splitlines() if l.strip()]
    # pttl should be 0 (clamped) or -2 if already reaped and missing; get should be nil
    if lines[0] not in ("0", "-2"):
        raise SystemExit(f"Expected 0 or -2 after expiry, got {lines[0]}")
    expect(lines[1], "nil")

    print("✅ TTL test passed.")

if __name__ == "__main__":
    main()


