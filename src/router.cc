#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
//  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
//       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
//       << " on interface " << interface_num << "\n";

    rule_.push_back(Route{route_prefix, prefix_length, next_hop, interface_num});
}

size_t Router::find_index(uint64_t ip_add) {
    int p = - 1;
    for (size_t i = 0; i < rule_.size(); i++) {
        int len = 32 - rule_[i].prefix_length_;
        if (len == 32 || ((ip_add & rule_[i].route_prefix_) >> len) == ((ip_add | rule_[i].route_prefix_) >> len)) {
            if (p == - 1 || rule_[i].prefix_length_ > rule_[p].prefix_length_) {
                p = i;
            }
        }
    }
    return p;
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
    for (size_t i = 0; i < _interfaces.size(); i++) {
        auto &que = interface(i)->datagrams_received();
        while (!que.empty()) {
            auto &dgram = que.front();
            if (dgram.header.ttl > 1) {
                dgram.header.ttl -= 1;
                dgram.header.compute_checksum();
                auto index = find_index(dgram.header.dst);
                std::cout << index << std::endl;
                auto interface_num = rule_[index].interface_num_;
                if (rule_[index].next_hop_.has_value()) {
                    interface(interface_num)->send_datagram(dgram, *rule_[index].next_hop_);
                } else {
                    interface(interface_num)->send_datagram(dgram, Address::from_ipv4_numeric(dgram.header.dst));
                }
            }
            que.pop();
        }
    }
}