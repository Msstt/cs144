#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

void NetworkInterfaceTimer::update(ip32 ip_add, EthernetAddress eth_add) {
    if (last_.find(ip_add) != last_.end()) {
        order_.erase(make_pair(last_[ip_add], ip_add));
    }
    last_[ip_add] = now_time_;
    order_.insert(make_pair(last_[ip_add], ip_add));
    data_[ip_add] = eth_add;
}

void NetworkInterfaceTimer::tick(size_t ms_since_last_tick) {
    now_time_ += ms_since_last_tick;
    while (!order_.empty()) {
        auto ip_add = order_.begin()->second;
        if (now_time_ - last_[ip_add] <= expire_ms_) {
            break;
        }
        order_.erase( make_pair(last_[ip_add], ip_add));
        last_.erase(ip_add);
        data_.erase(ip_add);
    }
}

bool NetworkInterfaceTimer::exist(ip32 ip_add) {
    return last_.find(ip_add) != last_.end();
}

EthernetAddress NetworkInterfaceTimer::get(ip32 ip_add) {
    return data_[ip_add];
}

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
        : name_( name )
        , port_( notnull( "OutputPort", move( port ) ) )
        , ethernet_address_( ethernet_address )
        , ip_address_( ip_address )
{
    cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
         << ip_address.ip() << "\n";
}

void NetworkInterface::transmit_IPv4( const InternetDatagram& dgram, EthernetAddress dst) {
    EthernetFrame frame;
    frame.header.dst = dst;
    frame.header.src = ethernet_address_;
    frame.header.type = EthernetHeader::TYPE_IPv4;
    frame.payload = serialize(dgram);
    transmit(frame);
}

void NetworkInterface::transmit_ARP(ip32 dst, uint16_t opcode) {
    EthernetFrame frame;
    frame.header.src = ethernet_address_;
    frame.header.type = EthernetHeader::TYPE_ARP;
    ARPMessage payload;
    payload.sender_ethernet_address = ethernet_address_;
    payload.sender_ip_address = ip_address_.ipv4_numeric();
    payload.target_ip_address = dst;
    if (add_.exist(dst)) {
        frame.header.dst = add_.get(dst);
        payload.target_ethernet_address = add_.get(dst);
    } else {
        frame.header.dst = ETHERNET_BROADCAST;
    }
    payload.opcode = opcode;
    frame.payload = serialize(payload);
    transmit(frame);
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
    ip32 dst = next_hop.ipv4_numeric();
    if (add_.exist(dst)) {
        transmit_IPv4(dgram, add_.get(dst));
    } else {
        need_[dst].push(dgram);
        if (!ARP_.exist(dst)) {
            ARP_.update(dst, ethernet_address_);
            transmit_ARP(dst, ARPMessage::OPCODE_REQUEST);
        }

    }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
    if (frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_) {
        return;
    }
    if (frame.header.type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram msg;
        if (!parse(msg, frame.payload)) {
            return;
        }
        datagrams_received_.push(msg);
    }
    if (frame.header.type == EthernetHeader::TYPE_ARP) {
        ARPMessage msg;
        if (!parse(msg, frame.payload)) {
            return;
        }
        add_.update(msg.sender_ip_address, msg.sender_ethernet_address);
        if (msg.opcode == ARPMessage::OPCODE_REQUEST) {
            if (msg.target_ip_address == ip_address_.ipv4_numeric()) {
                transmit_ARP(msg.sender_ip_address, ARPMessage::OPCODE_REPLY);
            }
        }
        while (!need_[msg.sender_ip_address].empty()) {
            transmit_IPv4(need_[msg.sender_ip_address].front(), add_.get(msg.sender_ip_address));
            need_[msg.sender_ip_address].pop();
        }
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
    ARP_.tick(ms_since_last_tick);
    add_.tick(ms_since_last_tick);
}