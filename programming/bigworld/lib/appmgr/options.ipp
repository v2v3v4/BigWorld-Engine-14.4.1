
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

BW_BEGIN_NAMESPACE

/**
 *	This method returns the root section of options tree.
 */
INLINE
DataSectionPtr Options::pRoot()
{
	instance().syncRoot();
	return instance().pRootSection_;
}

BW_END_NAMESPACE

/*options.ipp*/
