#include "cstdmf/bw_vector.hpp"
#include <queue>

#include <stdio.h>
#include <string.h>

#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug.hpp"
#include "bdiff.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

typedef unsigned char Byte;
typedef BW::vector<Byte> Bytes;
typedef unsigned short weak_checksum_type;
// 6 bytes to write a diff + 2 bytes to write an output header + 1 bytes saving = 9 bytes before
// we use some diff'd data
static const unsigned int BS = 9;

static weak_checksum_type 
	checksumBytes( const Bytes& buf, unsigned begin, unsigned length )
{
	MF_ASSERT_DEBUG( length >= sizeof(weak_checksum_type) );
	weak_checksum_type sum_a = 0;
	weak_checksum_type sum_b = 0;
	const unsigned end = begin+length;
	for ( unsigned i=begin; i<end; i++ )
	{
		sum_a += buf[i];
		sum_b += (end-i) * buf[i];
	}
	return ((sum_a & 0xff) | (sum_b << 8));
}

template <class T>
static void put( T x, Bytes& out )
{
	unsigned char * px = reinterpret_cast<unsigned char *>(&x);
	for (int i=0; i<sizeof(x); i++)
		out.push_back( px[i] );
}

bool performDiff( const Bytes& v1, const Bytes& v2, FILE * diff )
{
	if (v1.size() <= BS || v2.size() <= BS)
		return false;

	typedef BW::vector<unsigned> VecOfs;
	BW::vector<unsigned> m1[65536];
	for (unsigned i=0; i<v1.size()-BS; i++)
	{
		weak_checksum_type c = checksumBytes( v1, i, BS );
		m1[c].push_back(i);
	}

	unsigned lastMatch = 0;
	for (unsigned i=0; i<v2.size()-BS; i++)
	{
		weak_checksum_type c = checksumBytes( v2, i, BS );
		BW::vector<unsigned>& vec = m1[c];
		unsigned best1 = 0;
		unsigned bestLen = BS-1;
		bool found = false;
		for (VecOfs::const_iterator vi = vec.begin(); vi != vec.end(); ++vi )
		{
			unsigned j;
			unsigned m = std::min( v1.size()-*vi, v2.size()-i );
			if (m > 32767)
				m = 32767;
			if (m <= bestLen)
				continue;
			// only try to beat our present best
			if (v1[*vi]!=v2[i])
				continue;
			if (v1[*vi+bestLen] != v2[i+bestLen])
				continue;
			if (0 != memcmp(&v1[*vi+1], &v2[i+1], bestLen))
				continue;
			for (j=bestLen+1; j<m; j++)
			{
				if (v1[j+*vi] != v2[j+i])
					break;
			}
			if (j > bestLen)
			{
				best1 = *vi;
				bestLen = j;
				found = true;
				if (bestLen == 32767)
					break;
			}
		}
		if (found)
		{
//			INFO_MSG( "match (%d,%d)+%d (%d) opts=%d\n", best1, i, bestLen, i-lastMatch, vec.size() );
			unsigned short out;
			if (i-lastMatch)
			{
				out = i-lastMatch;
				if (1 != fwrite( &out, sizeof(out), 1, diff ))
					return false;
				for (unsigned j=lastMatch; j<i; j++)
				{
					if (EOF == fputc( v2[j], diff ))
						return false;
				}
			}
			i += bestLen;
			lastMatch = i;
			i--; // will be incremented next loop around
			out = bestLen | 0x8000;
			if (1 != fwrite( &out, sizeof(out), 1, diff ))
				return false;
			if (1 != fwrite( &best1, sizeof(best1), 1, diff ))
				return false;
			// check that we're still tiny :)
			if (ftell(diff) >= long(v2.size()))
				return false;
		}
		if (i-lastMatch == 32767)
		{
//			INFO_MSG( "write excess\n" );
			unsigned short out = 32767;
			if (1 != fwrite( &out, sizeof(out), 1, diff ))
				return false;
			for (unsigned j=lastMatch; j<i; j++)
			{
				if (EOF == fputc( v2[j], diff ))
					return false;
			}
			lastMatch = i+1;
			// check that we're still tiny :)
			if (ftell(diff) >= long(v2.size()))
				return false;
		}
	}
	if (lastMatch < v2.size())
	{
		unsigned short out = v2.size() - lastMatch;
		INFO_MSG( "write final %u (%" PRIzu ", %u)\n",
				(unsigned)v2.size() - lastMatch, v2.size(), lastMatch );
		if (1 != fwrite( &out, sizeof(out), 1, diff ))
			return false;
		for (unsigned j=lastMatch; j<v2.size(); j++)
		{
			if (EOF == fputc( v2[j], diff ))
				return false;
		}
	}

	return ftell(diff) < long(v2.size());
}

BW_END_NAMESPACE

// bdiff.cpp
