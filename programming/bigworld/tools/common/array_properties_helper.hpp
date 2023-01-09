#ifndef _ARRAY_PROPERTIES_HELPER_HPP_
#define _ARRAY_PROPERTIES_HELPER_HPP_


#include "base_properties_helper.hpp"
#include "cstdmf/bw_functor.hpp"

BW_BEGIN_NAMESPACE

// forward declarations
class EditorChunkItem;
class DataType;
typedef SmartPointer<DataType> DataTypePtr;


/**
 *	This is a properties helper class to manage arrays of properties.
 */
class ArrayPropertiesHelper : public BasePropertiesHelper
{
public:
	ArrayPropertiesHelper();
	virtual ~ArrayPropertiesHelper();

	virtual void init(
		EditorChunkItem* pItem,
		DataTypePtr dataType,
		PyObject* pSeq,
		BWBaseFunctor1<int>* changedCallback = NULL );

    virtual PyObject*		pSeq() const;

    virtual bool			addItem();
    virtual bool			delItem( int index );

	virtual BW::string		propName( PropertyIndex index );
    virtual int				propCount() const;
	virtual PropertyIndex	propGetIdx( const BW::string& name ) const;

	virtual PyObject*		propGetPy( PropertyIndex index );
	virtual bool			propSetPy( PropertyIndex index, PyObject * pObj );
	virtual DataSectionPtr	propGet( PropertyIndex index );
	virtual bool			propSet( PropertyIndex index, DataSectionPtr pTemp );

	void					propSetToDefault( int index );

	virtual bool			isUserDataObjectLink( int index );
	virtual bool			isUserDataObjectLinkArray( int index );

private:
	DataTypePtr			dataType_;
	PyObject*			pSeq_;
	SmartPointer< BWBaseFunctor1<int> >	changedCallback_;
};

BW_END_NAMESPACE

#endif //_ARRAY_PROPERTIES_HELPER_HPP_
