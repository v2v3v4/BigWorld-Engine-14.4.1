#ifndef ASSET_PIPELINE_DEPENDENCY
#define ASSET_PIPELINE_DEPENDENCY

#include "cstdmf/bw_namespace.hpp"
#include "resmgr/datasection.hpp"

// Do not alter the order of this list.
// It is used to construct an enum for serialisation.
// Add new dependency types to the end of the list only.
#define DEPENDENCY_TYPES			\
	X( SourceFileDependency )		\
	X( IntermediateFileDependency )	\
	X( OutputFileDependency )		\
	X( ConverterDependency )		\
	X( ConverterParamsDependency )	\
	X( DirectoryDependency )

BW_BEGIN_NAMESPACE

#define X( type ) type##Type,
	enum DependencyType { DEPENDENCY_TYPES InvalidDependencyType };
#undef X

/// \brief Dependency base class
///
/// Base class for all dependency types.
class Dependency
{
public:
	Dependency() : critical_( false ) {}
	virtual ~Dependency() {}

	/// Get the type of this dependency.
	/// \return the type of this dependency.
	virtual DependencyType getType() const = 0;

	/// Set this dependency to be critical.
	/// If a dependency is critical it will fail an asset conversion
	/// if there is error processing this dependency.
	/// \param critical true to make this dependency critical
	///                 false to make this dependency non critical.
	void setCritical( bool critical ) { critical_ = critical; }
	/// Is this dependency critical.
	/// If a dependency is critical it will fail an asset conversion
	/// if there is error processing this dependency.
	/// \return true if this dependency critical
	///         false if this dependency non critical.
	bool isCritical() const { return critical_; }

	/// Serialise the dependency in from a data section.
	/// If overriding this in a derived class, be sure to call the
	/// base implementation at some point within your implementation
	/// \param pSection the data section to serialise in from.
	virtual bool serialiseIn( DataSectionPtr pSection );
	/// Serialise the dependency out to a data section.
	/// If overriding this in a derived class, be sure to call the
	/// base implementation at some point within your implementation
	/// \param pSection the data section to serialise out to.
	virtual bool serialiseOut( DataSectionPtr pSection ) const;

protected:
	bool critical_;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_DEPENDENCY
