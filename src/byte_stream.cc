#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), buffer_{}, error_{}, eof_{}, bytes_pushed_{}, bytes_popped_{} {}

void Writer::push( string data )
{
  if ( !eof_ && !error_ ) {
    const auto data_size = std::min( available_capacity(), data.size()  );
    if ( data_size > 0 ) { /* only push the dataset for a positive to_push size*/
      /* only push the data string with upper bounds to_push sizes*/
      buffer_.push_back( data.substr( 0, data_size ) );
      bytes_pushed_ += data_size;
    }
  }
}

void Writer::close()
{
  // Your code here.
  eof_ = true;
}

void Writer::set_error()
{
  // Your code here.
  error_ = true;
}

bool Writer::is_closed() const
{
  // Your code here.
  return eof_;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - bytes_pushed_ + bytes_popped_;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_pushed_;
}

string_view Reader::peek() const
{
  // Your code here.
  return string_view (buffer_.front().data(), buffer_.front().size());
}

bool Reader::is_finished() const
{
  // Your code here.
  return eof_ && buffer_.empty();
}

bool Reader::has_error() const
{
  // Your code here.
  return error_;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  while (len > 0 && !buffer_.empty()) {
    const auto data_size = min( len, buffer_.front().size() );
    buffer_.front().erase( 0, data_size );
    bytes_popped_ += data_size;
    len -= data_size;
    if (buffer_.front().empty()) {
      buffer_.pop_front();
    }
  }
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return bytes_pushed_ - bytes_popped_;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}
