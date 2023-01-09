#ifndef BWCONFIG_HPP
#define BWCONFIG_HPP

#include "common.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/watcher.hpp"

#include "resmgr/datasection.hpp"

#include <queue>
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"

// For debugging configuration options types
#include <typeinfo>


BW_BEGIN_NAMESPACE

/**
 *	This namespace contains functions for getting values out of the
 *	configuration files.
 *
 *	The initial configuration files is server/bw.xml. This can chain other
 *	configuration files by adding a section "parentFile". If the value
 *	cannot be found in the initial configuration file, parent files are then
 *	checked.
 */
namespace BWConfig
{
	// Vector of pairs of file DataSections and corresponding filenames
	typedef std::pair< DataSectionPtr, BW::string > ConfigElement;
	typedef BW::vector< ConfigElement > Container;
	extern int shouldDebug;

	bool init( int argc, char * argv[] );
	bool hijack( Container & container, int argc, char * argv[] );

	bool isBad();

	template <class C>
		const char * typeOf( const C & c ) { return typeid(c).name(); }
	inline const char * typeOf( char * ) { return "String"; }
	inline const char * typeOf( BW::string ) { return "String"; }
	inline const char * typeOf( bool ) { return "Boolean"; }
	inline const char * typeOf( int ) { return "Integer"; }
	inline const char * typeOf( float ) { return "Float"; }

	void prettyPrint( const char * path,
			const BW::string & oldValue, const BW::string & newValue,
			const char * typeStr, bool isHit );

	template <class C>
	void printDebug( const char * path,
			const C & oldValue, C newValue, bool isHit )
	{
		BW::string oldValueStr = watcherValueToString( oldValue );
		BW::string newValueStr = watcherValueToString( newValue );

		prettyPrint( path, oldValueStr, newValueStr,
				typeOf( oldValue ), isHit );
	}

	/**
	 *	This function returns the data section in the configuration files
	 *	corresponding to the input path.
	 */
	DataSectionPtr getSection( const char * path,
		DataSectionPtr defaultValue = NULL );

	/**
	 *	This class iterates through all the configuration data sections
	 *	(chained through the ParentFile directive), and finds those sections
	 *	within the file data sections that match the given name.
	 *
	 *	@see BWConfig::beginSearchSections
	 *	@see BWConfig::endSearch.
	 */
	class SearchIterator
	{
	public:
		SearchIterator();
		SearchIterator( const BW::string & searchName,
				const Container & chain );
		SearchIterator( const SearchIterator & copy );

		/**
		 *	Destructor.
		 */
		~SearchIterator(){}

		DataSectionPtr operator*();
		SearchIterator & operator++();
		SearchIterator operator++( int );

		BW::string currentConfigPath() const { return currentConfigPath_; }

		/**
		 *	Equality operator for iterators.
		 *
		 *	@param other The other iterator to compare to.
		 */
		bool operator==( const SearchIterator & other ) const
		{
			return other.iter_ == iter_ &&
				other.chainTail_ == chainTail_;
		}

		/**
		 *	Inequality operator for iterators.
		 *
		 *	@param other The other iterator to compare to.
		 */
		bool operator!=( const SearchIterator & other ) const
		{
			return !this->operator==( other );
		}

		SearchIterator & operator=( const SearchIterator & other );

	private:
		void findNext();

	private:

		/// The current data section's search iterator.
		DataSection::SearchIterator iter_;

		typedef std::queue< ConfigElement > ConfigElementQueue;
		/// The rest of the configuration data sections to search through.
		ConfigElementQueue chainTail_;

		/// The section name to search for.
		BW::string searchName_;

		/// The path to the source of the bw.xml data section that is being
		/// iterated through.
		BW::string currentConfigPath_;
	};

	SearchIterator beginSearchSections( const char * sectionName );

	const SearchIterator & endSearch();

	/**
	 *	This function retrieves the name of all the children sections in the
	 * 	configuration files corresponding to the input path. It adds the
	 * 	children names to childrenNames.
	 */
	void getChildrenNames( BW::vector< BW::string >& childrenNames,
		const char * parentPath );

	/**
	 *	This function changes the input value to the value in the
	 *	configuration files corresponding to the input path. If the value is
	 *	not specified in the configuration files, the value is left
	 *	unchanged.
	 *
	 *	@return True if the value was found, otherwise false.
	 */
	template <class C>
		bool update( const char * path, C & value )
	{
		DataSectionPtr pSect = getSection( path );

		if (shouldDebug)
		{
			printDebug( path, value,
					pSect ? pSect->as( value ) : value, pSect );
		}

		if (pSect) value = pSect->as( value );

		return bool(pSect);
	}

	/**
	 *	This function returns the configuration value corresponding to the
	 *	input path. If this value is not specified in the configuration
	 *	files, the default value is returned.
	 */
	template <class C>
		C get( const char * path, const C & defaultValue )
	{
		DataSectionPtr pSect = getSection( path );

		if (shouldDebug)
		{
			printDebug( path, defaultValue,
				pSect ? pSect->as( defaultValue ) : defaultValue, pSect );
		}

		if (pSect)
		{
			return pSect->as( defaultValue );
		}

		return defaultValue;
	}


	BW::string get( const char * path, const char * defaultValue );

	inline
	BW::string get( const char * path )
	{
		return get( path, "" );
	}
};


/*
 *	This method is used to register the watcher nub with bwmachined. It also
 *	binds the watcher nub to an interface. This interface is specified by the
 *	CONFIG_PATH/monitoringInterface configuration option else by the root
 *	level monitoringInterface option else to the same interface as the nub.
 */
#define BW_REGISTER_WATCHER( ID, ABRV, CONFIG_PATH, DISPATCHER, ADDR )		\
{																			\
	BW::string interfaceName =												\
		BWConfig::get( CONFIG_PATH "/monitoringInterface",					\
				BWConfig::get( "monitoringInterface",						\
					ADDR.ipAsString() ) );									\
																			\
	WatcherNub::instance().registerWatcher(									\
			ID, ABRV, interfaceName.c_str(), 0 );							\
	WatcherNub::instance().attachTo( DISPATCHER );							\
}

BW_END_NAMESPACE

#endif // BWCONFIG_HPP
