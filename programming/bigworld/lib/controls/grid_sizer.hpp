#ifndef CONTROLS_GRID_SIZER_HPP
#define CONTROLS_GRID_SIZER_HPP

#include "sizer.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{
    /**
     *  A GridSizer keeps a grid of child sizers.
     */
    class GridSizer : public Sizer
    {
    public:
        enum SizeUnits
        {
            PIXELS,             // size is pixels
            WEIGHT              // size is a weight
        };

        GridSizer(uint32 columns, uint32 rows);
		GridSizer
		(
			uint32				columns, 
			uint32				rows, 
			CWnd				*container
		);
		GridSizer
		(
			uint32				columns, 
			uint32				rows, 
			CWnd				*parent, 
			uint32				containerID
		);
        /*virtual*/ ~GridSizer();

        void setChild
        (
            uint32				column,
            uint32				row,
            SizerPtr			child
        );

        void setColumnSize(uint32 col, uint32 sz, SizeUnits units);
        void setRowSize   (uint32 row, uint32 sz, SizeUnits units);

        uint32 columns() const;
        uint32 rows() const;

        uint32 leftEdgeGap  () const;
		uint32 topEdgeGap   () const;
		uint32 rightEdgeGap () const;
		uint32 bottomEdgeGap() const;
        void leftEdgeGap  (uint32 gap);   
		void topEdgeGap   (uint32 gap);
        void rightEdgeGap (uint32 gap);		
		void bottomEdgeGap(uint32 gap);

        uint32 gap() const;
        void gap(uint32 g);

        /*virtual*/ void onSize(CRect const &extents);
        /*virtual*/ CSize minimumSize() const;
        /*virtual*/ void draw(CDC *dc);

    protected:
		void create(uint32 width, uint32 height);

		void destroy();

        CWnd *window();

    private:
        struct SizeInfo
        {
            uint32				size_;
            SizeUnits           units_;
            SizeInfo();
        };

        CWnd                	*wnd_;
        uint32        			id_;        
        uint32            		leftEdgeGap_;
		uint32            		topEdgeGap_;
		uint32            		rightEdgeGap_;
		uint32            		bottomEdgeGap_;
		uint32            		gap_;
        uint32            		columns_;
        uint32            		rows_;
        SizerPtr                *children_;
        BW::vector<SizeInfo>   colInfoSizes_;
        BW::vector<SizeInfo>   rowInfoSizes_;
    };
}

BW_END_NAMESPACE

#endif // CONTROLS_GRID_SIZER_HPP
