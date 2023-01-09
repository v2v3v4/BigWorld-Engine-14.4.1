#ifndef RESOURCE_MODIFICATION_LISTENER_HPP
#define RESOURCE_MODIFICATION_LISTENER_HPP

#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bgtask_manager.hpp"

BW_BEGIN_NAMESPACE

class ResourceModificationListener
{
public:
	virtual ~ResourceModificationListener() {}

	enum Action
	{
		ACTION_ADDED,
		ACTION_DELETED,
		ACTION_MODIFIED,
		ACTION_MODIFIED_DELETED
	};

	virtual void onResourceModified(
		const BW::StringRef& basePath,
		const BW::StringRef& resourceID,
		Action modType ) = 0;

	void cancelReloadTasks();

	class ReloadTask : public BackgroundTask
	{
	public:
		ReloadTask( const BW::StringRef& basePath, 
			const BW::StringRef& resourceID,
			ResourceModificationListener* owner );
		virtual ~ReloadTask();

		virtual void doBackgroundTask( TaskManager & mgr );
		virtual void doMainThreadTask( TaskManager & mgr );

		const BW::string& basePath() const;
		const BW::string& resourceID() const;

	protected:
		virtual void executeReload() = 0;

		ResourceModificationListener* pOwner_;
		BW::string basePath_;
		BW::string resourceID_;
		uint32 timesRequeued_;
	};
	typedef SmartPointer<ReloadTask> ReloadTaskPtr;

protected:

	template <class ReloadContextType>
	class ReloadTask_Impl : public ReloadTask
	{
	public:
		ReloadTask_Impl(const ReloadContextType& context,
			const BW::StringRef& basePath, 
			const BW::StringRef& resourceID,
			ResourceModificationListener* owner ) :
			ReloadTask( basePath, resourceID, owner ),
			context_( context )
		{}
		
	protected:

		virtual void executeReload()
		{
			context_.reload( *this );
		}

	private:
		ReloadContextType context_;
	};

	template <class ReloadContextType>
	void queueReloadTask( const ReloadContextType& context,
		const BW::StringRef& basePath, 
		const BW::StringRef& resourceID  )
	{
		queueReloadTask( new ReloadTask_Impl<ReloadContextType>( context,
			basePath, resourceID, this ) );
	}


private:

	void notifyCompletedTask( ReloadTask* pTask );
	void queueReloadTask( const ReloadTaskPtr& task );
	
	typedef BW::vector< ReloadTaskPtr > TaskList;
	TaskList pendingTasks_;
};

typedef BW::vector<ResourceModificationListener*> ModificationListeners;

BW_END_NAMESPACE

#endif // RESOURCE_MODIFICATION_LISTENER_HPP
