#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST == true ) {
    reader().set_error();
    return;
  }
  if ( message.SYN == false && isn_.has_value() == false ) {
    return;
  }

  // Your code here.
  // debug( "unimplemented receive() called" );
  if ( message.SYN == true && isn_.has_value() == false ) {
    isn_.emplace( message.seqno );
  }

  if ( message.payload.empty() && !message.FIN ) {
    return;
  }

  uint64_t check_point = reassembler().get_next_index();
  uint64_t abs_seqno = message.seqno.unwrap( isn_.value(), check_point );

  uint64_t stream_first_index = abs_seqno - 1;

  if ( message.SYN ) {
    stream_first_index = 0;
  }
  reassembler_.insert( stream_first_index, message.payload, message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  // debug("unimplemented send() called");
  TCPReceiverMessage msg;
  if ( reader().has_error() ) {
    msg.RST = true;
  }
  if ( isn_.has_value() ) {
    uint64_t next = reassembler().get_next_index();
    uint64_t ack = next + 1;
    if ( writer().is_closed() ) {
      ack += 1;
    }
    msg.ackno = Wrap32::wrap( ack, isn_.value() );
  }
  msg.window_size = min<uint64_t>( writer().available_capacity(), UINT16_MAX );
  return msg;
}
