#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    // Your code here.
    RouteElem route;
    route._route_prefix = route_prefix;
    route._prefix_length = prefix_length;
    route._next_hop = next_hop;
    route._interface_num = interface_num;
    _routes.push_back(route);
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    // Your code here.
    const auto idx = find_longest_prefix(dgram.header().dst);
    if (dgram.header().ttl > 1 && idx > -1) {
        dgram.header().ttl--;
        if (_routes[idx]._next_hop.has_value())
            interface(_routes[idx]._interface_num).send_datagram(dgram, _routes[idx]._next_hop->ip());
        else
            interface(_routes[idx]._interface_num).send_datagram(dgram, Address::from_ipv4_numeric(dgram.header().dst));
    }
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}

//! Return the index of route position in router's table
//! \param[in] dst the destionation ip address
int Router::find_longest_prefix(const uint32_t dst) {
    int longest = -1;
    int idx = -1;
    int count = 0;
    for (auto route : _routes) {
        const uint32_t mask = route._prefix_length == 0 ? 0 : (0xffffffff << (32 - route._prefix_length));
        if ((mask & dst) == (mask & route._route_prefix)) {
            if (longest < route._prefix_length) {
                idx = count;
                longest = route._prefix_length;
            }
        }
        count++;
    }
    return idx;
}