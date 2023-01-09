#include "pch.hpp"
#include "py_gfx_value.hpp"
#include "py_movie_view.hpp"
#include "py_gfx_display_object.hpp"
#include "py_gfx_script_function.hpp"

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT_WITH_CALL(ScaleformBW::PyGFxValue)

namespace ScaleformBW {


//--------------------------------------

PY_BEGIN_METHODS(PyGFxValue)
	PY_METHOD(toDict)
	PY_METHOD(fromDict)
	PY_METHOD(resync)
	PY_METHOD(resyncSelf)
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES(PyGFxValue)
	PY_ATTRIBUTE(name)
	PY_ATTRIBUTE(children)
	PY_ATTRIBUTE(isDisplayObject)
	PY_ATTRIBUTE(isObjectOrMethod)
PY_END_ATTRIBUTES()

PyGFxValue::PyGFxValue(PyTypePlus *pType)
	:	hasCollectedChildren_(false),
		needResyncSelf_(false),
		PyObjectPlus(pType)
{
	BW_GUARD;
}

PyGFxValue *PyGFxValue::createInternal(const char *name, PyGFxValue *pyParent, GFx::Value &gfxValue, PyMovieView *pyMovieView)
{
	BW_GUARD;

	MF_ASSERT(pyMovieView != NULL);

	PyGFxValue *pRet;
	if (gfxValue.GetType() == GFx::Value::VT_DisplayObject)
		pRet = new PyGFxDisplayObject();
	else
		pRet = new PyGFxValue();

	if (name != NULL)
		pRet->name_ = name;

	pRet->parent_ = pyParent;
	pRet->pyMovieView_ = pyMovieView;
	pRet->gfxValue_ = gfxValue;

	pyMovieView->ownedValues_.insert(pRet);

	return pRet;
}

PyGFxValue *PyGFxValue::create(const char *name, PyGFxValue *pyParent)
{
	BW_GUARD;

	MF_ASSERT(pyParent != NULL);

	GFx::Value gfxValue;
	if (pyParent->gfxValue_.GetMember(name, &gfxValue))
		return createInternal(name, pyParent, gfxValue, pyParent->pyMovieView_);
	else
		return NULL;
}

PyGFxValue *PyGFxValue::create(const char *name, PyMovieView *pyMovieView)
{
	BW_GUARD;

	MF_ASSERT(pyMovieView != NULL);

	GFx::Value gfxValue;
	if (pyMovieView->pMovieView()->GetVariable(&gfxValue, name))
		return createInternal(name, NULL, gfxValue, pyMovieView);
	else
		return NULL;
}

PyGFxValue *PyGFxValue::create(GFx::Value &gfxValue, PyMovieView *pyMovieView)
{
	BW_GUARD;

	MF_ASSERT(pyMovieView != NULL);

	return createInternal(NULL, NULL, gfxValue, pyMovieView);
}

PyGFxValue::~PyGFxValue()
{
	BW_GUARD;

	if (pyMovieView_ != NULL)
	{
		pyMovieView_->ownedValues_.erase(this);
		pyMovieView_ = NULL;
	}

	releaseGFxValue();
}

void PyGFxValue::releaseGFxValue()
{
	BW_GUARD;

	unbindFromScript();

	gfxValue_.SetNull();
	
	pyMovieView_ = NULL;
	parent_ = NULL;
	children_.clear();
}

PyObject *PyGFxValue::convertValueToPython(GFx::Value &gfxValue, PyMovieView *pyMovieView, PyGFxValue *pyOwner)
{
	BW_GUARD;

	PyObject *pyValue = NULL;

	// on-demand auto-sync GFx Value owned by PyGFxValue
	if (pyOwner != NULL && pyOwner->needResyncSelf_)
		pyOwner->resyncSelf();

	// for basic AS types, convert GFx::Value to one of integral Python types,
	// for AS Arrays, convert into Python list
	// for DisplayObjects, Objects and etc. unknown types - return reference to ourself
	GFx::Value::ValueType type = gfxValue.GetType();
	switch (type)
	{
	case GFx::Value::VT_Undefined:
	case GFx::Value::VT_Null:
		pyValue = Py_None;
		Py_XINCREF(pyValue);
		break;

	case GFx::Value::VT_Boolean:
		pyValue = PyBool_FromLong(gfxValue.GetBool());
		break;

	case GFx::Value::VT_Number:
		pyValue = PyFloat_FromDouble(gfxValue.GetNumber());
		break;

	case GFx::Value::VT_Int:
		pyValue = PyLong_FromLong(gfxValue.GetInt());
		break;

	case GFx::Value::VT_UInt:
		pyValue = PyLong_FromLong(gfxValue.GetUInt());
		break;

	case GFx::Value::VT_String:
		pyValue = PyString_FromString(gfxValue.GetString());
		break;

	case GFx::Value::VT_StringW:
		pyValue = PyUnicode_FromWideChar(gfxValue.GetStringW(), wcslen(gfxValue.GetStringW()) );
		break;

	case GFx::Value::VT_Array:
		{
			// convert to Python list
			uint arrSize = gfxValue.GetArraySize();
			pyValue = PyList_New(arrSize);
			for (uint i = 0; i < arrSize; ++i)
			{
				GFx::Value gfxArrElem;
				if (gfxValue.GetElement(i, &gfxArrElem))
					PyList_SetItem(pyValue, i, convertValueToPython(gfxArrElem, pyMovieView));
			}
		}
		break;

	default:
		if (pyOwner == NULL && pyMovieView == NULL)
		{
			// don't know what to do if only have raw GFxValue and no connection with PyMovieView
			pyValue = Py_None;
			Py_XINCREF(pyValue);
		}
		else if (pyOwner == NULL)
		{
			// orbitrary object-type value returned by AS method, make new nameless PyGFxValue from it
			pyValue = PyGFxValue::create(gfxValue, pyMovieView);
			break;
		}
		else
		{
			// return Object and DisplayObject by reference
			pyValue = pyOwner;
			Py_XINCREF(pyValue);
			break;
		}
	}

	return pyValue;
}

GFx::Value PyGFxValue::convertValueToGFx(PyObject *pyValue, PyMovieView *pyMovieView)
{
	BW_GUARD;

	GFx::Movie *pMovie = pyMovieView->pMovieView();
	GFx::Value gfxValue;
	gfxValue.SetUndefined();

	if (pyValue == NULL)
	{
		gfxValue.SetNull();
	}
	else if (pyValue->ob_type == &PyGFxValue::s_type_)
	{
		gfxValue = ((PyGFxValue*)pyValue)->gfxValue_;
	}
	else if (PyUnicode_CheckExact(pyValue))
	{
		PyObject *utf8 = PyUnicode_AsUTF8String(pyValue);
		gfxValue.SetString(PyString_AsString(utf8));
	}
	else if (PyString_CheckExact(pyValue))
	{
		gfxValue.SetString(PyString_AsString(pyValue));
	}
	else if (PyInt_CheckExact(pyValue))
	{
		gfxValue.SetInt(PyInt_AsLong(pyValue));
	}
	else if (PyLong_CheckExact(pyValue))
	{
		gfxValue.SetNumber(PyLong_AsDouble(pyValue));
	}
	else if (PyFloat_CheckExact(pyValue))
	{
		gfxValue.SetNumber(PyFloat_AsDouble(pyValue));
	}
	else if (PyBool_Check(pyValue))
	{
		gfxValue.SetBoolean(pyValue == Py_True);
	}
	else if (PyTuple_Check(pyValue))
	{
		pMovie->CreateArray(&gfxValue);
		Py_ssize_t countPy = PyTuple_GET_SIZE(pyValue);
		MF_ASSERT( countPy >= 0 && countPy <= UINT_MAX );
		uint count = static_cast<uint>( countPy );
		for (uint i = 0; i < count; ++i)
		{
			PyObject *pyElem = PyTuple_GetItem(pyValue, i);
			gfxValue.SetElement(i, convertValueToGFx(pyElem, pyMovieView));
		}
	}
	else if (PyList_Check(pyValue))
	{
		pMovie->CreateArray(&gfxValue);
		Py_ssize_t countPy = PyList_GET_SIZE(pyValue);
		MF_ASSERT( countPy >= 0 && countPy <= UINT_MAX );
		uint count = static_cast<uint>( countPy );
		for (uint i = 0; i < count; ++i)
		{
			PyObject *pyElem = PyList_GetItem(pyValue, i);
			gfxValue.SetElement(i, convertValueToGFx(pyElem, pyMovieView));
		}
	}
	else if (PyDict_Check(pyValue))
	{
		// make AS hashmap (aka object)
		pMovie->CreateObject(&gfxValue);
		
		PyObject *pyDictItems = PyDict_Items(pyValue);
		Py_ssize_t numItems = PyList_Size(pyDictItems);
		for (Py_ssize_t i = 0; i < numItems; ++i)
		{
			// all 3 are borrowed references, no need to decref them after use
			PyObject *pyKVPair = PyList_GetItem(pyDictItems, i);
			PyObject *pyKVKey = PyTuple_GetItem(pyKVPair, 0);
			PyObject *pyKVValue = PyTuple_GetItem(pyKVPair, 1);
			
			const char *objKey = PyString_AsString(pyKVKey);
			GFx::Value objVal = convertValueToGFx(pyKVValue, pyMovieView);

			gfxValue.SetMember(objKey, objVal);
		}

		Py_XDECREF(pyDictItems);
	}

	return gfxValue;
}

void PyGFxValue::collectChildValues()
{
	BW_GUARD;

	if (hasCollectedChildren_)
		return;

	children_.clear();
	hasCollectedChildren_ = true;

	// note: collecting not values themself but names, because Visit() seem to stick to
	// the hierarchy obtained only once, on first try (TODO - reconsider after next SF version)
	struct PyGFxChildrenCollector : public GFx::Value::ObjectVisitor
	{
		PyGFxValue *pyParent_;
		BW::vector<BW::string> names;
		PyGFxChildrenCollector(PyGFxValue *pyParent) : pyParent_(pyParent) { }
		bool IncludeAS3PublicMembers() const  { return true; }
		void Visit(const char *name, const GFx::Value &gfxValue) { names.push_back(name); }
	};

	PyGFxChildrenCollector collector(this);
	gfxValue_.VisitMembers(&collector);

	for (BW::vector<BW::string>::iterator it = collector.names.begin(); it != collector.names.end(); ++it)
		children_[*it] = PyGFxValue::create(it->c_str(), this);
}

bool PyGFxValue::bindToScript(PyObject *pyScript)
{
	BW_GUARD;

	if (pyScript == NULL || pyMovieView_ == NULL)
		return false;

	if (pyScript_ == pyScript)
		return true;

	unbindFromScript();

	pyScript_ = pyScript;
	if (pyScript == Py_None)
		return true;

	collectChildValues();

	for (Children_t::iterator it = children_.begin(); it != children_.end(); ++it)
	{
		PyGFxValue *pyChild = it->second.get();
		if (pyChild == NULL || pyChild->pyMovieView_ == NULL)
			continue;

		const BW::string &funcName = it->first;
		if (!PyObject_HasAttrString(pyScript, funcName.c_str()))
			continue;

		PyGFxScriptFunction *pyScriptFn = SF_HEAP_NEW(SF::Memory::GetGlobalHeap()) PyGFxScriptFunction(pyScript, pyMovieView_, funcName);
		
		pyMovieView_->pMovieView()->CreateFunction(&pyChild->gfxValue_, pyScriptFn);
		if (!gfxValue_.SetMember(funcName.c_str(), pyChild->gfxValue_))
		{
			delete pyScriptFn;
			return false;
		}

		pyChild->pyScriptFn_ = pyScriptFn;
	}

	return true;
}

void PyGFxValue::unbindFromScript()
{
	BW_GUARD;

	static GFx::Value gfxNullVal(GFx::Value::VT_Null);

	if (pyScript_ != NULL)
	{
		for (Children_t::iterator it = children_.begin(); it != children_.end(); ++it)
		{
			PyGFxValue *pyChild = it->second.get();
			if (pyChild != NULL && pyChild->pyScriptFn_ != NULL)
			{
				pyChild->unbindFromScriptFunc();
				gfxValue_.SetMember(it->first.c_str(), gfxNullVal);	
			}
		}
		pyScript_ = NULL;
	}

	unbindFromScriptFunc();
}

void PyGFxValue::unbindFromScriptFunc()
{
	BW_GUARD;

	if (pyScriptFn_ != NULL)
	{
		pyScriptFn_->releasePointers();
		pyScriptFn_ = NULL;
		gfxValue_.SetNull();
	}
}

void PyGFxValue::resync()
{
	BW_GUARD;

	resyncSelf();

	hasCollectedChildren_ = false;
	collectChildValues();
}

void PyGFxValue::resyncSelf()
{
	BW_GUARD;

	if (!needResyncSelf_)
		return;

	needResyncSelf_ = false;
	
	if (name_.empty())
		return;

	if (parent_ != NULL)
		parent_->gfxValue_.GetMember(name_.c_str(), &gfxValue_);
	else if (pyMovieView_ != NULL)
		pyMovieView_->pMovieView()->GetVariable(&gfxValue_, name_.c_str());
}

PyObject *PyGFxValue::children()
{
	BW_GUARD;

	collectChildValues();

	PyObject *pyDict = PyDict_New();
	for (Children_t::iterator it = children_.begin(); it != children_.end(); ++it)
	{
		PyGFxValue *pyChild = it->second.get();
		if (pyChild != NULL)
			PyDict_SetItem(pyDict, Script::getData(it->first), pyChild->convertValueToPython());
	}

	return pyDict;
}

void PyGFxValue::fromDict(PyObjectPtr dict)
{
	BW_GUARD;

	PyObject *pyDict = dict.get();
	if (!PyDict_Check(pyDict))
	{
		PyErr_SetString(PyExc_TypeError, "Failed to convert argument from dict to PyGFxValue - given argument is not of dict type");
		return;
	}

	if (gfxValue_.GetType() != GFx::Value::VT_Object
		&& gfxValue_.GetType() != GFx::Value::VT_DisplayObject)
	{
		PyErr_SetString(PyExc_TypeError, "Failed to convert argument from dict to PyGFxValue - the type of GFx Value is not ActionScript Object.");
		return;
	}

	PyObject *pyDictItems = PyDict_Items(pyDict);
	Py_ssize_t numItems = PyList_Size(pyDictItems);
	for (Py_ssize_t i = 0; i < numItems; ++i)
	{
		// all 3 are borrowed references, no need to decref them after use
		PyObject *pyKVPair = PyList_GetItem(pyDictItems, i);
		PyObject *pyKVKey = PyTuple_GetItem(pyKVPair, 0);
		PyObject *pyKVValue = PyTuple_GetItem(pyKVPair, 1);

		const char *objKey = PyString_AsString(pyKVKey);
		GFx::Value objVal = convertValueToGFx(pyKVValue, pyMovieView_);

		if (!gfxValue_.SetMember(objKey, objVal))
			PyErr_Format(PyExc_ValueError, "Failed to assign value by key '%s' to PyGFxValue - most likely this key is missing in ActionScript Object.", objKey);
	}

	resync();

	Py_XDECREF(pyDictItems);
}

PyObject *PyGFxValue::toDict()
{
	BW_GUARD;

	if (gfxValue_.GetType() != GFx::Value::VT_Object
		&& gfxValue_.GetType() != GFx::Value::VT_DisplayObject)
	{
		PyErr_SetString(PyExc_TypeError, "Failed to convert PyGFxValue to dict - the type of GFx Value is not ActionScript Object.");
		Py_Return;
	}

	return children();
}

ScriptObject PyGFxValue::pyGetAttribute( const ScriptString & attrObj )
{
	BW_GUARD;

	const char * attr = attrObj.c_str();

	Children_t::iterator it = children_.find(attr);
	if (strcmp(attr, "script") == 0)
	{
		if (pyScript_ == NULL)
		{
			return ScriptObject::none();
		}
		else
		{
			return pyScript_;
		}
	}
	else if (it == children_.end())
	{
		PyGFxValue *pyChild = PyGFxValue::create(attr, this);
		children_[attr] = pyChild;
		if (pyChild != NULL)
		{
			return ScriptObject( pyChild->convertValueToPython(),
				ScriptObject::FROM_NEW_REFERENCE );
		}
	}
	else if (it->second != NULL)
	{
		return ScriptObject( it->second->convertValueToPython(),
			ScriptObject::FROM_NEW_REFERENCE );
	}

	return PyObjectPlus::pyGetAttribute( attrObj );
}

bool PyGFxValue::pySetAttribute( const ScriptString & attrObj,
	const ScriptObject & value )
{
	BW_GUARD;

	const char * attr = attrObj.c_str();

	if (strcmp(attr, "script") == 0)
	{
		if (bindToScript(value.get()))
		{
			return true;
		}
		else
		{
			PyErr_Format(PyExc_AttributeError, 
				"Cannot assign 'script' attribute - "
				"failed to bind script to PyGFxValue '%s'", name_.c_str());
			return false;
		}
	}

	GFx::Value gfxValueNew = convertValueToGFx(value.get(), pyMovieView_);

	Children_t::iterator it = children_.find(attr);
	if (it != children_.end())
	{
		PyGFxValue *pyChild = it->second.get();
		GFx::Value::ValueType type = gfxValue_.GetType();

		if (type == GFx::Value::VT_DisplayObject
			|| type == GFx::Value::VT_Object)
		{
			// assign nested values of DisplayObjects and Objects with SetMember and
			// resync nested hierarchy of PyGFxValue objects to appropriate GFx Values
			if (gfxValue_.SetMember(attr, gfxValueNew))
			{
				resync();
				return true;
			}
		}
		else
		{
			// assign integral types and arrays directly with Value::operator=
			pyChild->gfxValue_ = gfxValueNew;
			return true;
		}
	}
	else
	{
		// we don't have created child PyGFxValue yet, 
		// so just pass new value through SetMember of GFx Value
		if (gfxValue_.SetMember(attr, gfxValueNew))
		{
			return true;
		}
	}

	return PyObjectPlus::pySetAttribute( attrObj, value );
}

PyObject *PyGFxValue::pyCall(PyObject *args)
{
	BW_GUARD;

	if ((parent_ == NULL && pyMovieView_ == NULL) || name_.empty())
	{
		PyErr_Format(PyExc_TypeError, "Cannot invoke Action Script method on object not attached to the Movie clip");
		return NULL;	
	}

	if (gfxValue_.GetType() != GFx::Value::VT_Object
		&& gfxValue_.GetType() != GFx::Value::VT_Closure)
	{
		PyErr_Format(PyExc_TypeError, "Cannot invoke Action Script method on object of non-Object type (method name: %s)",
			name_.c_str());
		return NULL;
	}

	GFx::Value &obj = parent_->gfxValue_;
	Py_ssize_t numArgsPy = PyTuple_Size(args);
	MF_ASSERT( numArgsPy >= 0 && numArgsPy <= UINT_MAX );
	uint numArgs = static_cast<uint>( numArgsPy );
	GFx::Value gfxResult;
	GFx::Movie *pMovie = pyMovieView_->pMovieView();

	bool success = false;
	if (numArgs == 0)
	{
		// empty argument list
		success = (parent_ == NULL) ? pMovie->Invoke(name_.c_str(), &gfxResult, NULL, 0)
			: obj.Invoke(name_.c_str(), &gfxResult, NULL, 0);
	}
	else
	{
		// arguments stored in the tuple of args passed to this function.
		BW::vector<GFx::Value> gfxArgs(numArgs);
		for (uint i = 0; i < numArgs; ++i)
			gfxArgs[i] = convertValueToGFx(PyTuple_GetItem(args, i), pyMovieView_);

		success = (parent_ == NULL) ? pMovie->Invoke(name_.c_str(), &gfxResult, &gfxArgs[0], numArgs)
			: obj.Invoke(name_.c_str(), &gfxResult, &gfxArgs[0], numArgs);
	}

	if (!success)
	{
		PyErr_Format(PyExc_Exception, "PyGFxValue - Failed to invoke method %s.", name_.c_str());
		return NULL;
	}

	// mark all connected child values at same level with this function object as out-of-sync, but don't forcibly sync them now
	// TODO: reconsider, maybe on-demand resync should be recursive instead of single-level?
	if (success && parent_ != NULL)
	{
		for (Children_t::iterator it = parent_->children_.begin();
			it != parent_->children_.end(); ++it)
		{
			PyGFxValue *pyValue = it->second.get();
			if (pyValue != NULL)
				pyValue->needResyncSelf_ = true;
		}
	}

	// convert result into Python without referencing it to any PyGFxValue (since we only have raw GFx Value there)
	return convertValueToPython(gfxResult, pyMovieView_);
}

void PyGFxValue::pyAdditionalMembers( const ScriptList & pList ) const
{
	BW_GUARD;

	// Const cast to populate the list.
	const_cast< PyGFxValue * >( this )->collectChildValues();

	for (Children_t::const_iterator it = children_.begin();
		it != children_.end(); ++it)
	{
		PyGFxValue *pyChild = it->second.get();
		if (pyChild != NULL)
		{
			pList.append( ScriptString::create( it->first ) );
		}
	}
}

PyObject *PyGFxValue::pyRepr()
{
	BW_GUARD;

	if (isDisplayObject())
		return PyString_FromString("DisplayObject");
	else if (isObjectOrMethod())
		return PyString_FromString("Object");

	return PyObjectPlus::pyRepr();
}

}

BW_END_NAMESPACE
