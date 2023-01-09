#ifndef HIERARCHICAL_CONFIG_HPP
#define HIERARCHICAL_CONFIG_HPP

#include "datasection.hpp"
#include "cstdmf/stringmap.hpp"

BW_BEGIN_NAMESPACE

class HierarchicalConfig
{
public:
	HierarchicalConfig();
	HierarchicalConfig( const DataSectionPtr pRootSection );
	/// Open a hierarchical config contained within a single file
	/// \param filename The name of the file containing the hierarchical config
	HierarchicalConfig( const StringRef & filename );
	/// Construct a hierarchical config from a directory structure
	/// \param filename The name of the file that is recursively searched for in every directory
	/// \param rootDirectory The root directory to recursively search. If empty the base resource paths will be used instead
	HierarchicalConfig( const StringRef & filename, 
						const StringRef & rootDirectory );

	void load( const DataSectionPtr pRootSection );
	void load( const StringRef & filename );
	void load( const StringRef & filename, 
			   const StringRef & rootDirectory );

	/// Returns the resolved config for the given path.
	/// \param path The path to resolve
	DataSectionPtr get( const StringRef & path ) const;

	DataSectionPtr getRoot() const { return pRootSection_; }

private:
	static void populateRule( DataSectionPtr pRule,
							  const DataSectionPtr pOverrides );
	static void populateChildren( const DataSectionPtr pSection, 
								  const StringRef & filename, 
								  const StringRef & directory,
								  const StringHashMap< DataSectionPtr > & namedRules );
	static void populateConfiguration( DataSectionPtr pConfiguration, 
									   const DataSectionPtr pSection, 
									   const StringRef & path );

private:
	DataSectionPtr pRootSection_;
};

BW_END_NAMESPACE

#endif // HIERARCHICAL_CONFIG_HPP
