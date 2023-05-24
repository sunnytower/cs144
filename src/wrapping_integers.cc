#include "wrapping_integers.hh"
#include <stdint.h>
using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return zero_point + n;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  const constexpr uint64_t high_32_bit = 0xFFFFFFFF00000000;
  uint64_t diff = this->raw_value_ - zero_point.raw_value_;
  if ( checkpoint > diff ) {
    uint64_t real_checkpoint = ( ( checkpoint - diff ) + ( 1UL << 31 ) ) & high_32_bit;
    return real_checkpoint + diff;
  } else {
    return diff;
  }
}
