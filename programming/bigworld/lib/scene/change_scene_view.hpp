#ifndef CHANGE_SCENE_VIEW_HPP
#define CHANGE_SCENE_VIEW_HPP

#include "forward_declarations.hpp"
#include "scene_event_view.hpp"

#include "cstdmf/bw_vector.hpp"

namespace BW
{

class AABB;

class SCENE_API ChangeSceneViewListener
{
public:
	virtual ~ChangeSceneViewListener();

	virtual void onAreaLoaded( const AABB & bb );
	virtual void onAreaUnloaded( const AABB & bb );
	virtual void onAreaChanged( const AABB & bb );
};

class SCENE_API ChangeSceneView :
	public SceneEventView
{
public:
	
	void addListener( ChangeSceneViewListener* pListener );
	void removeListener( ChangeSceneViewListener* pListener );

	void notifyAreaLoaded( const AABB & bb ) const;
	void notifyAreaUnloaded( const AABB & bb ) const;
	void notifyAreaChanged( const AABB & bb ) const;

protected:
	
	typedef BW::vector<ChangeSceneViewListener*> Listeners;
	Listeners listeners_;
};



} // namespace BW

#endif // CHANGE_SCENE_VIEW_HPP
