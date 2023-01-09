#include "pch.hpp"

#include "member_description.hpp"

#include "cstdmf/debug_message_categories.hpp"
#include "cstdmf/md5.hpp"

#include "pyscript/script.hpp"

#include "script/script_object.hpp"


BW_BEGIN_NAMESPACE


/**
 *	This method parses a DataSection to initialise this object.
 *
 *	@param interfaceName 	The name of the entitydef interface that this
 *							member belongs to.
 *	@param componentName	The name of the component to which this member
 *							belongs (if any).
 *	@param pSection 		The data section of the entitydef member.
 *	@param isForClient		Whether this is a client-specific member.
 *	@param pNumLatestEventMembers
 *							This points to a count that is incremented if this
 *							member is a SendLatestOnly member.
 *
 *	@return true on success, false otherwise.
 */
bool MemberDescription::parse( const BW::string & interfaceName,
		DataSectionPtr pSection,
		bool isForClient,
		unsigned int * pNumLatestEventMembers )
{
	interfaceName_ = interfaceName;
	isForClient_ = isForClient;

#if 0
	if (componentName_ != "")
	{
		DEBUG_MSG( "Entity member %s.%s is supplied by component %s.\n",
				interfaceName.c_str(), pSection->sectionName().c_str(),
				componentName.c_str() );
	}
#endif

	if ((pNumLatestEventMembers != NULL) &&
			pSection->readBool( "SendLatestOnly", false ))
	{
		latestEventIndex_ = *pNumLatestEventMembers;
		++(*pNumLatestEventMembers);
	}

	isReliable_ = pSection->readBool( "IsReliable", isReliable_ );

	DataSectionPtr pVarLenHeaderSection =
		pSection->findChild( "VariableLengthHeaderSize" );

	if (pVarLenHeaderSection.exists())
	{
		if (!isForClient)
		{
			ERROR_MSG( "MemberDescription::parse: "
					"%s: VariableLengthHeaderSize is valid only on client "
					"methods and properties\n",
				this->toString().c_str() );
			return false;
		}

		varLenHeaderSize_ = pSection->readUInt( "VariableLengthHeaderSize",
			Mercury::DEFAULT_VARIABLE_LENGTH_HEADER_SIZE );

		if ((varLenHeaderSize_ == 0) || (varLenHeaderSize_ > 4))
		{
			ERROR_MSG( "MemberDescription::parse: "
					"Invalid VariableLengthHeaderSize for %s\n",
				this->toString().c_str() );
			return false;
		}

		BW::string warnLevelString = pVarLenHeaderSection->readString(
			"WarnLevel", "log" );

		if (!this->oversizeWarnLevelFromString( warnLevelString ))
		{
			ERROR_MSG( "MemberDescription::parse: "
					"Invalid VariableLengthHeaderSize/WarnLevel for %s\n",
				this->toString().c_str() );

			return false;
		}
	}

	return true;
}


/**
 *	This method checks whether the given payload length is oversize with
 *	respect to the configured variable length header size, and handles it
 *	appropriately for the configured warn level.
 *
 *
 *	@return 	false if an exception has been raised, true otherwise.
 */
bool MemberDescription::checkForOversizeLength( size_t length,
		EntityID entityID ) const
{
	const int streamSize = this->getServerToClientStreamSize();

	if (streamSize >= 0)
	{
		// Fixed size message within the non-sub-indexed range.
		MF_ASSERT( streamSize == static_cast< int >( length ) );
		return true;
	}

	const int variableLengthHeaderSize = -streamSize;
	MF_ASSERT( variableLengthHeaderSize ==
		static_cast<int>( this->getVarLenHeaderSize() ) );

	if ((variableLengthHeaderSize == 0) || (variableLengthHeaderSize > 4))
	{
		CRITICAL_MSG( "MemberDescription::checkForOversizeLength: "
				"Invalid variable length header size: %d\n",
			variableLengthHeaderSize );
		return true;
	}

	if (!Mercury::InterfaceElement::canHandleLength( static_cast<int>( length ),
			static_cast<int>( variableLengthHeaderSize ) ))
	{
		if (oversizeWarnLevel_ != OVERSIZE_SHOULD_RAISE_EXCEPTION)
		{
			// When raising an exception, the method is not sent.
#if ENABLE_WATCHERS
			stats_.countOversized( static_cast<int>( length ) );
#endif // ENABLE_WATCHERS
		}

		if (oversizeWarnLevel_ == OVERSIZE_NO_WARNING)
		{
			return true;
		}

		char entityBuffer[64];
		entityBuffer[0] = '\0';
		if (entityID != NULL_ENTITY_ID)
		{
			bw_snprintf( entityBuffer, sizeof(entityBuffer),
				" for entity %d", entityID );
		}

		WARNING_MSG( "MemberDescription::checkForOversizeLength: "
				"%s%s exceeds variable length size "
				"(%" PRIzu " bytes for %d-byte header, will result in "
				"%d-bytes to describe length)\n",
			this->toString().c_str(), entityBuffer, length,
			variableLengthHeaderSize,
			variableLengthHeaderSize + static_cast<int>( sizeof(uint32) ) );

		if (oversizeWarnLevel_ == OVERSIZE_SHOULD_PRINT_CALLSTACK)
		{
			Script::printStack();
		}
		else if (oversizeWarnLevel_ == OVERSIZE_SHOULD_RAISE_EXCEPTION)
		{
			PyErr_Format( PyExc_ValueError, "Oversized for %s "
					"(using %d-byte variable length header size)",
				this->toString().c_str(), variableLengthHeaderSize );

			return false;
		}
	}

	return true;
}


/**
 *	This method sets the latest event index for this description. Typically
 *	the parse method sets this value but this can be used to set the value for
 *	MemberDescription instances that do not have the 'parse()' method called.
 *	For example, detailedPosition.
 */
void MemberDescription::setLatestEventIndex( int index )
{
	MF_ASSERT( latestEventIndex_ == -1 );

	latestEventIndex_ = index;
}


/**
 *	This method returns the oversize warn level as a human-readable string.
 *	This method is used for watchers.
 */
BW::string MemberDescription::oversizeWarnLevelAsString() const
{
	switch (oversizeWarnLevel_)
	{
	case OVERSIZE_NO_WARNING:
		return "none";
	case OVERSIZE_SHOULD_LOG:
		return "log";
	case OVERSIZE_SHOULD_PRINT_CALLSTACK:
		return "callstack";
	case OVERSIZE_SHOULD_RAISE_EXCEPTION:
		return "exception";
	default:
		return "(invalid)";
	}
}


/**
*	Accessor for warning level for watchers.
*
*	Due to the watcher macros, warnLevel can't be const-ref.
*/
void MemberDescription::watcherOversizeWarnLevelFromString(
	   BW::string warnLevel )
{
   if (!isForClient_)
   {
	   ERROR_MSG( "MemberDescription::watcherOversizeWarnLevelFromString: "
			   "%s is not a client-side property or method\n",
		   this->toString().c_str() );
	   return;
   }

   if (!this->oversizeWarnLevelFromString( warnLevel ))
   {
	   ERROR_MSG( "MemberDescription::watcherOversizeWarnLevelFromString: "
			   "%s: Invalid warn level: \"%s\"\n",
		   this->toString().c_str(), warnLevel.c_str() );
   }
}


/**
 *	This method sets the oversize warn level from the given string.
 *
 *	@param warnLevel 	The warning level.
 *
 *	@return true if the warn level was valid, false otherwise.
 */
bool MemberDescription::oversizeWarnLevelFromString(
		const BW::string & warnLevel )
{
	if (warnLevel == "none")
	{
		oversizeWarnLevel_ = OVERSIZE_NO_WARNING;
	}
	else if (warnLevel == "log")
	{
		oversizeWarnLevel_ = OVERSIZE_SHOULD_LOG;
	}
	else if (warnLevel == "callstack")
	{
		oversizeWarnLevel_ = OVERSIZE_SHOULD_PRINT_CALLSTACK;
	}
	else if (warnLevel == "exception")
	{
		oversizeWarnLevel_ = OVERSIZE_SHOULD_RAISE_EXCEPTION;
	}
	else
	{
		return false;
	}

	return true;
}


/**
 *	This method should be called by subclasses to add member-level data to the
 *	digest.
 */
void MemberDescription::addToMD5( MD5 & md5 ) const
{
	if (isForClient_)
	{
		int streamSize = this->getServerToClientStreamSize();

		if ((streamSize < 0) &&
			(varLenHeaderSize_ != Mercury::DEFAULT_VARIABLE_LENGTH_HEADER_SIZE))
		{
			md5.append( &varLenHeaderSize_, sizeof( varLenHeaderSize_ ) );
		}
	}

	MF_ASSERT( name_.size() <= INT_MAX );
	md5.append( name_.c_str(), static_cast<int>(name_.size()) );
}


BW_END_NAMESPACE

// member_description.cpp
