//*********************************************************************
//* C_Base64 - a simple base64 encoder and decoder.
//*
//*     Copyright (c) 1999, Bob Withers - bwit@pobox.com
//*
//* This code may be freely used for any purpose, either personal
//* or commercial, provided the authors copyright notice remains
//* intact.
//*********************************************************************

#ifndef Base64_H
#define Base64_H

#include "bw_namespace.hpp"

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This class provides encoding/decoding of Base64 streams.
 */
class CSTDMF_DLL Base64
{
public:
	static BW::string encode(const char* data, size_t len);
	static BW::string encode( const BW::string & data )
	{
		return encode( data.data(), data.size() );
	}

	static int decode(const BW::string & data, char* results, size_t bufSize);
	static bool decode( const BW::string & inData, BW::string & outData );
};

BW_END_NAMESPACE

#endif // Base64_H
