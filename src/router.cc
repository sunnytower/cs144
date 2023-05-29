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
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  route_table_.push_back( { route_prefix, prefix_length, next_hop, interface_num } );
}

void Router::route()
{
  /* use for longest prefix match */
  sort( route_table_.begin(), route_table_.end(), []( const route_entry& a, const route_entry& b ) {
    return a.prefix_length > b.prefix_length;
  } );
  /* query every interface */
  for ( auto& interface_ : interfaces_ ) {
    auto datagram = interface_.maybe_receive();
    if ( datagram.has_value() ) {
      /* drop the package */
      if ( datagram->header.ttl == 0 || datagram->header.ttl == 1 ) {
        continue;
      }

      /* match */
      for ( const auto& entry : route_table_ ) {
        /* match the prefix */
        if ( ( datagram->header.dst & ( 0xFFFFFFFF << ( 32 - entry.prefix_length ) ) ) == entry.route_prefix ) {
          /* match the next hop */
          if ( entry.next_hop.has_value() ) {
            /* send the package to the next hop */
            interface_.send_datagram( datagram.value(), entry.next_hop.value() );
          } else {
            /* send the package to the destination */
            interface( entry.interface_num )
              .send_datagram( datagram.value(), Address::from_ipv4_numeric( datagram->header.dst ) );
          }
          break;
        }
      }
    }
  }
}
