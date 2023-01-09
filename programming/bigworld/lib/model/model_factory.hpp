#ifdef _MSC_VER 
#pragma once
#endif

#ifndef MODEL_FACTORY_HPP
#define MODEL_FACTORY_HPP


#include "resmgr/forward_declarations.hpp"

#include "model.hpp"


BW_BEGIN_NAMESPACE

class NodefullModel;
class NodelessModel;


// This factory class is passed to Moo::createModelFromFile()
class ModelFactory
{
	const BW::StringRef& 	resourceID_;
	DataSectionPtr& 	pFile_;
public:
	typedef Model ModelBase;

	ModelFactory( const BW::StringRef& resourceID, DataSectionPtr& pFile );

	NodefullModel * newNodefullModel();

	NodelessModel * newNodelessModel();

	//Model* newBillboardModel();
};

BW_END_NAMESPACE

#endif // MODEL_FACTORY_HPP
