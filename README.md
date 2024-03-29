# CS144_Introduction_to_Computer_Networking_Solutions_Fall2020
Solutions for Stanford-CS144-Fall2020

[Course Main Page](https://cs144.github.io)

[Course Github](https://github.com/cs144/sponge)

## Docker image Usage
```bash
docker pull vidocqh/cs144:latest
docker run -td --privileged --name cs144_container vidocqh/cs144
```
1. You can use `-v` flag for mounting the `sponge` directory.
2. You need to edit `tun.sh` and `tap.sh` for avoiding `root` user checking. (Since `Lab 4`)
    + [tun.sh](https://github.com/VidocqH/CS144_Introduction_to_Computer_Networking/blob/main/sponge/tun.sh)
    + [tap.sh](https://github.com/VidocqH/CS144_Introduction_to_Computer_Networking/blob/main/sponge/tap.sh)

## Finished
+ **Lab 0: Networking Warmup**
    1. sponge/apps/webget.cc
    2. sponge/libsponge/byte_stream.cc
    3. sponge/libsponge/byte_stream.hh
+ **Lab 1: Stitching substrings into a byte stream**
    1. sponge/libsponge/stream_reassembler.cc
    2. sponge/libsponge/stream_reassembler.hh
+ **Lab 2: the TCP receiver**
    1. sponge/libsponge/wrapping_integers.cc
    2. sponge/libsponge/wrapping_integers.hh
    3. sponge/libsponge/tcp_receiver.cc
    4. sponge/libsponge/tcp_receiver.hh
    5. Bug fixed:
        - Lab 1: sponge/libsponge/stream_reassembler.cc & stream_reassembler.hh
+ **Lab 3: the TCP sender**
    1. sponge/libsponge/tcp_sender.cc
    2. sponge/libsponge/tcp_sender.hh
+ **Lab 4: the TCP connection**
    1. sponge/libsponge/tcp_connection.cc
    2. sponge/libsponge/tcp_sender.hh
    3. Bug fixed:
        - Lab 0: sponge/apps/webget.cc
        - Lab 0: sponge/libsponge/byte_stream.cc & byte_stream.hh
        - Lab 2: sponge/libsponge/tcp_receiver.cc & tcp_receiver.hh
        - Lab 3: sponge/libsponge/tcp_sender.cc & tcp_sender.hh
+ **Lab 5: the network interface**
    1. sponge/libsponge/network_interface.cc
    2. sponge/libsponge/network_interface.hh
+ **Lab 6: the IP router**
    1. sponge/libsponge/router.cc
    2. sponge/libsponge/router.hh
