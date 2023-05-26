#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) )
  , initial_RTO_ms_( initial_RTO_ms )
  , timer_( initial_RTO_ms_ )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return next_seqno_ - last_ackno_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return consecutive_retransmissions_;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  if ( !segments_out_.empty() ) {
    const TCPSenderMessage msg = segments_out_.front();
    segments_out_.pop_front();
    return msg;
  }
  return {};
}

void TCPSender::push( Reader& outbound_stream )
{
  if ( outbound_stream.has_error() ) {
    return;
  }
  const auto effective_window_size = last_window_size_ == 0 ? 1 : last_window_size_;
  while ( sequence_numbers_in_flight() < effective_window_size ) {
    TCPSenderMessage msg;
    if ( !SYN ) {
      msg.SYN = true;
      SYN = true;
    }
    msg.seqno = Wrap32::wrap( next_seqno_, isn_ );
    const auto payload_size = min( effective_window_size - sequence_numbers_in_flight() - msg.sequence_length(),
                                   TCPConfig::MAX_PAYLOAD_SIZE );
    read( outbound_stream, payload_size, msg.payload );
    if ( !FIN && outbound_stream.is_finished() ) {
      if ( sequence_numbers_in_flight() + msg.sequence_length() < effective_window_size ) {
        msg.FIN = true;
        FIN = true;
      }
    }
    if ( msg.sequence_length() > 0 ) {
      segments_out_.push_back( msg );
      outstanding_segments_.push( msg );
      next_seqno_ += msg.sequence_length();
      if ( !timer_.isStart() ) {
        timer_.start();
      }
    } else {
      /* not available data */
      break;
    }
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  TCPSenderMessage msg;
  msg.seqno = Wrap32::wrap( next_seqno_, isn_ );
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  last_window_size_ = msg.window_size;
  if ( msg.window_size == 0 ) {
    zero_window_ = true;
  }
  if ( !SYN ) {
    return;
  }
  const auto msg_ackno = msg.ackno.value().unwrap( isn_, last_ackno_ );
  if ( msg_ackno > last_ackno_ && msg_ackno <= next_seqno_ ) {
    last_ackno_ = msg_ackno;
  } else {
    return;
  }
  timer_.resetRTO();
  consecutive_retransmissions_ = 0;
  while ( !outstanding_segments_.empty() ) {
    const auto& outstanding_msg = outstanding_segments_.front();
    if ( last_ackno_ - outstanding_msg.seqno.unwrap( isn_, last_ackno_ ) >= outstanding_msg.sequence_length() ) {
      outstanding_segments_.pop();
    } else {
      break;
    }
  }
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  if ( !timer_.alarm( ms_since_last_tick ) ) {
    return;
  }
  /* retransmit the earliest segment */
  if ( !outstanding_segments_.empty() ) {
    segments_out_.push_front( outstanding_segments_.front() );
    /* increment counter and double RTO if window size is not zero */
    if ( !zero_window_ ) {
      consecutive_retransmissions_++;
      timer_.doubleRTO();
    }
    /* reset timer_ */
    // timer_.reset();
  } else {
    timer_.stop();
  }
}
