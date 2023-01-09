// console_manager.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

/**
 *	This method returns the current active console. If no console is active,
 *	NULL is returned.
 */
INLINE XConsole * ConsoleManager::pActiveConsole() const
{
	return pActiveConsole_;
}


/**
 *	This static method returns the singleton instance of this class.
 */
INLINE ConsoleManager & ConsoleManager::instance()
{
	SINGLETON_MANAGER_WRAPPER( ConsoleManager )
	MF_ASSERT(s_instance_);
	return *s_instance_;
}

// console_manager.ipp
