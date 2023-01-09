#include "egcontroller.hpp"

#include "cellapp/cellapp.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

IMPLEMENT_CONTROLLER_TYPE( EgController, DOMAIN_REAL );

void EgController::startReal( bool isInitialStart )
{
	CellApp::instance().registerForUpdate( this );
}

void EgController::stopReal( bool isFinalStop )
{
	MF_VERIFY( CellApp::instance().deregisterForUpdate( this ) );
}

void EgController::update()
{
	TRACE_MSG( "EgController::update\n" );
}

BW_END_NAMESPACE

// egcontroller.cpp
