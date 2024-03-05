#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

void Trie::insert(uint32_t route_prefix, uint8_t prefix_length, optional<Address> next_hop, size_t interface_num) {
    shared_ptr<Node> p = root_;
    for (uint8_t i = 1; i <= prefix_length; i++) {
        bool c = route_prefix >> (32 - i) & 1;
        if (p->next_.find(c) == p->next_.end()) {
            p->next_[c] = make_shared<Node>();
        }
        p = p->next_[c];
    }
    p->value_ = make_shared<std::pair<std::optional<Address>, size_t>>(next_hop, interface_num);
}

std::pair<std::optional<Address>, size_t> Trie::query(uint64_t ip_add) {
    shared_ptr<Node> p = root_;
    std::pair<std::optional<Address>, size_t> result;
    if (p->value_ != nullptr) {
        result = *p->value_;
    }
    for (uint8_t i = 1; i <= 32; i++) {
        bool c = ip_add >> (32 - i) & 1;
        if (p->next_.find(c) == p->next_.end()) {
            break;
        }
        p = p->next_[c];
        if (p->value_ != nullptr) {
            result = *p->value_;
        }
    }
    return result;
}

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

    trie_.insert(route_prefix, prefix_length, next_hop, interface_num);
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
                auto [next_hop, interface_num] = trie_.query(dgram.header.dst);
                if (next_hop.has_value()) {
                    interface(interface_num)->send_datagram(dgram, *next_hop);
                } else {
                    interface(interface_num)->send_datagram(dgram, Address::from_ipv4_numeric(dgram.header.dst));
                }
            }
            que.pop();
        }
    }
}