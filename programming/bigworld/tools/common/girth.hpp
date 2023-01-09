#ifndef GIRTH_HPP
#define GIRTH_HPP


#include <algorithm>
#include "cstdmf/bw_vector.hpp"

#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

class Girth
{
public:
	Girth( DataSectionPtr ds );

	float girth() const { return girth_; }
	float width() const { return width_; }
	float height() const { return height_; }
	float depth() const { return depth_; }
	float radius() const { return std::min<float>( width_, depth_ ) * 0.5f; }
	float maxSlope() const { return maxSlope_; }
	float maxClimb() const { return maxClimb_; }
	bool always() const { return always_; }

private:
	float girth_;
	float width_;
	float height_;
	float depth_;
	float maxSlope_;
	float maxClimb_;
	bool always_;
};


class Girths
{
	BW::vector<Girth> girths_;

public:
	Girths();

	size_t size() const { return girths_.size(); }
	const Girth& operator[]( size_t index ) const;
};

BW_END_NAMESPACE

#endif //GIRTH_HPP
