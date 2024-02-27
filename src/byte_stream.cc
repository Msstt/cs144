#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return writer_closed;
}

void Writer::push( string data )
{
  for ( auto& ch : data ) {
    if ( buffer.size() == capacity_ ) {
      break;
    }
    buffer.push_back( ch );
    pushed_count++;
  }
}

void Writer::close()
{
  writer_closed = true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer.size();
}

uint64_t Writer::bytes_pushed() const
{
  return pushed_count;
}

bool Reader::is_finished() const
{
  return writer_closed && buffer.empty();
}

uint64_t Reader::bytes_popped() const
{
  return popped_count;
}

string_view Reader::peek() const
{
  return string_view( &buffer.front(), 1 );
}

void Reader::pop( uint64_t len )
{
  while ( len > 0 && !buffer.empty() ) {
    buffer.pop_front();
    len--;
    popped_count++;
  }
}

uint64_t Reader::bytes_buffered() const
{
  return buffer.size();
}
