#ifdef _MSC_VER 
#pragma once
#endif

#ifndef MODEL_MAP_HPP
#define MODEL_MAP_HPP

#include "forward_declarations.hpp"
#include "resmgr/resource_modification_listener.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is a map of all the currently loaded models.
 */
class ModelMap:
	public ResourceModificationListener
{
public:
	ModelMap();
	~ModelMap();

	void add( Model * pModel, const BW::StringRef & resourceID );
	void tryDestroy( const Model * pModel, bool isInModelMap );

	ModelPtr find( const BW::StringRef & resourceID );

	void findChildren(	const BW::StringRef & parentResID,
						BW::vector< ModelPtr > & children );

protected:
	void onResourceModified(
		const BW::StringRef& basePath,
		const BW::StringRef& resourceID,
		ResourceModificationListener::Action action);

private:
	typedef StringRefMap< Model * > Map;

	Map					map_;
	RecursiveMutex		mutex_;
};

BW_END_NAMESPACE

#endif // MODEL_MAP_HPP
