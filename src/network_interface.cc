#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  /* if destination ethernet address is known. */
  if ( arp_table_.find( next_hop.ipv4_numeric() ) != arp_table_.end() ) {
    EthernetFrame frame;
    frame.header.type = EthernetHeader::TYPE_IPv4;
    frame.header.src = ethernet_address_;
    frame.header.dst = arp_table_.at( next_hop.ipv4_numeric() ).ethernet_address;
    frame.payload = serialize( dgram );
    send_queue_.push( frame );
  } else {
    if (arp_waiting_.find(next_hop.ipv4_numeric()) == arp_waiting_.end()) {
      /* need to send arp request */
      ARPMessage arp;
      arp.opcode = ARPMessage::OPCODE_REQUEST;
      arp.sender_ethernet_address = ethernet_address_;
      arp.sender_ip_address = ip_address_.ipv4_numeric();
      arp.target_ip_address = next_hop.ipv4_numeric();
      // arp.target_ethernet_address = ETHERNET_BROADCAST;

      EthernetFrame frame;
      frame.header.type = EthernetHeader::TYPE_ARP;
      frame.header.dst = ETHERNET_BROADCAST;
      frame.header.src = ethernet_address_;
      frame.payload = serialize( arp );
      send_queue_.push( frame );
      arp_waiting_.insert({next_hop.ipv4_numeric(), ARP_REQUEST_TIMEOUT});
      /* save datagram */
      datagram_waiting_.insert({next_hop, dgram});
    }
  }

}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST ) {
    return {};
  }
  /* ipv4 package */
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram dgram;
    if (parse( dgram, frame.payload )) {
      return dgram;
    } else {
      return {};
    }
  }
  /* arp package */
  if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arp;
    if (parse( arp, frame.payload )) {
      if (arp.opcode == ARPMessage::OPCODE_REQUEST && arp.target_ip_address == ip_address_.ipv4_numeric()) {
        /* reply */
        ARPMessage reply;
        reply.opcode = ARPMessage::OPCODE_REPLY;
        reply.sender_ethernet_address = ethernet_address_;
        reply.sender_ip_address = ip_address_.ipv4_numeric();
        reply.target_ethernet_address = arp.sender_ethernet_address;
        reply.target_ip_address = arp.sender_ip_address;

        EthernetFrame reply_frame;
        reply_frame.header.type = EthernetHeader::TYPE_ARP;
        reply_frame.header.dst = arp.sender_ethernet_address;
        reply_frame.header.src = ethernet_address_;
        reply_frame.payload = serialize( reply );
        send_queue_.push( reply_frame );
      } else if (arp.opcode == ARPMessage::OPCODE_REPLY) {
        /* update arp table */
        arp_table_.insert({arp.sender_ip_address, {arp.sender_ethernet_address, TTL_TIMEOUT}});
        /* remove arp_waiting_ */
        if (arp_waiting_.find(arp.sender_ip_address) != arp_waiting_.end()) {
          arp_waiting_.erase(arp.sender_ip_address);
        }
        /* send datagram_waiting*/
        for (auto& elem: datagram_waiting_) {
          if (elem.first.ipv4_numeric() == arp.sender_ip_address) {
            send_datagram(elem.second, elem.first);
            datagram_waiting_.erase(elem.first);
          }
        }
      }
    }
  }
  return {};
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  for ( auto iter = arp_table_.begin(); iter != arp_table_.end(); ) {
    iter->second.ttl -= ms_since_last_tick;
    if (iter->second.ttl <= 0) {
      iter = arp_table_.erase(iter);
    } else {
      iter++;
    }
  }

  for (auto iter = arp_waiting_.begin(); iter != arp_waiting_.end(); ) {
    iter->second -= ms_since_last_tick;
    if (iter->second <= 0) {
      iter = arp_waiting_.erase(iter);
    } else {
      iter++;
    }
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if (send_queue_.empty()) {
    return {};
  } else {
    EthernetFrame frame = send_queue_.front();
    send_queue_.pop();
    return frame;
  }
}
