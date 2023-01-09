#ifndef VISUAL_MANIPULATOR_ANIMATION_HPP
#define VISUAL_MANIPULATOR_ANIMATION_HPP

#include "math/matrix.hpp"
#include "cstdmf/smartpointer.hpp"

#include <utility>
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

namespace VisualManipulator
{
	class Animation : public ReferenceCount
	{
	public:
		Animation();
		typedef BW::vector<Matrix> MatrixVector;
		
		void addChannel( const BW::string& identifier, const MatrixVector& transforms );
		void name( const BW::string& name ) { name_ = name; }

		bool save( const BW::string& filename );

	private:
		typedef std::pair<BW::string, MatrixVector> Channel;
		typedef BW::vector<Channel> Channels;

		Channels channels_;
		BW::string name_;

		uint32 frameCount_;
	};
}

BW_END_NAMESPACE

#endif // VISUAL_MANIPULATOR_ANIMATION_HPP
