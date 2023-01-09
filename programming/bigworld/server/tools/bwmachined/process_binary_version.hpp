#ifndef PROCESS_BINARY_VERSION_HPP
#define PROCESS_BINARY_VERSION_HPP

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE


/**
 *	Simple class for a representing a BigWorld version, for use as a key
 *	for ServerPlatform::VersionsProcesses.
 */
class ProcessBinaryVersion
{
public:

	/** Constructor.*/
	ProcessBinaryVersion( int major, int minor ) :
		major_( major ),
		minor_( minor )
	{}
		
	/** Return the major version number. */
	int major() const { return major_; }
	/** Return the minor version number. */
	int minor() const { return minor_; }

	/** Comparison operator. */
	bool operator<( const ProcessBinaryVersion & other ) const
	{
		return (major_ < other.major_) ||
			((major_ == other.major_) && (minor_ < other.minor_));
	}

	/** Returns a human-readable string for this version. */
	const BW::string str() const
	{
		BW::ostringstream stream;
		stream << major_ << "." << minor_;
		return stream.str();
	}

private:
	int major_;
	int minor_;
};


BW_END_NAMESPACE


#endif // PROCESS_BINARY_VERSION_HPP
