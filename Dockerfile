FROM ubuntu:20.04
RUN DEBIAN_FRONTEND="noninteractive" apt-get update \
    && DEBIAN_FRONTEND="noninteractive" apt-get -y install git g++ clang-format-6.0 telnet gdb make cmake netcat libpcap-dev traceroute iproute2 iptables
