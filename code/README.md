# IP Packet Fragmentation

This code demonstrates how the kernel handles IP packet fragmentation on IPv4, IPv6 and on dual-stack sockets.

## Usage

On Linux and macOS:
```sh
gcc -o main main.c
```

Send on an IPv4 socket:
```sh
./main ipv4
```

Send on an IPv6 socket:
```sh
./main ipv6
```

Send on a dual-stack socket:
```sh
./main dual
```

The kernel's behavior can be observed using tcpdump or Wireshark.
