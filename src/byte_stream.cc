#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity ), error_( false ), buffer_(), isClose_( false ), pushCount( 0 ), popCount( 0 )
{ buffer_.reserve( capacity_ ); }

void Writer::push( string data )
{
  uint64_t capa = available_capacity();
  if ( data.length() > capa ) {
    data = data.substr( 0, capa );
  }
  buffer_ += data;
  pushCount += data.size();
  return;
}

void Writer::close()
{ isClose_ = true; }

bool Writer::is_closed() const
{ return isClose_; }

uint64_t Writer::available_capacity() const
{ return capacity_ - buffer_.size(); }

uint64_t Writer::bytes_pushed() const
{ return pushCount; }

string_view Reader::peek() const
{ return string_view( buffer_.data(), buffer_.size() ); }

void Reader::pop( uint64_t len )
{
  if ( buffer_.size() == 0 ) {
    return;
  }
  len = min( len, buffer_.size() );
  buffer_.erase( 0, len );
  popCount += len;
  return;
}

bool Reader::is_finished() const
{ return isClose_ == true && buffer_.size() == 0; }

uint64_t Reader::bytes_buffered() const
{ return buffer_.size(); }

uint64_t Reader::bytes_popped() const
{ return popCount; }
