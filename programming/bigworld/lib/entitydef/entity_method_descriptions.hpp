#ifndef ENTITY_METHOD_DESCRIPTIONS_HPP
#define ENTITY_METHOD_DESCRIPTIONS_HPP

#include "method_description.hpp"

#include "cstdmf/watcher.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/stringmap.hpp"

#include "bwentity/bwentity_api.hpp"

BW_BEGIN_NAMESPACE

class MethodDescription;

typedef BW::vector< MethodDescription >	MethodDescriptionList;

/**
 *	This class is used to store descriptions of the methods of an entity.
 */
class EntityMethodDescriptions
{
public:
	EntityMethodDescriptions();
	bool init( DataSectionPtr pMethods,
		MethodDescription::Component component,
		const char * interfaceName,
		const BW::string & componentName,
		unsigned int * pNumLatestEventMembers = NULL );
	void supersede();

	BWENTITY_API unsigned int size() const;
	BWENTITY_API
	unsigned int exposedSize() const { return int( exposedMethods_.size() ); }

	BWENTITY_API MethodDescription * internalMethod( unsigned int index ) const;
	BWENTITY_API
	const MethodDescription * exposedMethod( unsigned int index ) const;
	const MethodDescription * exposedMethodFromMsgID( Mercury::MessageID msgID,
		BinaryIStream & data, const ExposedMethodMessageRange & range ) const;
	BWENTITY_API MethodDescription * find( const char * name ) const;

	MethodDescriptionList & internalMethods()	{ return internalMethods_; }
	const MethodDescriptionList & internalMethods() const	{ return internalMethods_; }

	void setExposedMsgIDs( int maxExposedMethodCount,
		const ExposedMethodMessageRange * pRange );

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif

private:
	bool checkExposedForClientSafety( const char * interfaceName );

	typedef StringMap< uint32 > Map;
	typedef MethodDescriptionList	List;

	Map		map_;
	List	internalMethods_;

	BW::vector< unsigned int >	exposedMethods_;

	int maxExposedMethodCount_;
};

BW_END_NAMESPACE

#endif // ENTITY_METHOD_DESCRIPTIONS_HPP
