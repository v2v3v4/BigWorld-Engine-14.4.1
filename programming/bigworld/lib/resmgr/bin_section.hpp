/**
 * An implementation of DataSection that provides a view onto a binary file.
 */

#ifndef _BIN_SECTION_HPP
#define _BIN_SECTION_HPP

#include "binary_block.hpp"
#include "datasection.hpp"

#include "cstdmf/debug.hpp"

BW_BEGIN_NAMESPACE

class IFileSystem;

/**
 * TODO: to be documented.
 */
class BinSection : public DataSection
{
public:
	BinSection( const BW::StringRef& tag, BinaryPtr pBinary );
	~BinSection();

	///	@name Section access functions.
	//@{
	int countChildren();
	DataSectionPtr openChild( int index );
	DataSectionPtr newSection( const BW::StringRef &tag,
		DataSectionCreator* creator=NULL );
	DataSectionPtr findChild( const BW::StringRef &tag,
		DataSectionCreator* creator=NULL );
	void delChild(const BW::StringRef &tag );
	void delChild(DataSectionPtr pSection);
	void delChildren();
	//@}

	/// This method returns the binary data owned by this section.
	BinaryPtr asBinary();

	/// This method sets the binary data of this section
	bool setBinary(BinaryPtr pBinary);

	///	This method returns the section's name.
	virtual BW::string sectionName() const;

	/// This method returns the number of bytes used by this section.
	virtual int bytes() const;

	/// This method saves the section
	virtual bool save( const BW::StringRef& filename = "" );

	/// This method sets the parent section associated with this section
	void setParent( DataSectionPtr pParent );

	virtual bool canPack() const	{ return false; }

	virtual DataSectionPtr convertToZip( const BW::StringRef& saveAs="", 
								DataSectionPtr pRoot=NULL,
								BW::vector<DataSectionPtr>* children=NULL );

	// creator
	static DataSectionCreator* creator();
private:
	void introspect();
	BinaryPtr recollect();

	BW::string		tag_;
	BinaryPtr		binaryData_;

	typedef BW::vector< DataSectionPtr > Children;

	bool			introspected_;
	Children		children_;

	DataSectionPtr	parent_;
};

typedef SmartPointer<BinSection> BinSectionPtr;

BW_END_NAMESPACE

#endif // _BIN_SECTION_HPP

