
//*********************************************************************
//* Base64 - a simple base64 encoder and decoder.
//*
//*     Copyright (c) 1999, Bob Withers - bwit@pobox.com
//*
//* This code may be freely used for any purpose, either personal
//* or commercial, provided the authors copyright notice remains
//* intact.
//*
//* Enhancements by Stanley Yamane:
//*     o reverse lookup table for the decode function
//*     o reserve string buffer space in advance
//*
//*********************************************************************
#include "pch.hpp"
#include "base64.h"

#include "stdmf.hpp"
#include "debug.hpp"

DECLARE_DEBUG_COMPONENT2( "CStdMF", 0 )

BW_BEGIN_NAMESPACE

static const char          fillchar = '=';
static const BW::string::size_type np = BW::string::npos;
static const char			npc = (char) np;

                               // 0000000000111111111122222222223333333333444444444455555555556666
                               // 0123456789012345678901234567890123456789012345678901234567890123
namespace
{
BW::string Base64Table("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");


// Decode Table gives the index of any valid base64 character in the Base64 table]
// 65 == A, 97 == a, 48 == 0, 43 == +, 47 == /

                                               // 0  1  2  3  4  5  6  7  8  9
BW::string::size_type DecodeTable[] = {np,np,np,np,np,np,np,np,np,np,  // 0 - 9
                                                 np,np,np,np,np,np,np,np,np,np,  //10 -19
                                                 np,np,np,np,np,np,np,np,np,np,  //20 -29
                                                 np,np,np,np,np,np,np,np,np,np,  //30 -39
                                                 np,np,np,62,np,np,np,63,52,53,  //40 -49
                                                 54,55,56,57,58,59,60,61,np,np,  //50 -59
                                                 np,np,np,np,np, 0, 1, 2, 3, 4,  //60 -69
                                                  5, 6, 7, 8, 9,10,11,12,13,14,  //70 -79
                                                 15,16,17,18,19,20,21,22,23,24,  //80 -89
                                                 25,np,np,np,np,np,np,26,27,28,  //90 -99
                                                 29,30,31,32,33,34,35,36,37,38,  //100 -109
                                                 39,40,41,42,43,44,45,46,47,48,  //110 -119
                                                 49,50,51,np,np,np,np,np,np,np,  //120 -129
                                                 np,np,np,np,np,np,np,np,np,np,  //130 -139
                                                 np,np,np,np,np,np,np,np,np,np,  //140 -149
                                                 np,np,np,np,np,np,np,np,np,np,  //150 -159
                                                 np,np,np,np,np,np,np,np,np,np,  //160 -169
                                                 np,np,np,np,np,np,np,np,np,np,  //170 -179
                                                 np,np,np,np,np,np,np,np,np,np,  //180 -189
                                                 np,np,np,np,np,np,np,np,np,np,  //190 -199
                                                 np,np,np,np,np,np,np,np,np,np,  //200 -209
                                                 np,np,np,np,np,np,np,np,np,np,  //210 -219
                                                 np,np,np,np,np,np,np,np,np,np,  //220 -229
                                                 np,np,np,np,np,np,np,np,np,np,  //230 -239
                                                 np,np,np,np,np,np,np,np,np,np,  //240 -249
                                                 np,np,np,np,np,np               //250 -256
												};
}; // anonymous namespace


BW::string Base64::encode(const char* data, size_t len)
{
    uint		i;
    char		c;
	BW::string	ret;

	ret.reserve( len * 4 / 3 + 3 );

    for (i = 0; i < len; ++i)
    {
        c = (data[i] >> 2) & 0x3f;
        ret.push_back( Base64Table[c] );
        c = (data[i] << 4) & 0x3f;
        if (++i < len)
            c |= (data[i] >> 4) & 0x0f;

        ret.push_back( Base64Table[c] );
        if (i < len)
        {
            c = (data[i] << 2) & 0x3f;
            if (++i < len)
                c |= (data[i] >> 6) & 0x03;

            ret.push_back( Base64Table[c] );
        }
        else
        {
            ++i;
            ret.push_back( fillchar );
        }

        if (i < len)
        {
            c = data[i] & 0x3f;
            ret.push_back( Base64Table[c] );
        }
        else
        {
            ret.push_back( fillchar );
        }
    }

    return(ret);
}

// Returns the length of the decoded string on success, otherwise -1.
int Base64::decode( const BW::string& data, char* results, size_t bufSize )
{
	// TODO: Deprecate this method or make it ore fault tolerant.
    size_t			i;
    char			c;
    char			c1;
	size_t			len = data.length();
	int				d = 0;	//destination index

	if ( len > bufSize )
	{
		ERROR_MSG( "Base64::decode - destination buffer not large enough\n" );
		return -1;
	}

    for (i = 0; i < len; ++i)
    {
        c = (char) DecodeTable[(unsigned char)data[i]];
        ++i;
        c1 = (char) DecodeTable[(unsigned char)data[i]];
        c = (c << 2) | ((c1 >> 4) & 0x3);
        results[d++]=c;
        if (++i < len)
        {
            c = data[i];
            if (fillchar == c)
                break;

            c = (char) DecodeTable[(unsigned char)data[i]];
            c1 = ((c1 << 4) & 0xf0) | ((c >> 2) & 0xf);
            results[d++]=c1;
        }

        if (++i < len)
        {
            c1 = data[i];
            if (fillchar == c1)
                break;

            c1 = (char) DecodeTable[(unsigned char)data[i]];
            c = ((c << 6) & 0xc0) | c1;
            results[d++]=c;
        }
    }

    return d;
}


/**
 *	This method decodes the base64 string in inData and puts the result in
 *	outData. On failure, outData may be in a bad state.
 *
 *	@return True on success, otherwise false.
 */
bool Base64::decode( const BW::string& inData, BW::string & outData )
{
	if (inData.size() % 4 != 0)
	{
		return false;
	}

	outData.clear();
	outData.reserve( (inData.size() + 3)/4 * 3);

    size_t			i;
    char			c;
    char			c1;
	size_t			len = inData.size();

    for (i = 0; i < len; ++i)
    {
        c = (char) DecodeTable[(unsigned char)inData[i]];
		if (c == npc) return false;
        ++i;
		if (i == len) return false;
        c1 = (char) DecodeTable[(unsigned char)inData[i]];
		if (c1 == npc) return false;
        c = (c << 2) | ((c1 >> 4) & 0x3);
		outData.push_back( c );
        if (++i < len)
        {
            c = inData[i];
            if (fillchar == c)
			{
				// TODO: Could check that the extra bits are 0.
                break;
			}

            c = (char) DecodeTable[(unsigned char)c];
			if (c == npc) return false;
            c1 = ((c1 << 4) & 0xf0) | ((c >> 2) & 0xf);
            outData.push_back( c1 );
        }

        if (++i < len)
        {
            c1 = inData[i];
            if (fillchar == c1)
			{
				// TODO: Could check that the extra bits are 0.
                break;
			}

            c1 = (char) DecodeTable[(unsigned char)c1];
			if (c1 == npc) return false;
            c = ((c << 6) & 0xc0) | c1;
            outData.push_back( c );
        }
    }

    return true;
}

BW_END_NAMESPACE
