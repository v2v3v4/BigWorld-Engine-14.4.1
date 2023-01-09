//*********************************************************************
//* C_Base64 - a simple base64 encoder and decoder.
//*
//*     Copyright (c) 1999, Bob Withers - bwit@pobox.com
//*
//* This code may be freely used for any purpose, either personal
//* or commercial, provided the authors copyright notice remains
//* intact.
//*********************************************************************

#ifndef ScopedBool_HPP
#define ScopedBool_HPP

#include "bw_namespace.hpp"

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

#define BW_SCOPED_BOOL( target ) ScopedBool scopedBool_##target( target )

/**
 *  This class provides the means for setting a flag when a scope is entered/exited.
 */
class ScopedBool
{
public:
	ScopedBool( bool & target );
	~ScopedBool();

private:
	bool & target_;
};

#include "scoped_bool.ipp"

BW_END_NAMESPACE

#endif // ScopedBool_HPP
