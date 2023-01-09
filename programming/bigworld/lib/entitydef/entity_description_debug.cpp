#include "pch.hpp"

#include "entity_description_debug.hpp"
#include "entity_description_map.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "DataDescription", 0 )


BW_BEGIN_NAMESPACE

namespace EntityDescriptionDebug
{
void dump( const MethodDescription & mDesc, int detailLevel )
{
	char exposedSubMsg[16] = {'\0'};
	if (mDesc.hasSubMsgID())
	{
		bw_snprintf( exposedSubMsg, sizeof(exposedSubMsg),
			"+%d", mDesc.exposedSubMsgID() );
	}
	
	const int streamSize = mDesc.streamSize( false );
	DEBUG_MSG( "\t\t%2d%s/%2d %s - %d byte%s%s\n",
		mDesc.exposedMsgID(), exposedSubMsg,
		mDesc.internalIndex(), mDesc.name().c_str(),
		abs(streamSize), (abs(streamSize) == 1 ? "":"s"),
		streamSize < 0 ? " used for length" : "" );
}


/**
 *	This function dumps out information about the input EntityDescription.
 */
void dump( const EntityDescription & eDesc, int detailLevel )
{
	DEBUG_MSG( "%s\n", eDesc.name().c_str() );
	DEBUG_MSG( "hasScript: Client %d, Base %d, Cell %d)\n",
			eDesc.canBeOnClient(),
			eDesc.canBeOnBase(),
			eDesc.canBeOnCell() );

	DEBUG_MSG( "\tProperties:\n" );
	for (uint i = 0; i < eDesc.propertyCount(); ++i)
	{
		DataDescription * pDD = eDesc.property( i );
		const int exposedIndex = pDD->clientServerFullIndex();
		const int streamSize = pDD->streamSize();

		if (detailLevel > 2 && streamSize < 0)
		{
			DEBUG_MSG( "\t\t%2d/%2d %-12s %-8s %3d %s%s%s - %d byte%s "
					"used for length\n",
				exposedIndex,
				pDD->index(),
				pDD->name().c_str(),
				pDD->dataType()->typeName().c_str(),
				pDD->detailLevel(),
				pDD->getDataFlagsAsStr(),
				pDD->isPersistent() ? " DB" : "",
				pDD->isIdentifier() ? " ID" : 
					pDD->isUnique() ? " UQ" :
					pDD->isIndexed() ? " IX" :
					"",
				-streamSize, (-streamSize == 1 ? "":"s") );
		}
		else if (detailLevel > 2)
		{
			DEBUG_MSG( "\t\t%2d/%2d %-12s %-8s %3d %s%s%s - %d byte%s\n",
				exposedIndex,
				pDD->index(),
				pDD->name().c_str(),
				pDD->dataType()->typeName().c_str(),
				pDD->detailLevel(),
				pDD->getDataFlagsAsStr(),
				pDD->isPersistent() ? " DB" : "",
				pDD->isIdentifier() ? " ID" : 
					pDD->isUnique() ? " UQ" :
					pDD->isIndexed() ? " IX" :
					"",
				streamSize, (streamSize == 1 ? "":"s") );
		}
		else if (streamSize < 0)
		{
			DEBUG_MSG( "\t\t%2d/%2d %-12s %-8s - %d byte%s "
					"used for length\n",
				exposedIndex,
				pDD->index(),
				pDD->name().c_str(),
				pDD->dataType()->typeName().c_str(),
				-streamSize, (-streamSize == 1 ? "":"s") );
		}
		else
		{
			DEBUG_MSG( "\t\t%2d/%2d %-12s %-8s - %d byte%s\n",
				exposedIndex,
				pDD->index(),
				pDD->name().c_str(),
				pDD->dataType()->typeName().c_str(),
				streamSize, (streamSize == 1 ? "":"s") );
		}
	}

	if (eDesc.clientMethodCount())
	{
		DEBUG_MSG( "\tClient Methods:\n" );
		for (uint i = 0; i < eDesc.clientMethodCount(); ++i)
		{
			dump( *eDesc.client().internalMethod( i ), detailLevel );
		}
	}

	if (eDesc.base().size())
	{
		DEBUG_MSG( "\tBase Methods:\n" );
		for (uint i = 0; i < eDesc.base().size(); ++i)
		{
			dump( *eDesc.base().internalMethod( i ), detailLevel );
		}
	}

	if (eDesc.cell().size())
	{
		DEBUG_MSG( "\tCell Methods:\n" );
		for (uint i = 0; i < eDesc.cell().size(); ++i)
		{
			dump( *eDesc.cell().internalMethod( i ), detailLevel );
		}
	}

	if (detailLevel > 2)
	{
		DEBUG_MSG( "\tVolatile Info:\n" );
		DEBUG_MSG( "\t\tposition %f\n", eDesc.volatileInfo().positionPriority() );
		DEBUG_MSG( "\t\tyaw %f\n", eDesc.volatileInfo().yawPriority() );
		DEBUG_MSG( "\t\tpitch %f\n", eDesc.volatileInfo().pitchPriority() );
		DEBUG_MSG( "\t\troll %f\n", eDesc.volatileInfo().rollPriority() );

		DEBUG_MSG( "\tLoD Levels (%d):\n", eDesc.lodLevels().size() );

		for (int i = 0; i < eDesc.lodLevels().size(); ++i)
		{
			const DataLoDLevel & level = eDesc.lodLevels().getLevel( i );
			DEBUG_MSG( "\t\t%10s %10.0f %10.0f %10.0f %10.0f\n",
					level.label().c_str(),
					isEqual( level.start(), FLT_MAX ) ? 99999 : level.start(),
					isEqual( level.hyst(), FLT_MAX ) ? 99999 : level.hyst(),
					isEqual( level.low(), FLT_MAX ) ? 99999 : level.low(),
					isEqual( level.high(), FLT_MAX ) ? 99999 : level.high() );
		}
	}
}


/**
 *	This function dumps out information about the input EntityDescriptionMap.
 */
void dump( const EntityDescriptionMap & map, int detailLevel )
{
	if (detailLevel < 1) return;

	int numTypes = map.size();
	DEBUG_MSG( "Num types = %d\n", numTypes );

	DEBUG_MSG( "ID	: %-15sExposed/Not Exp.  Client,  Base,  Cell\n",
			"Name");

	for (EntityTypeID typeIndex = 0; typeIndex < numTypes; ++typeIndex)
	{
		const EntityDescription & eDesc = map.entityDescription( typeIndex );

		DEBUG_MSG( "%2d(%2d): %-15s Props %2d/%2d. Methods %2d, %2d/%-2d, %2d/%-2d\n",
			eDesc.index(),
			eDesc.clientIndex(),
			eDesc.name().c_str(),
			eDesc.clientServerPropertyCount(),
			eDesc.propertyCount(),
			eDesc.clientMethodCount(),
			eDesc.base().exposedSize(),
			eDesc.base().size(),
			eDesc.cell().exposedSize(),
			eDesc.cell().size() );
	}

	if (detailLevel < 2) return;

	for (EntityTypeID typeIndex = 0; typeIndex < numTypes; ++typeIndex)
	{
		const EntityDescription & eDesc = map.entityDescription( typeIndex );
		dump( eDesc, detailLevel );
	}
}

} // namespace EntityDescriptionDebug

BW_END_NAMESPACE

// entity_description_debug.cpp
