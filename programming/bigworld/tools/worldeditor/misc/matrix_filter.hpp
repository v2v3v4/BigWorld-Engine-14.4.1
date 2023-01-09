#ifndef MATRIX_FILTER_HPP
#define MATRIX_FILTER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

BW_BEGIN_NAMESPACE

/**
 *  Manages WE's terrain height filters 
 */
class MatrixFilter
{
public:
	struct FilterDef
	{
		FilterDef();
		void clear();

		float				constant_;
		float				noise_;
		int					noiseSizeX_;
		int					noiseSizeY_;
		float				strengthRatio_;
		BW::vector<float>	kernel_;
		int					kernelWidth_;
		int					kernelHeight_;
		float				kernelSum_;
		bool				included_;
		BW::string			name_;
	};

	static MatrixFilter& instance();

	size_t size() const;

	const FilterDef& filter( size_t index ) const;

private:
	MatrixFilter();
	bool init();

	bool inited_;

	typedef BW::vector<FilterDef> FilterDefVec;
	FilterDefVec filters_;
};

BW_END_NAMESPACE

#endif // MATRIX_FILTER_HPP
