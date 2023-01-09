#ifndef _PROPERTIES_HELPER_HPP_
#define _PROPERTIES_HELPER_HPP_


#include "base_properties_helper.hpp"
#include "cstdmf/bw_functor.hpp"

BW_BEGIN_NAMESPACE

// forward declarations
class EditorChunkItem;
class BaseUserDataObjectDescription;


/**
 *	This is a properties helper class. It must be initialised using a
 *	editor chunk, a type description and a python dictionary.
 */
class PropertiesHelper : public BasePropertiesHelper
{
public:
	typedef BW::map< int, PropertyIndex > PropertyMap;

	PropertiesHelper();

	virtual void init(
		EditorChunkItem* pItem,
		BaseUserDataObjectDescription* pType,
		PyObject* pDict,
		BWBaseFunctor1<int>* changedCallback = NULL );

	virtual BaseUserDataObjectDescription*	pType() const;

	virtual BW::string		propName( PropertyIndex index );
    virtual int				propCount() const;
	virtual PropertyIndex	propGetIdx( const BW::string& name ) const;

	virtual PyObject*		propGetPy( PropertyIndex index );
	virtual bool			propSetPy( PropertyIndex index, PyObject * pObj );
	virtual DataSectionPtr	propGet( PropertyIndex index );
	virtual bool			propSet( PropertyIndex index, DataSectionPtr pTemp );

	PyObjectPtr				propGetDefault( int index );
	void					propSetToDefault( int index );

	virtual void			setEditProps(
								const BW::list<BW::string>& names, BW::vector<bool>& allowEdit );
	virtual void			clearEditProps( BW::vector<bool>& allowEdit );
	virtual void			clearProperties();
	virtual void			clearPropertySection( DataSectionPtr pOwnSect );

	virtual bool			isUserDataObjectLink( int index );
	virtual bool			isUserDataObjectLinkArray( int index );

	BW::vector<BW::string>	command();
	PropertyIndex				commandIndex( int index );

private:
	BaseUserDataObjectDescription*			pType_;
	PyObject*								pDict_;
	SmartPointer< BWBaseFunctor1<int> >	changedCallback_;
	PropertyMap								propMap_;
};

BW_END_NAMESPACE

#endif //_PROPERTIES_HELPER_HPP_
