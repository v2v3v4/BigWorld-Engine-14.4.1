#ifndef PY_INPUT__HPP
#define PY_INPUT__HPP

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "ime_event.hpp"


BW_BEGIN_NAMESPACE

class PyKeyEvent : public PyObjectPlus
{
	Py_Header( PyKeyEvent, PyObjectPlus )

public:
	PyKeyEvent(const KeyEvent& event, PyTypeObject* pType = &PyKeyEvent::s_type_) 
		: PyObjectPlus( pType )
	{
		event_.fill( event );
	}

	~PyKeyEvent() {}

	PY_FACTORY_DECLARE()

	PY_RO_ATTRIBUTE_DECLARE( event_.repeatCount(), repeatCount );
	PY_RO_ATTRIBUTE_DECLARE( event_.key(), key );
	PY_RO_ATTRIBUTE_DECLARE( event_.modifiers(), modifiers );
	PY_RO_ATTRIBUTE_DECLARE( event_.cursorPosition(), cursorPosition );

	PyObject* pyGet_character();
	PY_RO_ATTRIBUTE_SET( character )

	bool isKeyDown() { return event_.isKeyDown(); }
	PY_AUTO_METHOD_DECLARE( RETDATA, isKeyDown, END );

	bool isShiftDown() { return event_.isShiftDown(); }
	PY_AUTO_METHOD_DECLARE( RETDATA, isShiftDown, END );

	bool isCtrlDown() { return event_.isCtrlDown(); }
	PY_AUTO_METHOD_DECLARE( RETDATA, isCtrlDown, END );

	bool isAltDown() { return event_.isAltDown(); }
	PY_AUTO_METHOD_DECLARE( RETDATA, isAltDown, END );

	bool isModifierDown() { return event_.modifiers() != 0; }
	PY_AUTO_METHOD_DECLARE( RETDATA, isModifierDown, END );

	bool isKeyUp() { return event_.isKeyUp(); }
	PY_AUTO_METHOD_DECLARE( RETDATA, isKeyUp, END );

	bool isRepeatedEvent() { return event_.isRepeatedEvent(); }
	PY_AUTO_METHOD_DECLARE( RETDATA, isRepeatedEvent, END );

	bool isMouseButton() { return event_.isMouseButton(); }
	PY_AUTO_METHOD_DECLARE( RETDATA, isMouseButton, END );



	const KeyEvent& event() const { return event_; }
private:
	KeyEvent event_;
};

class PyMouseEvent : public PyObjectPlus
{
	Py_Header( PyMouseEvent, PyObjectPlus )

public:
	PyMouseEvent(const MouseEvent& event, PyTypeObject* pType = &PyMouseEvent::s_type_) 
		: PyObjectPlus( pType )
	{
		event_.fill( event );
	}

	~PyMouseEvent() {}

	PY_FACTORY_DECLARE()

	PY_RO_ATTRIBUTE_DECLARE( event_.dx(), dx );
	PY_RO_ATTRIBUTE_DECLARE( event_.dy(), dy );
	PY_RO_ATTRIBUTE_DECLARE( event_.dz(), dz );
	PY_RO_ATTRIBUTE_DECLARE( event_.cursorPosition(), cursorPosition );


	const MouseEvent& event() const { return event_; }
private:
	MouseEvent event_;
};

class PyAxisEvent : public PyObjectPlus
{
	Py_Header( PyAxisEvent, PyObjectPlus )

public:
	PyAxisEvent(const AxisEvent& event, PyTypeObject* pType = &PyAxisEvent::s_type_) 
		: PyObjectPlus( pType )
	{
		event_.fill( event );
	}

	~PyAxisEvent() {}

	PY_FACTORY_DECLARE()

	PY_RO_ATTRIBUTE_DECLARE( event_.axis(), axis );
	PY_RO_ATTRIBUTE_DECLARE( event_.value(), value );
	PY_RO_ATTRIBUTE_DECLARE( event_.dTime(), dTime );	


	const AxisEvent& event() const { return event_; }
public:
	AxisEvent event_;
};

class PyIMEEvent : public PyObjectPlus
{
	Py_Header( PyIMEEvent, PyObjectPlus )

public:
	PyIMEEvent(const IMEEvent& event, PyTypeObject* pType = &PyIMEEvent::s_type_) 
		:	PyObjectPlus( pType ),
			event_( event )
	{

	}

	~PyIMEEvent() {}

	PY_RO_ATTRIBUTE_DECLARE( event_.stateChanged(), stateChanged );
	PY_RO_ATTRIBUTE_DECLARE( event_.readingVisibilityChanged(), readingVisibilityChanged );
	PY_RO_ATTRIBUTE_DECLARE( event_.readingChanged(), readingChanged );
	PY_RO_ATTRIBUTE_DECLARE( event_.candidatesVisibilityChanged(), candidatesVisibilityChanged );
	PY_RO_ATTRIBUTE_DECLARE( event_.candidatesChanged(), candidatesChanged );
	PY_RO_ATTRIBUTE_DECLARE( event_.selectedCandidateChanged(), selectedCandidateChanged );
	PY_RO_ATTRIBUTE_DECLARE( event_.compositionChanged(), compositionChanged );
	PY_RO_ATTRIBUTE_DECLARE( event_.compositionCursorPositionChanged(), compositionCursorPositionChanged );

	const IMEEvent& event() const { return event_; }
private:
	IMEEvent event_;
};

namespace Script
{
	int setData( PyObject * pObject, KeyEvent & rEvent,
		const char * varName = "" );
	PyObject * getData( const KeyEvent & rEvent );

	int setData( PyObject * pObject, IMEEvent & rEvent,
		const char * varName = "" );
	PyObject * getData( const IMEEvent & rEvent );

	int setData( PyObject * pObject, MouseEvent & rEvent,
		const char * varName = "" );
	PyObject * getData( const MouseEvent & rEvent );

	int setData( PyObject * pObject, AxisEvent & rEvent,
		const char * varName = "" );
	PyObject * getData( const AxisEvent & rEvent );
};

BW_END_NAMESPACE

#endif // PY_INPUT__HPP
