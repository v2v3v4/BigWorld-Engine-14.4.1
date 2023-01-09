#ifndef PY_GUI_ITEM_HPP_
#define PY_GUI_ITEM_HPP_

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "gui_manager.hpp"

BW_BEGIN_NAMESPACE

/*~ class GUI.Item
 *	@components{ tools }
 */
class PyItem : public PyObjectPlus
{
	Py_Header( PyItem, PyObjectPlus )

public:
	PyItem( GUI::ItemPtr pItem,
		PyTypeObject * pType = &PyItem::s_type_ );
	~PyItem();

	GUI::ItemPtr	pItem() const;

	ScriptObject	pyGetAttribute( const ScriptString & attrObj );
	PyObject * 		subscript( PyObject * entityID );
	int				length();

	PY_METHOD_DECLARE( py_has_key )
	PY_METHOD_DECLARE( py_keys )
	PY_METHOD_DECLARE( py_values )
	PY_METHOD_DECLARE( py_items )

	PY_METHOD_DECLARE( py_createItem )
	PY_METHOD_DECLARE( py_deleteItem )

	PY_METHOD_DECLARE( py_copy )

	PyObject * child( int index );
	PY_AUTO_METHOD_DECLARE( RETOWN, child, ARG( int, END ) )
	PyObject * childName( int index );
	PY_AUTO_METHOD_DECLARE( RETOWN, childName, ARG( int, END ) )

	PY_RO_ATTRIBUTE_DECLARE( pItem_->name(), name )
	PY_RO_ATTRIBUTE_DECLARE( pItem_->displayName(), displayName )

	static PyObject * 	s_subscript( PyObject * self, PyObject * entityID );
	static Py_ssize_t	s_length( PyObject * self );

	PY_FACTORY_DECLARE()

private:
	GUI::ItemPtr pItem_;
};

typedef SmartPointer<PyItem> PyItemPtr;

PY_SCRIPT_CONVERTERS_DECLARE( PyItem )

BW_END_NAMESPACE

#endif // PY_GUI_ITEM_HPP_
