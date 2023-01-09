#ifndef __EVENT_HISTORY_STATS_HPP__
#define __EVENT_HISTORY_STATS_HPP__

#include "cstdmf/watcher.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used to track EventHistory usage. Statistics are exposed
 *	as watchers.
 */
class EventHistoryStats
{
	typedef BW::map< BW::string, uint32 > CountsMap;
	typedef MapWatcher< CountsMap > 		CountsWatcher;
	typedef BW::map< BW::string, uint32 > SizesMap;
	typedef MapWatcher< SizesMap > 			SizesWatcher;
	typedef BW::map< BW::string, uint32 > OversizesMap;
	typedef MapWatcher< SizesMap > 			OversizesWatcher;

	bool 			isEnabled_;
	CountsMap 		counts_;
	SizesMap		sizes_;
	OversizesMap	oversizes_;

public:
	EventHistoryStats() : isEnabled_( false ) {}

	void init( const BW::string & parentPath );

	/**
	 *	This function is called by functions dealing with EventHistory
	 *	to build up the statistics exposed by this class.
	 */
	void trackEvent( const BW::string& typeName,
			const BW::string& memberName, uint32 size, int16 streamSize )
	{
		if (isEnabled_)
		{
			BW::string key( typeName );
			key.reserve( typeName.size() + memberName.size() + 1 );
			key += '.';
			key += memberName;
			counts_[ key ]++;
			sizes_[ key ] += size;
			if (streamSize < 0 && streamSize > -4)
			{
				// See InterfaceElement::canHandleLength
				uint32 oversizeSize = ( 1 << ( -8 * streamSize ) ) - 1;
				if (size >= oversizeSize)
				{
					oversizes_[ key ]++;
				}
			}
		}
	}

	bool isEnabled() const	{ return isEnabled_; }
	void setEnabled( bool enable )
	{
		// Enabling the watcher has side effect of clearing stats. This is
		// probably a useful thing for debugging so that people can perform
		// different actions and look at how much it affects the amount (and
		// size) of events.
		if (enable)
		{
			counts_.clear();
			sizes_.clear();
			oversizes_.clear();
		}
		isEnabled_ = enable;
	}
};

BW_END_NAMESPACE

#endif // __EVENT_HISTORY_STATS_HPP__
