#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

#include <string>

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // debug("unimplemented sequence_numbers_in_flight() called");
  // return {};
  return bytes_in_flight_;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  // debug("unimplemented consecutive_retransmissions() called");
  // return {};
  return consecutive_retransmissions_;
}

void TCPSender::push(const TransmitFunction& transmit)
{
  // debug( "unimplemented push() called" );
  // (void)transmit;
  if (writer().has_error() == true) {
    TCPSenderMessage msg;
    msg.RST = true;
    msg.seqno = Wrap32::wrap(next_seqno_, isn_);
    transmit(msg);
    outstanding_.clear();
    bytes_in_flight_ = 0;
  }

  if (reader().bytes_buffered() == 0 && reader().is_finished() == false && isSYN_Send_ == true) {
    return;
  }
  uint64_t window = windowSize_;
  if (window == 0) {
    window = 1;
  }

  bool flight_empty = outstanding_.empty();

  while (bytes_in_flight_ < window) {

    uint64_t before = bytes_in_flight_;
    TCPSenderMessage msg;

    uint64_t remain = window - bytes_in_flight_;
    if (isSYN_Send_ == false) {
      msg.SYN = true;
      isSYN_Send_ = true;
      remain--;
    }

    uint64_t length = min({ remain,reader().bytes_buffered(),TCPConfig::MAX_PAYLOAD_SIZE });

    string payload;
    read(reader(), length, payload);
    msg.payload = payload;
    remain -= payload.length();

    if (isFIN_Send_ == false && reader().is_finished() == true && remain >= 1) {
      msg.FIN = true;
      remain--;
      isFIN_Send_ = true;
    }

    if (msg.sequence_length() == 0) {
      break;
    }

    msg.seqno = Wrap32::wrap(next_seqno_, isn_);
    next_seqno_ += msg.sequence_length();

    transmit(msg);
    outstanding_.emplace_back(msg);
    bytes_in_flight_ += msg.sequence_length();
    if (bytes_in_flight_ == before) {
      break;
    }

    if (flight_empty == true) {
      ticks = 0;
    }
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // debug("unimplemented make_empty_message() called");
  // return {};
  TCPSenderMessage msg;
  msg.SYN = false;
  msg.FIN = false;
  if (writer().has_error() == true) {
    msg.RST = true;
  }
  msg.seqno = Wrap32::wrap(next_seqno_, isn_);
  return msg;
}

void TCPSender::receive(const TCPReceiverMessage& msg)
{
  // debug("unimplemented receive() called");
  // (void)msg;
  if (msg.RST == true) {
    writer().set_error();
    return;
  }
  windowSize_ = msg.window_size;
  if (msg.ackno.has_value()) {
    uint64_t ack_absno = msg.ackno.value().unwrap(isn_, next_seqno_);

    if (ack_absno > next_seqno_) {
      return;
    }

    while (outstanding_.empty() == false) {
      auto& seg = outstanding_.front();
      uint64_t end = seg.seqno.unwrap(isn_, next_seqno_) + seg.sequence_length();
      if (end <= ack_absno) {
        bytes_in_flight_ -= seg.sequence_length();
        outstanding_.pop_front();
        if (outstanding_.empty() == true) {
          ticks = 0;
        }
      }
      else {
        break;
      }
    }

    if (last_ack_absno_ < ack_absno) {
      rto_ = initial_RTO_ms_;
      consecutive_retransmissions_ = 0;
      last_ack_absno_ = ack_absno;
      if (outstanding_.empty() == false) {
        ticks = 0;
      }
    }
  }
}

void TCPSender::tick(uint64_t ms_since_last_tick, const TransmitFunction& transmit)
{
  ticks += ms_since_last_tick;
  // debug("unimplemented tick({}, ...) called", ms_since_last_tick);
  // (void)transmit;
  if (ticks >= rto_) {
    if (outstanding_.empty() == false) {
      auto& seg = outstanding_.front();
      transmit(seg);
      if (windowSize_ > 0) {
        consecutive_retransmissions_++;
        rto_ *= 2;
      }
      ticks = 0;
    }
  }
}
