#include "tcp_receiver.hh"
#include <cstdint>
using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  if ( message.SYN ) {
    isn_ = message.seqno;
    syn_ = true;
    reassembler.insert( 0, message.payload, message.FIN, inbound_stream );
    return;
  }
  if ( syn_ ) {
    const auto abs_sqeno = message.seqno.unwrap( isn_, inbound_stream.bytes_pushed() );
    reassembler.insert( abs_sqeno - 1, message.payload, message.FIN, inbound_stream );
  }
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  TCPReceiverMessage res;
  if ( inbound_stream.available_capacity() >= UINT16_MAX ) {
    res.window_size = UINT16_MAX;
  } else {
    res.window_size = inbound_stream.available_capacity();
  }

  if ( syn_ ) {
    res.ackno = Wrap32::wrap( inbound_stream.bytes_pushed() + inbound_stream.is_closed() + 1, isn_ );
  } else {
    res.ackno = {};
  }
  return res;
}
