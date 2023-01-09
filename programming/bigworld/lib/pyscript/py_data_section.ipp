// py_data_section.ipp

#ifdef CODE_INLINE
    #define INLINE    inline
#else
    #define INLINE
#endif


INLINE DataSectionPtr PyDataSection::pSection() const
{
	return pSection_;
}

// py_data_section.ipp
