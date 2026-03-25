#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  // debug( "unimplemented wrap( {}, {} ) called", n, zero_point.raw_value_ );
  return Wrap32( ( zero_point + n ).raw_value_ % ( 1UL << 32 ) );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  // debug("unimplemented unwrap( {}, {} ) called", zero_point.raw_value_, checkpoint);
  uint32_t offset = this->raw_value_ - zero_point.raw_value_;
  uint64_t base = checkpoint & ~( ( 1ULL << 32 ) - 1 );
  uint64_t candi3 = base + offset;
  uint64_t candi1 = candi3 + ( 1ULL << 32 );
  uint64_t candi2 = candi3 > ( 1ULL << 32 ) ? candi3 - ( 1ULL << 32 ) : candi3;
  uint64_t sec = ullabs( candi1 - checkpoint ) < ullabs( candi2 - checkpoint ) ? candi1 : candi2;
  uint64_t fir = ullabs( sec - checkpoint ) < ullabs( candi3 - checkpoint ) ? sec : candi3;
  return fir;
}
