#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32 { zero_point + n };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint32_t dist = raw_value_ - zero_point.raw_value_ - checkpoint;
  uint64_t result = checkpoint + dist;
  if (dist > (1u << 31) && result >= (1ul << 32)) {
    result -= (1ul << 32);
  }
  return result;
}