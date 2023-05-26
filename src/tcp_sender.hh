#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include <deque>
#include <queue>

class TCPTimer
{
private:
  uint64_t initial_RTO_ms_;
  uint64_t logical_RTO_ms_;
  uint64_t current_RTO_ms_;
  bool started_;

public:
  explicit TCPTimer( uint64_t initial_RTO_ms )
    : initial_RTO_ms_( initial_RTO_ms ), logical_RTO_ms_( initial_RTO_ms ), current_RTO_ms_( 0 ), started_( true )
  {}

  bool isStart() const { return started_; }
  uint64_t RTO() const { return logical_RTO_ms_; }
  void doubleRTO() { logical_RTO_ms_ *= 2; }
  void resetRTO()
  {
    logical_RTO_ms_ = initial_RTO_ms_;
    current_RTO_ms_ = 0;
  }
  void reset() { current_RTO_ms_ = 0; }
  void start()
  {
    started_ = true;
    current_RTO_ms_ = 0;
  }
  void stop()
  {
    started_ = false;
    current_RTO_ms_ = 0;
  }
  bool alarm( const size_t ms )
  {
    if ( started_ ) {
      current_RTO_ms_ += ms;
      if ( current_RTO_ms_ >= logical_RTO_ms_ ) {
        return true;
      }
    }
    return false;
  }
};

class TCPSender
{
private:
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  std::deque<TCPSenderMessage> segments_out_ {};
  bool SYN { false };
  bool FIN { false };
  uint64_t consecutive_retransmissions_ { 0 };
  std::queue<TCPSenderMessage> outstanding_segments_ {};
  TCPTimer timer_;
  uint64_t next_seqno_ { 0 };
  uint64_t last_ackno_ { 0 };
  uint64_t last_window_size_ { 0 };

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
