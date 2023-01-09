#ifndef _DIR_SECTION_HPP
#define _DIR_SECTION_HPP

#include "file_system.hpp"

BW_BEGIN_NAMESPACE

class DataSectionCache;

/**
 * An implementation of DataSection that provides a view onto a directory tree node.
 */
class DirSection : public DataSection
{
public:
	DirSection(const BW::StringRef& path, IFileSystem* pFileSystem);
	~DirSection();

	///	@name virtual section access functions
	//@{
	/// This methods returns the number of sections under this one.
	int countChildren();

	/// This method opens a section with the given index.
	DataSectionPtr openChild( int index );

	/// This method returns the name of the child with the given index.
	BW::string childSectionName( int index );

	///	This method does nothing for DirSection
	DataSectionPtr newSection( const BW::StringRef &tag,
		DataSectionCreator* creator=NULL );

	/// This method searches for a new section directly under the current section.
	DataSectionPtr findChild( const BW::StringRef &tag,
		DataSectionCreator* creator=NULL  );

	/// These methods currently do nothing.
	void delChild (const BW::StringRef &tag );
	void delChild ( DataSectionPtr pSection );

	void delChildren();

	///	This method returns the section's name.
	virtual BW::string sectionName() const;

	/// This method returns the number of bytes used by this section.
	virtual int bytes() const;

	/// This method saves the section to the given filename
	virtual bool save(const BW::StringRef& filename = "");

	/// This method saves the given section (as if it were) our child
	virtual bool saveChild( DataSectionPtr pChild, bool isBinary );

	/// This method saves the given child of this data section, only if it already exists.
	virtual bool saveChildIfExists( DataSectionPtr pChild, bool isBinary );

	virtual DataSectionPtr convertToZip(const BW::StringRef& saveAs="",
		DataSectionPtr pRoot=NULL, BW::vector<DataSectionPtr>* children=NULL );
	//@}

	void tag(const BW::StringRef& tag) { tag_.assign( tag.data(), tag.size() ); }
	const BW::string& tag() const { return tag_; }
private:	

	BW::string					fullPath_;
	BW::string					tag_;
	BW::vector<BW::string>	children_;
	IFileSystem*				pFileSystem_;

	void						addChildren();
};

BW_END_NAMESPACE

#endif // _DIR_SECTION_HPP

