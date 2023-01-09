#include "pch.hpp"

#include "resmgr/bwresource.hpp"

#include "cstdmf/log_msg.hpp"

BW_BEGIN_NAMESPACE

TEST( BWResource_InitArgCArgV )
{
	BWResource bwResource;
	int argc = 0;
	BWResource::init( argc, NULL );

	BWResource::fini();
	LogMsg::fini();
}

TEST( BWResource_InitArgCArgV2 )
{
	BWResource bwResource;
	int argc = 0;
	BWResource::init( argc, NULL );

	BWResource::fini();
	LogMsg::fini();
}


TEST( BWResource_InitArgCArgVTwice )
{
	BWResource * pResource = new BWResource();

	int argc = 0;
	BWResource::init( argc, NULL );

	delete pResource;
	BWResource::fini();

	pResource = new BWResource();
	BWResource::init( argc, NULL );
	delete pResource;
	BWResource::fini();

	LogMsg::fini();
}

BW_END_NAMESPACE

// test_bwresource.cpp
