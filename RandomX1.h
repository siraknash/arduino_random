///////////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////

#ifndef	RANDOM__NUS__X_1__HEADER__FILE__
#define	RANDOM__NUS__X_1__HEADER__FILE__

#include	<stdint.h>
#include	<limits.h>
#include	<Arduino.h>


/*
 * RandomX1 provides a faster and more random pseudo-random number generator
 * than the native avr random(). Using the randomBits() function yields the
 * best performance and should be used if possible, but a random() function
 * matching the native interface is provided.
 *
 * randomBits() provides a 5x to 6x speed-up for average ranges of numbers
 * (100 - 1000).  The best case is when just getting a single bit, which has
 * a speed-up of 11x.  Using random() provides a 1.5x to 6x speed-up, for
 * average ranges of numbers, depending on the specific range of numbers
 * being requested.
 *
 */
class RandomX1
{
public:
	//
	// Constructor, seed initialized to 0.  randomSeed() can be called at a
	// later time to set (or re-set) random seed.
	//
	RandomX1(unsigned long seed = 0)
	{
		m_ctrlIndex = 0;
		m_bitCounts[0] = BITS_PER_NATIVE_RANDOM;
		m_bitCounts[1] = BITS_PER_NATIVE_RANDOM;
		m_bits[0] = ::random(LONG_MAX);
		m_bits[1] = ::random(LONG_MAX);

		::randomSeed(seed);
	}

	virtual ~RandomX1()
	{
	}

	//
	// randomSeed()
	//
	// Set a new seed.  This can be called at any time to modify the seed.
	// This is just a wrapper around the standard ::randomSeed() function.
	// With no argument.
	//
	void randomSeed(unsigned long seed)
	{
		::randomSeed(seed);
	}

	//
	// randomBits()
	//
	// Returns a random number between 0 (inclusive) and 2^bits (exclusive),
	// optionally modified by offset.
	//
	// When possible, this function should be used as it is considerably
	// than non-powers of 2.  Calling random() with values such as (64, 256,
	// etc...) is equivalent to calling this function wth (6, 8, etc...)
	//
	// NOTE: The maximum number of bits is limited to 20 (0-1048576). If
	//       absolutely necessary this can be safely increased up to a
	//       maximum of 31 by modifying MAXIMUM_BITS_PER_RANDOM_REQUEST in
	//		 this file.
	//
	// Arguments:
	//		bit_count, number of random bits to generate
	//		offset, value to be added to result, can be negative (optional)
	//
	// - When 'bit_count' == 0, 0 is always returned
	// - Any value of 'bit_count' greater than MAXIMUM_BITS_PER_RANDOM_REQUEST
	//		(default value: 20) returns as if MAXIMUM_BITS_PER_RANDOM_REQUEST
	//		was the 'bit_count' argument.
	//
	inline long randomBits(uint8_t bit_count, long offset = 0)
	{
		if(bit_count == 0) {
			return 0;
		}

		// Limit requested bit count
		if(bit_count > MAX_BITS_PER_RANDOM_REQUEST) {
			bit_count = MAX_BITS_PER_RANDOM_REQUEST;
		}

		m_ctrlIndex ^= 0x01;
		return this->_get_bits(bit_count) + offset;
	}

	//
	// random()
	//
	// Returns a random number between specified 'min_val' (inclusive) and
	// 'max_val' (exclusive).
	//
	// NOTE: The maximum range (max_val - min_val is limited to 1048576).
	//
	// Arguments:
	//		min_val, minimum value to return, inclusive (optional)
	//		max_val, maximum value to return, exclusive
	//
	// Arguments can be negative values.
	//
	inline long random(long min_val, long max_val)
	{
		long		diff, res;
		uint8_t		req_bits;

		diff = max_val - min_val - 1;

		if(diff <= 0) {
			return 0;
		}

		// Limit requested value range
		if(diff > MAX_VALUE_PER_RANDOM_REQUEST) {
			diff = MAX_VALUE_PER_RANDOM_REQUEST;
		}

		// Get the minimum number of bits needed
		req_bits = this->_get_required_bits(diff);

		res = this->randomBits(req_bits);

		// out of range
		if(res >= diff) {
			// randomBits advanced the control index, try again
			res = this->randomBits(req_bits);

			if(res >= diff) {
				// bits have already been removed by randomBits, so it's safe
				// to just peek and only remove the bits if they can be used
				if(!this->_peek_bits(req_bits, diff, res)) {
					if(res >= diff) {
						// advance index and peek at the other bits
						m_ctrlIndex ^= 0x01;

						if(!this->_peek_bits(req_bits, diff, res)) {
							// tried 2-4 times, give up and just use native interface
							res = ::random(diff);
						}
					}
				}
			}
		}

		return (res + min_val);
	}

	//
	// random(), without min value
	//
	inline long random(long max_val)
	{
		return this->random(0, max_val);
	}

private:
	//
	// _get_required_bits()
	//
	// Returns the minimum number of bits needed to represent 'num'
	//
	inline uint8_t _get_required_bits(long num)
	{
		// Required number of bits needed for values 1 --> 255 (log2(n))
		static const uint8_t REQUIRED_BITS[256] = {
			0x00, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03,
			0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
			0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
			0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
			0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
			0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
			0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
			0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
			0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
			0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
			0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
			0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
			0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
			0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
			0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
			0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
		};

		uint32_t	temp;
		uint8_t		ret;

		// If the result after shifting off 8 bits is zero, this number can
		// be represented by 8 bits...
		if((temp = (num >> 8)) == 0) {
			ret = REQUIRED_BITS[num];
		}
		// Need more than 8 bits, shift off another 8 bits and check if we've
		// found the required bits; if not, more bits are needed (more than 16),
		// so keep going.
		else if((num = (temp >> 8)) == 0) {
			ret = 8 + REQUIRED_BITS[temp];
		}
		// 17 --> 24 bits...
		else if((temp = (num >> 8)) == 0) {
			ret = 16 + REQUIRED_BITS[num];
		}
		// 25 --> 32 bits...
		else {
			ret = 24 + REQUIRED_BITS[temp];
		}

		return ret;
	}

	//
	// _peek_bits()
	//
	inline bool _peek_bits(uint8_t bit_count, long num, long &res)
	{
		// Only actually do peek if there's enough bits
		if(m_bitCounts[m_ctrlIndex] >= bit_count) {
			if((m_bits[m_ctrlIndex] & this->_get_mask(bit_count)) < num) {
				// can satisfy requenst, so get the bits and return true
				res = this->_get_bits(bit_count);
				return true;
			}
		}

		return false;
	}

	//
	// _get_bits()
	//
	inline long _get_bits(uint8_t bit_count)
	{
		long	ret = 0;

		// Check if bit_count needs more bits than currently available
		if(bit_count > m_bitCounts[m_ctrlIndex]) {
			// Take the available bits; used as high-order bits. 'bit_count' is
			// updated to the remaining number of bits needed.
			bit_count -= m_bitCounts[m_ctrlIndex];
			ret = (m_bits[m_ctrlIndex] << bit_count);

			// Generate new bits
			m_bits[m_ctrlIndex] = ::random(LONG_MAX);
			m_bitCounts[m_ctrlIndex] = BITS_PER_NATIVE_RANDOM;
		}

		ret |= (m_bits[m_ctrlIndex] & this->_get_mask(bit_count));

		// Get rid of bits that were just used
		m_bitCounts[m_ctrlIndex] -= bit_count;
		m_bits[m_ctrlIndex] >>= bit_count;
		return ret;
	}

	//
	// _get_mask()
	//
	// Returns mask to get a particular number of bits using bit-wise AND
	//
	inline long _get_mask(uint8_t bit_count)
	{
		static const long MASKS[32] = {
			0x00000000, 0x00000001, 0x00000003, 0x00000007,
			0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
			0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
			0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
			0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
			0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
			0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
			0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff
		};

		return MASKS[bit_count];
	}

private:
	uint32_t		m_bits[2];
	uint8_t			m_bitCounts[2];
	uint8_t			m_ctrlIndex;

public:
	static const uint8_t	MAX_BITS_PER_RANDOM_REQUEST = 20;
	static const uint8_t	MAX_VALUE_PER_RANDOM_REQUEST = ((1 << MAX_BITS_PER_RANDOM_REQUEST) - 1);

private:
	static const uint8_t	BITS_PER_NATIVE_RANDOM = 31;
};


#endif

