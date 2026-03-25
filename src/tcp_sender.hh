#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <functional>
#include <string>
#include <deque>

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender(ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms)
    : input_(std::move(input))
    , isn_(isn)
    , initial_RTO_ms_(initial_RTO_ms)
    , rto_(initial_RTO_ms)
    , consecutive_retransmissions_(0)
    , windowSize_(1)
    , isSYN_Send_(false)
    , isFIN_Send_(false)
    // , ackno_(isn_)
    , next_seqno_(0)
    , last_ack_absno_(0)
    , outstanding_()
    , bytes_in_flight_(0)
    , ticks(0)
  {
  }

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive(const TCPReceiverMessage& msg);

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void(const TCPSenderMessage&)>;

  /* Push bytes from the outbound stream */
  void push(const TransmitFunction& transmit);

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick(uint64_t ms_since_last_tick, const TransmitFunction& transmit);

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // For testing: how many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // For testing: how many consecutive retransmissions have happened?
  const Writer& writer() const { return input_.writer(); }
  const Reader& reader() const { return input_.reader(); }
  Writer& writer() { return input_.writer(); }

private:
  Reader& reader() { return input_.reader(); }

  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  uint64_t rto_;
  uint64_t consecutive_retransmissions_;

  uint64_t windowSize_;
  bool isSYN_Send_;
  bool isFIN_Send_;

  uint64_t next_seqno_;//absolute seqno
  uint64_t last_ack_absno_;
  std::deque<TCPSenderMessage> outstanding_;
  uint64_t bytes_in_flight_;
  uint64_t ticks;
};
