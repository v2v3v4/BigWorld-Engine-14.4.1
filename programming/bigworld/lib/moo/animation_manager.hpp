#ifndef ANIMATION_MANAGER_HPP
#define ANIMATION_MANAGER_HPP

#include "cstdmf/bw_map.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/singleton.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

class Animation;
typedef SmartPointer<Animation> AnimationPtr;
class Node;
typedef SmartPointer<Node> NodePtr;

/**
 *	Control centre for the animation system. All animations are stored here
 *	and retrieved through the get and find methods.
 */
class AnimationManager : public Singleton< AnimationManager >
{
public:
	AnimationManager();

	AnimationPtr get( const BW::string & resourceID, NodePtr rootNode );
	AnimationPtr get( const BW::string & resourceID );
	AnimationPtr getEmptyTestAnimation( const BW::string & resourceID );

	/*
	 * Public so that ModelViewer can change the original Animation when
	 * fiddling with compression.
	 */
	AnimationPtr				find( const BW::string& resourceID );

	BW::string					resourceID( Animation * pAnim );

	void fullHouse( bool noMoreEntries = true );

	typedef BW::map< BW::string, Animation * > AnimationMap;

private:
	AnimationMap				animations_;
	SimpleMutex					animationsLock_;

	bool						fullHouse_;

	void						del( Animation * pAnimation );
	friend class Animation;

	AnimationManager(const AnimationManager&);
	AnimationManager& operator=(const AnimationManager&);
};

} // namespace Moo

#ifdef CODE_INLINE
#include "animation_manager.ipp"
#endif

BW_END_NAMESPACE

#endif // ANIMATION_MANAGER_HPP
