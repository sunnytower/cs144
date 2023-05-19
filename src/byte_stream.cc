#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), buffer_{}, error_{}, eof_{}, bytes_pushed_{}, bytes_popped_{} {}

void Writer::push( string data )
{
  if (eof_) {
    return;
  }
  // Your code here.
  if (buffer_.size() + data.size() > capacity_) {
    bytes_pushed_ += capacity_ - buffer_.size();
    buffer_ += data.substr(0, capacity_ - buffer_.size());
  } else {
    bytes_pushed_ += data.size();
    buffer_ += data;
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
  return capacity_ - buffer_.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_pushed_;
}

string_view Reader::peek() const
{
  // Your code here.
  return string_view( buffer_ );
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
  if (len > buffer_.size()) {
    bytes_popped_ += buffer_.size();
    buffer_.erase(0, buffer_.size());
  } else {
    bytes_popped_ += len;
    buffer_.erase(0, len);
  }
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buffer_.size();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}
