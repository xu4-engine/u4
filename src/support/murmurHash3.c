//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

/*
 * NOTE: This is MurmurHash3_x86_32() cleaned up for C99.  The original is at
 * https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
 */

#include <stdint.h>

//-----------------------------------------------------------------------------
// Platform-specific functions and macros

#define ROTL32(x,r) ((x << r) | (x >> (32 - r)))

//-----------------------------------------------------------------------------
// Block read - if your platform needs to do endian-swapping or can only
// handle aligned reads, do the conversion here

#ifdef UNALIGNED_U32
#define getblock32(ptrU32, i)   ptrU32[i]
#else
// This version works on all CPUs.
#define getblock32(bp, i)   (((uint32_t)bp[0])|bp[1]<<8|bp[2]<<16|bp[3]<<24)
#endif

//-----------------------------------------------------------------------------
// Finalization mix - force all bits of a hash block to avalanche

#define fmix32(h) \
  h ^= h >> 16; \
  h *= 0x85ebca6b; \
  h ^= h >> 13; \
  h *= 0xc2b2ae35; \
  h ^= h >> 16

//-----------------------------------------------------------------------------

uint32_t murmurHash3_32( const uint8_t* data, int len, uint32_t seed )
{
  const uint32_t c1 = 0xcc9e2d51;
  const uint32_t c2 = 0x1b873593;
  int nblocks = len / 4;
  int i;
  uint32_t h1 = seed;
  uint32_t k1;


  //----------
  // body

  const uint8_t* tail = data + nblocks*4;
#ifdef UNALIGNED_U32
  const uint32_t * blocks = (const uint32_t *) tail;
#else
  const uint8_t* blocks = data;
#endif

  for(i = -nblocks; i; i++)
  {
    k1 = getblock32(blocks,i);
#ifndef UNALIGNED_U32
    blocks += 4;
#endif

    k1 *= c1;
    k1 = ROTL32(k1,15);
    k1 *= c2;

    h1 ^= k1;
    h1 = ROTL32(h1,13);
    h1 = h1*5+0xe6546b64;
  }

  //----------
  // tail

  k1 = 0;

  switch(len & 3)
  {
  case 3: k1 ^= tail[2] << 16;
    // Fall through...
  case 2: k1 ^= tail[1] << 8;
    // Fall through...
  case 1: k1 ^= tail[0];
          k1 *= c1; k1 = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= len;

  fmix32(h1);

  return h1;
}
