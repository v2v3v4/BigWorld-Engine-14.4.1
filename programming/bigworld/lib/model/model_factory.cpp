#include "pch.hpp"

#include "model_factory.hpp"

#include "resmgr/datasection.hpp"

#include "nodefull_model.hpp"
#include "nodeless_model.hpp"



DECLARE_DEBUG_COMPONENT2( "Model", 0 )


BW_BEGIN_NAMESPACE

ModelFactory::ModelFactory(	const BW::StringRef& resourceID,
							DataSectionPtr& pFile ) :
		resourceID_( resourceID ),
		pFile_( pFile )
{
	BW_GUARD;
}


NodefullModel * ModelFactory::newNodefullModel()
{
	BW_GUARD;
	NodefullModel * model = new NodefullModel( resourceID_, pFile_ );

	if (model && model->valid())
		return model;
	else
		return NULL;
}

NodelessModel * ModelFactory::newNodelessModel()
{
	BW_GUARD;
	return new NodelessModel( resourceID_, pFile_ );
}


//Model * ModelFactory::newBillboardModel()
//{
//	return new BillboardModel( resourceID_, pFile_ );
//}

BW_END_NAMESPACE

// model_factory.cpp
