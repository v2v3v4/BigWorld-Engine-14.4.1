#ifndef GENERIC_ENTITY_VIEW_EXTENSION_HPP
#define GENERIC_ENTITY_VIEW_EXTENSION_HPP

#include "Classes/EntityView.hpp"

#import "Classes/ApplicationDelegate.h"
#include "IOSClientFilterEnvironment.h"

#include "connection/avatar_filter.hpp"
#include "connection_model/bw_entity.hpp"


/**
 *	This template class provides a default entity view for most entities. This
 *	can be subclassed to provide specific overrides.
 */
template< class ENTITY_EXTENSION >
class GenericEntityViewExtension : public ENTITY_EXTENSION,
		public EntityViewDataProvider
{
public:
	
	/**
	 *	Constructor.
	 */
	GenericEntityViewExtension(
				const typename ENTITY_EXTENSION::EntityType * pEntity ) :
			ENTITY_EXTENSION( pEntity ),
			EntityViewDataProvider(),
			pView_( NULL )
	{}
	
	
	/**
	 *	Destructor.
	 */
	virtual ~GenericEntityViewExtension()
	{
		this->removeView();
	}
	
	
	/** Override from EntityViewDataProvider. */
	virtual const BW::BWEntity * pViewEntity() const
	{
		return this->ENTITY_EXTENSION::pEntity();
	}
	

	/** Override from EntityViewDataProvider. */
	virtual BW::string spriteName() const
	{
		return "default";
	}
	
	
	/** Override from EntityViewDataProvider. */
	virtual BW::string popoverString() const
	{
		return this->pEntity()->entityTypeName();
	}
	

	/** Override from EntityViewDataProvider. */
	virtual const BW::Vector3 spriteTint() const
	{
		return BW::Vector3( 255, 255, 255 );
	}
	

	/** Override from EntityViewDataProvider. */
	virtual void onClick()
	{
	}
	

	/** Override from EntityExtension. */
	virtual void onEnterWorld()
	{
		if (pView_)
		{
			return;
		}
		
		pView_ = new EntityView( *this );
		
		ApplicationDelegate * delegate = [ApplicationDelegate sharedDelegate];
		[delegate addEntityView: pView_];
		
		this->setEntityFilter(
			new BW::AvatarFilter( *(delegate.filterEnvironment) ) );
	}
	
	
	/** Override from EntityExtension. */
	virtual void onLeaveWorld()
	{
		this->removeView();
	}
	
private:

	/**
	 *	This method removes the view from the delegate.
	 */
	void removeView()
	{
		if (!pView_)
		{
			return;
		}
		
		[[ApplicationDelegate sharedDelegate] removeEntityView: pView_];
		delete pView_;
		pView_ = NULL;
		
	}

	
	EntityView * pView_;
};


#endif // GENERIC_ENTITY_VIEW_EXTENSION_HPP

