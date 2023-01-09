#ifndef CLOSED_CAPTIONS_HPP
#define CLOSED_CAPTIONS_HPP

#include "commentary.hpp"
#include "cstdmf/concurrency.hpp"
#include "romp/font.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

BW_BEGIN_NAMESPACE

class SimpleGUIComponent;
class TextGUIComponent;

/**
 *	This class displays commentary messages, for those
 *	who cannot hear because they just can't, or they
 *	haven't got a sound card.
 */
class ClosedCaptions : public Commentary::View, public PyObjectPlus
{
	Py_Header( ClosedCaptions, PyObjectPlus )

public:
	ClosedCaptions( int bufferSize = 5, PyTypeObject * pType = &s_type_ );
	~ClosedCaptions();

	void onAddMsg( const BW::wstring & msg, int id );
	void update( float dTime );

	void visible( bool state );
	bool visible() const;

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, visible, visible )

	PY_METHOD_DECLARE( py_update )

	PY_METHOD_DECLARE( py_addAsView )
	PY_METHOD_DECLARE( py_delAsView )

	PY_FACTORY_DECLARE()

private:
	ClosedCaptions( const ClosedCaptions& );
	ClosedCaptions& operator=( const ClosedCaptions& );

	void parseEventQueue();
	void addMsg( const BW::wstring & msg, int id );

	class Caption
	{
	public:
		Caption() :
			backing_(),
			component_(),
			age_( 0.f )
		{
		}

		SimpleGUIComponent * backing_;
		TextGUIComponent* component_;
		float			  age_;
	};
	typedef BW::vector<Caption>	Captions;

	SimpleGUIComponent * root_;
	
	Captions	msgs_;
	int			head_;
	CachedFontPtr font_;
	float		timeout_;
	float		fadeout_;
	float		clipHeight_;
	Vector2		margin_;

	struct PendingMessage
	{
		PendingMessage( const BW::wstring& msg, int id )
			:msg_( msg ),
			 id_( id )
		{
		};

		BW::wstring msg_;
		int id_;
	};
	typedef BW::vector< PendingMessage > PendingMessages;
	PendingMessages	pendingMessages_;
};

BW_END_NAMESPACE

#include "ashes/simple_gui_component.hpp"

#ifdef CODE_INLINE
#include "closed_captions.ipp"
#endif

#endif // CLOSED_CAPTIONS_HPP
