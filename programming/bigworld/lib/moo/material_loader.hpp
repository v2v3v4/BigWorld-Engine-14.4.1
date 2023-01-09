#ifndef MATERIAL_LOADER_HPP
#define MATERIAL_LOADER_HPP


BW_BEGIN_NAMESPACE

namespace Moo
{

class Material;

/**
 *	This class wraps a method that processes a certain type
 *	of data section in a material file.
 */
class SectionProcessor
{
public:
	typedef bool (*Method)( Material& mat, DataSectionPtr pSect );

	SectionProcessor( Method m ) : method_( m ) {}

	Method operator *() { return method_; }

private:
	Method	method_;
};

typedef std::pair<const char *,SectionProcessor::Method> SectProcEntry;

/**
 *	This class maintains the map of all section processors.
 */
class SectionProcessors : public StringHashMap<SectionProcessor>
{
public:
	static SectionProcessors& instance();
};

} // namespace Moo

BW_END_NAMESPACE

#endif
