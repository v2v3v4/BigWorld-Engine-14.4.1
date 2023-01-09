#ifndef _ZIP_SECTION_HPP
#define _ZIP_SECTION_HPP

#include "file_system.hpp"
#include "dir_section.hpp"

BW_BEGIN_NAMESPACE

class DataSectionCreator;
class ZipFileSystem;
typedef SmartPointer<ZipFileSystem> ZipFileSystemPtr;

/**
 * Data section representing a zip file archive.
 */
class ZipSection : public DataSection
{
public:
	ZipSection( const BW::StringRef& fullPath, FileSystemPtr parentSystem,
		BW::vector<DataSectionPtr>* pChildren = NULL);
	ZipSection( ZipFileSystemPtr parentZip, const BW::StringRef& tag,
		BW::vector<DataSectionPtr>* pChildren = NULL );
	~ZipSection();

	///	@name Section access functions.
	//@{
	int countChildren();
	DataSectionPtr openChild( int index );
	BW::string childSectionName( int index );
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
					BW::vector<DataSectionPtr>* children=NULL);

	virtual bool saveChild( DataSectionPtr pChild, bool isBinary );	

	void tag(const BW::StringRef& tag) { tag_.assign( tag.data(), tag.size() ); }
	const BW::string& tag() const { return tag_; }
	BW::string absolutePath();

	// creator
	static DataSectionCreator* creator();
private:
	typedef BW::vector< DataSectionPtr > Children;

	void introspect();
	BinaryPtr recollect();
	

	BW::string			tag_;
	bool				introspected_;
	mutable Children	children_;

	DataSectionPtr		parent_;
	ZipFileSystemPtr	pFileSystem_;
};
typedef SmartPointer<ZipSection> ZipSectionPtr;

BW_END_NAMESPACE

#endif //_ZIP_SECTION_HPP

