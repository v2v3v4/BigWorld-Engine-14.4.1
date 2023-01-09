#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


/**
 *	Return the primary model, is present and a model.
 */
INLINE PyModel * Entity::pPrimaryModel() const
{
	BW_GUARD;
	if (primaryEmbodiment_)
	{
		return ScriptObjectPtr< PyModel >::create( 
			primaryEmbodiment_->scriptObject() ).get();
	}
	else
	{
		return NULL;
	}
}


/* entity.ipp */
