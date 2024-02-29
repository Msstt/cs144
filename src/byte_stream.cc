#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return writer_closed_;
}

void Writer::push( string data )
{
  for ( auto& ch : data ) {
    if ( buffer_.size() == capacity_ ) {
      break;
    }
    buffer_.push_back( ch );
    pushed_count_++;
  }
}

void Writer::close()
{
  writer_closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_.size();
}

uint64_t Writer::bytes_pushed() const
{
  return pushed_count_;
}

bool Reader::is_finished() const
{
  return writer_closed_ && buffer_.empty();
}

uint64_t Reader::bytes_popped() const
{
  return popped_count_;
}

string_view Reader::peek() const
{
  return string_view( &buffer_.front(), 1 );
}

void Reader::pop( uint64_t len )
{
  while ( len > 0 && !buffer_.empty() ) {
    buffer_.pop_front();
    len--;
    popped_count_++;
  }
}

uint64_t Reader::bytes_buffered() const
{
  return buffer_.size();
}
