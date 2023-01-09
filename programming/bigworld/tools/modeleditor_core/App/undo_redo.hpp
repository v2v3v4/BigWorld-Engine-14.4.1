#ifndef UNDO_REDO_HPP
#define UNDO_REDO_HPP

#include "gizmo/undoredo.hpp"

BW_BEGIN_NAMESPACE

typedef SmartPointer< class XMLSection > XMLSectionPtr;
typedef std::pair< BW::string, BW::string > StringPair;

class UndoRedoOp : public UndoRedo::Operation
{
public:
	UndoRedoOp( int kind, DataSectionPtr data, DataSectionPtr parent = NULL, bool materialFlagChange = false, StringPair item = StringPair() );
	~UndoRedoOp();
	virtual void undo();
	virtual bool iseq( const UndoRedo::Operation & oth ) const;
	const DataSectionPtr data() const	{ return data_; }
private:
	int kind_;
	DataSectionPtr data_;
	DataSectionPtr parent_;

	XMLSectionPtr state_;
	bool materialFlagChange_;
	StringPair item_;
};


class UndoRedoMatterName : public UndoRedo::Operation
{
public:
	UndoRedoMatterName( const BW::string & oldName, const BW::string & newName );
	~UndoRedoMatterName();

	virtual void undo();
	virtual bool iseq( const UndoRedo::Operation & oth ) const;
private:
	BW::string oldName_;
	BW::string newName_;

};


class UndoRedoTintName : public UndoRedo::Operation
{
public:
	UndoRedoTintName( const BW::string & matterName, const BW::string & oldName, const BW::string & newName );
	~UndoRedoTintName();

	virtual void undo();
	virtual bool iseq( const UndoRedo::Operation & oth ) const;
private:
	BW::string matterName_;
	BW::string oldName_;
	BW::string newName_;

};

BW_END_NAMESPACE

#endif // UNDO_REDO_HPP