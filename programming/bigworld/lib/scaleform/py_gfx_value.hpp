#ifndef PY_GFXVALUE_HPP
#define PY_GFXVALUE_HPP

#include "config.hpp"
#include "cstdmf/bw_unordered_map.hpp"

#include "pyscript/script.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "py_gfx_script_function.hpp"

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{
	class PyMovieView;

	/*
	* Wrapper around Scaleform GFx::Value object for Python. Supports variety operations
	* with wrapped value as well as children retrieval and conversion from-and-to Python value.
	*/
	class PyGFxValue : public PyObjectPlus
	{
		Py_Header(PyGFxValue, PyObjectPlus);
	
	protected:
		PyGFxValue(PyTypePlus *pType = &s_type_);
		virtual ~PyGFxValue();

		// main factory method
		static PyGFxValue *createInternal(const char *name, PyGFxValue *pyParent, GFx::Value &gfxValue, PyMovieView *pyMovieView);

	public:
		static PyGFxValue *create(const char *name, PyGFxValue *pyParent);
		static PyGFxValue *create(const char *name, PyMovieView *pyMovieView);
		static PyGFxValue *create(GFx::Value &gfxValue, PyMovieView *pyMovieView);

		// cleans up stored SF value
		void releaseGFxValue();

		// converts SF value into Python object, returns Py_None on failure, requires PyMovieView reference to convert
		// non-integral value types into PyGFxValue (or derived) object
		static PyObject *convertValueToPython(GFx::Value &gfxValue, PyMovieView *pyMovieView)
			{ return PyGFxValue::convertValueToPython(gfxValue, pyMovieView, NULL); }
		
		// non-static method version, converts current GFx Value of this PyGFxValue
		PyObject *convertValueToPython() { return PyGFxValue::convertValueToPython(gfxValue_, pyMovieView_, this); }

		// converts Python value into SF value, requires PyMovieView reference to bind newly created values to appropriate movie
		// for proper release of SF values when movie is destroyed
		static GFx::Value convertValueToGFx(PyObject *pyValue, PyMovieView *pyMovieView);

		// returns Python dict with child value names as keys and PyGFxValue objects as values
		PyObject *children();
		PY_RO_ATTRIBUTE_GET(this->children(), children);
		PY_RO_ATTRIBUTE_SET(children);

		// object type retrieval
		bool isDisplayObject() { return gfxValue_.GetType() == GFx::Value::VT_DisplayObject; }
		bool isObjectOrMethod() { return gfxValue_.GetType() == GFx::Value::VT_Object || gfxValue_.GetType() == GFx::Value::VT_Closure; }
		PY_RO_ATTRIBUTE_DECLARE(isDisplayObject(), isDisplayObject);
		PY_RO_ATTRIBUTE_DECLARE(isObjectOrMethod(), isObjectOrMethod);

		void resyncSelf(); // reconnects GFx Value to its AS value using GetMember() of parent value or movie's GetVariable()
		void resync(); // calls resyncSelf on this object and all children
		PY_AUTO_METHOD_DECLARE(RETVOID, resyncSelf, END);
		PY_AUTO_METHOD_DECLARE(RETVOID, resync, END);
		
		// conversion of Object-type GFx value from/to dict
		PyObject *toDict();
		void fromDict(PyObjectPtr pyDict);
		PY_AUTO_METHOD_DECLARE(RETOWN, toDict, END);
		PY_AUTO_METHOD_DECLARE(RETVOID, fromDict, ARG(PyObjectPtr, END));

		// access to Python attributes by name
		ScriptObject pyGetAttribute( const ScriptString & attrObj );
		bool pySetAttribute( const ScriptString & attrObj,
			const ScriptObject & value );

		// etc. stuff for scripts
		void pyAdditionalMembers( const ScriptList & pList ) const;
		PY_RO_ATTRIBUTE_DECLARE(name_, name);
		PyObject *pyRepr();

		// callable object support
		PY_METHOD_DECLARE(pyCall);

	protected:
		void collectChildValues(); // grabs entire list of children values, without their hierarchy
		void unbindFromScript();
		void unbindFromScriptFunc();
		bool bindToScript(PyObject *pyScript); // creates PyScriptFunction for each public method exposed by Python object and binds to members of gfxValue_ with same names via SetMember
		static PyObject *convertValueToPython(GFx::Value &gfxValue, PyMovieView *pyMovieView, PyGFxValue *pyOwner);

		typedef BW::unordered_map<BW::string, SmartPointer<PyGFxValue> > Children_t;

		BW::string name_; // can be empty for nameless values
		GFx::Value gfxValue_; // actual data
		PyMovieView *pyMovieView_; // guaranteed to be set to NULL when PyMovieView destroys, therefore not smartpointer

		ScriptObject pyScript_; // optional script object assigned via 'script' attribute to this PyGFxValue
		PyGFxScriptFunctionPtr pyScriptFn_; // binds method of the parent_.pyScript_ to gfxValue_ of this object
		
		SmartPointer<PyGFxValue> parent_; // only assigned if this PyGFxValue is obtained by attribute accessor of another PyGFxValue
		Children_t children_;
		
		bool hasCollectedChildren_;
		bool needResyncSelf_; // if true, the next convertValueToPython() call will resync this GFx Value using GetMember() of the movie or parent value
	};

	typedef SmartPointer<PyGFxValue> PyGfxValuePtr;
}

BW_END_NAMESPACE

#endif
