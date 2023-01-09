#ifndef VISUAL_MANAGER_HPP
#define VISUAL_MANAGER_HPP

#include "cstdmf/bw_map.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/smartpointer.hpp"

#include "visual.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	This singleton class keeps track of and loads Visuals.
 */
class VisualManager:
	public ResourceModificationListener
{
public:
	typedef StringRefMap< Visual* > VisualMap;

	~VisualManager();

	static VisualManager* instance();

	static void init();
	static void fini();

	VisualPtr get( const BW::StringRef& resourceID, bool loadIfMissing = true );

	void fullHouse( bool noMoreEntries = true );

	void add( Visual * pVisual, const BW::StringRef & resourceID );
protected:
	void onResourceModified(
		const BW::StringRef& basePath,
		const BW::StringRef& resourceID,
		ResourceModificationListener::Action action);

private:
	VisualManager();
	VisualManager(const VisualManager&);
	VisualManager& operator=(const VisualManager&);

	static void del( Visual* pVisual );
	void delInternal( Visual* pVisual );

	VisualPtr find( const BW::StringRef & resourceID );

	VisualMap visuals_;
	SimpleMutex	visualsLock_;

	bool fullHouse_;

	static VisualManager* pInstance_;

	friend Visual::~Visual();
};

} // namespace Moo

#ifdef CODE_INLINE
#include "visual_manager.ipp"
#endif

BW_END_NAMESPACE


#endif // VISUAL_MANAGER_HPP
