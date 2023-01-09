#include "pch.hpp"
#include "semi_dynamic_shadow.hpp"

#include "cstdmf/watcher.hpp"
#include "cstdmf/watcher_path_request.hpp"

#include "space/space_manager.hpp"
#include "space/client_space.hpp"
#include "space/deprecated_space_helpers.hpp"
#include "scene/scene.hpp"
#include "scene/change_scene_view.hpp"
#include "scene/scene_draw_context.hpp"
#include "scene/draw_helpers.hpp"
#include "scene/intersection_set.hpp"
#include "scene/scene_intersect_context.hpp"

#include "moo_utilities.hpp"
#include "viewport_setter.hpp"
#include "effect_visual_context.hpp"
#include "effect_material.hpp"
#include "image_texture.hpp"
#include "fullscreen_quad.hpp"
#include "nvapi_wrapper.hpp"
#include "render_event.hpp"
#include "complex_effect_material.hpp"
#include "semi_dynamic_shadow_scene_view.hpp"

#include "cstdmf/vectornodest.hpp"
#include "romp/enviro_minder.hpp"
#include "romp/time_of_day.hpp"

#include "moo/draw_context.hpp"
#include "speedtree/speedtree_renderer.hpp"
#include "sticker_decal.hpp"

#include "romp/sun_and_moon.hpp"

#include "duplo/pymodel.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// - Data Structure
// -----------------------------------------------------------------------------
//
// The main classes that make up the backbone of the whole data structure of the
// module are:
//
// - DivisionData.
//      A linear array with the size equal to the area of the covered space.
//      The method to compare each division of covered area with additional some
//      unique for each division data used in this module only to
//      drop and receive shadows. Here, in particular, each active division maps
//      the coordinate of its square on the shadow map to its coordinate
//      in the world space.
//      Any division to which the mapping set up considered
//      drawn on the shadow map or ready for rendering.
//
// - WorldDivisionArrangement
//      Mapping world coordinates division with its index in the structure
//      DivisionData.
//
// - ShadowMapDivisionArrangement
//      Mapping coordinates of a square slot of shadow atlas with its
//      index in the structure DivisionData.
//
// - CoveredArea
//      World coordinates of the current the covered area.
//
// Using the above representation of the data can be easily and clearly express
// algorithms for treating a variety of situations that should handle the
// module:
//
//      - Move the covered area. If the camera is in the corner of the map,
//      cover the area has decreased.
//
//      Make the class CoveredArea.
//
//      - Load/unload divisions. Can occur at any time with any divisions.
//
//      Methods Impl::updateIngoingDivisions and Impl::updateoutgoingDivisions
//      iterate the space and set the connection between DivisionData,
//      WorldDivisionArrangement and ShadowMapDivisionArrangement depending on 
//      the status of the division (loaded or not loaded).
//
//      - Destruction of objects (including trees). Moving the individual
//      objects in the editor. Must be updated as well as the division, who owns
//      the object, and the divisions to which this object has the potential to
//      cast a shadow.
//
//      Methods:
//      * UpdateArea. Staging of the division in turn, must be called by
//      third-party modules.
//      * Impl::updateDivisionQueue. The calculation of divisions which are 
//      occluders (including itself) and directed them to the queue for 
//      rendering.
//
//      - Move the sun. To the Editor. It is necessary to update all the
//      divisions.
//

//-----------------------------------------------------------------------------
//-- constants
//-----------------------------------------------------------------------------

namespace 
{
    //-- constants

    /// It is used in the calculation of division crop-matrixs.
    /// Now for all the divisions used one and same limits visibility.
    const float g_cropZn = 0.f; // can be used unnecessarily 0.f orthogonal
                                // projection
    const float g_cropZf = 3000.f;

    /// It is used in the calculation of the matrix view-sun,
    /// the distance of the sun from the center of the world.
    const float g_lightDist = 1500.f;

    /// Time at which the divisions redraw the shadow of the request for renewal.
    float g_divisionUpdateTime = 2.f;

    /// On/Off update the shadow map in background.
    bool g_enableSemiUpdate = true;

    /// If on a background update the shadow map, you will only be updated
    /// terrain.
    bool g_updateTerrainOnly = true;

    int g_updatePerimeter = 2; // 3;

    /// The minimum number of SceneObjects to be drawn in a single frame
    /// When updating the shadows.
    size_t g_updateMinItemsPerFrame = 50;

    /// Guaranteed number of frames that will be drawn for one division
    /// When updating the shadow map. Takes precedence over
    /// g_updateMinItemsPerFrame.
    size_t g_updateDivisionMaxFrames = 7;

    /// The size of the shadow map.
    const int SM_RT_SIZE = 4096;

    const int SM_BORDER_SIZE = 1;

    bool g_isFirstTime = true;
}


enum QUALITY_SM
{
    QL_HIGH,
    QL_LOW,
    QL_TOTAL
};

enum UPDATE_PRIORITY 
{
    U_PRIORITY_HIGH  = 0,
    U_PRIORITY_NORMAL  = 1,
    U_PRIORITY_DYNAMIC = 2,
    U_PRIORITY_LOW     = 10000,
};

enum UPDATE_ITEMS
{
    UI_TERRAIN = 1,
    UI_OBJECTS = 1 << 1,
    UI_CLEAR   = 1 << 2,
    UI_QUALITY = 1 << 3,
};

enum CUQ_FLAGS
{
    CUQ_UPDATE = 1,
    CUQ_BIND   = 1 << 1,
    CUQ_UNBIND = 1 << 2,
};


//-----------------------------------------------------------------------------
//-- helper functions
//-----------------------------------------------------------------------------
namespace 
{
   //----------------------------------------------------------------------------------------------
    // Calculate data divisions need for casting.
    // As long as it's only crop-matrix.
    void _calcCropProjectionMatrix( const BoundingBox& divisionBounds, 
        const Matrix& sunViewMatrix, Matrix& outSunCropMatrix )
    {
        BW_GUARD;
        
        MF_ASSERT( !divisionBounds.insideOut() );

        BoundingBox bb = divisionBounds;
        bb.transformBy(sunViewMatrix);

        Matrix translate;
        Matrix scale;

        translate.setIdentity();
        scale.setIdentity();

        // "Target" camera on the bounding box.
        translate.translation(Vector3(
            -bb.centre().x, 
            -bb.centre().y,
            0.0f)
        );

        // "Squeezed" size of the camera the size of the bounding box.
        scale.orthogonalProjection( bb.width(), bb.height(), g_cropZn, g_cropZf );

        outSunCropMatrix.setIdentity();
        outSunCropMatrix.postMultiply(translate);
        outSunCropMatrix.postMultiply(scale);
    }
    
    //----------------------------------------------------------------------------------------------

} // unnamed namespace

//-----------------------------------------------------------------------------
//-- Auxilary
//-----------------------------------------------------------------------------

namespace 
{
    const size_t kMaxSize = std::numeric_limits<size_t>::max();
    const int kMaxInt = std::numeric_limits<int32>::max();

    // Class to represent the coordinates of the divisions in the world space.

    // We are using grid space defined by the SemiDynamicShadowView implemented
    // on the scene of the current space. This can be based on the chunk space
    // layout. But it is not necessary to be such.
    //
    // (x, y)
    //               /\ X in Grid Space, X in World Space
    //           ... |
    //    ...  (2, 0)|
    //  (1, 1) (1, 0)|
    //  (0, 1) (0, 0)|
    // < - - - - - - *
    // Y in Grid Space, Z in World Space

    class WorldCoord : 
        public Moo::IntVector2
    {
    public:
        WorldCoord() :
            Moo::IntVector2( kMaxInt, kMaxInt )
        {
        }

        WorldCoord( int32 x, int32 y ) :
            Moo::IntVector2( x, y )
        {
        }

        bool isValid() const
        {
            return x != kMaxInt && 
                y != kMaxInt;
        }
    };

    
    // Class to represent the coordinates of the squares clicking on the map
    // shadows.

    // * - - - - > x
    // | (0, 0) (1, 0) (2, 0)
    // | (0, 1) (1, 1) ...
    // | (2, 0) ...
    // | ...
    // \/
    // y
    //

    struct ShadowMapCoord
    {
        ShadowMapCoord()
            : x( kMaxSize )
            , y( kMaxSize )
            , quality( kMaxSize )
        {
        }

        ShadowMapCoord( size_t x, size_t y, size_t qual )
            : x(x)
            , y(y)
            , quality(qual)
        {
        }

        bool isValid() const
        {
            return x != kMaxSize && y != kMaxSize && quality != kMaxSize;
        }

        bool operator == ( const ShadowMapCoord& other ) const
        {
            return ( x == other.x ) && ( y == other.y ) && ( quality == other.quality );
        }

        size_t x;
        size_t y;
        size_t quality;
    };

} // namespace

//-----------------------------------------------------------------------------
//-- DivisionData
//-----------------------------------------------------------------------------

namespace 
{
    //----------------------------------------------------------------------------------------------

    /**
     *  A class for storing data associated with the divisions that are in the
     *  covered area.
     *  Data is stored in a linear array. Accessed via index.
     *  The cell array may or may not contain data divisions (to be free).
     *  A cell array containing the data division can be connected or not
     *  connected to the world
     *  coordinates and the coordinates of the division to division the 
     *  shadow map.
     */
    class DivisionData
    {
        struct PrivateData;

    public:
        DivisionData(size_t size);
        ~DivisionData();

        struct Data
        {
            PrivateData* nextFree;
            BoundingBox  visibilityBox;
            Matrix       sunCropMatrix;
        };

        /// Write the data in the cell index.
        size_t bind( const BoundingBox& visibilityBox, 
            const Matrix& sunCropMatrix );

        /// Update crop-matrix for the division.
        void updateSunCropMatrix( size_t index, const Matrix& sunCropMatrix );

        /// Release the cell index.
        void unbind( size_t index );

        /// Return data cell.
        const Data& getData( size_t index ) const;
        
        /// Assign a cell with world coordinates.
        void setWorldCoord( size_t index, const WorldCoord& worldCoord );
        const WorldCoord& getWorldCoord( size_t index ) const;

        /// Assign the slot with these coordinates in the shadow atlas.
        void setShadowMapCoord( size_t index, const ShadowMapCoord& shadowMapCoord );
        const ShadowMapCoord& getShadowMapCoord( size_t index ) const;

    private:

        struct PrivateData
        {
            PrivateData()
            {
                data.nextFree = NULL;
            }

            Data data;

            /// World coordinates of division.
            WorldCoord worldCoord;

            /// Coordinates of slot in the shadow map.
            ShadowMapCoord shadowMapCoord;

            size_t index;
        };

        PrivateData* m_freeList;
        BW::vector<PrivateData> m_data;
    };

    //----------------------------------------------------------------------------------------------

    DivisionData::DivisionData( size_t size )
    {
        BW_GUARD;
        m_freeList = NULL;
        m_data.resize(size * size);

        BW::vector<PrivateData>::reverse_iterator data_it = m_data.rbegin();
        for ( ; data_it != m_data.rend(); ++data_it )
        {
            data_it->data.nextFree = m_freeList;
            m_freeList = &*data_it;
        }
    }

    DivisionData::~DivisionData()
    {
    }

    size_t DivisionData::bind( const BoundingBox& visibilityBox, 
        const Matrix& sunCropMatrix )
    {
        BW_GUARD;
        MF_ASSERT( m_freeList != NULL );

        PrivateData& data  = *m_freeList;
        PrivateData* begin = &m_data.front();
        m_freeList = m_freeList->data.nextFree;

        data.data.visibilityBox = visibilityBox;
        data.data.sunCropMatrix = sunCropMatrix;
        data.worldCoord = WorldCoord();
        data.shadowMapCoord = ShadowMapCoord();
        data.index = std::distance( begin, &data );
        
        return data.index;
    }

    void DivisionData::unbind( size_t index )
    {
        BW_GUARD;
        MF_ASSERT( index < m_data.size() );

        PrivateData& d = m_data[index];

        d.data.visibilityBox = BoundingBox();
        d.data.sunCropMatrix = Matrix();
        d.worldCoord = WorldCoord();
        d.shadowMapCoord = ShadowMapCoord();

        d.data.nextFree = m_freeList;
        m_freeList = &d;
    }

    const DivisionData::Data& DivisionData::getData( size_t index ) const
    {
        BW_GUARD;
        MF_ASSERT( index < m_data.size() );

        return m_data[index].data;
    }

    void DivisionData::updateSunCropMatrix( size_t index, 
        const Matrix& sunCropMatrix )
    {
        BW_GUARD;
        MF_ASSERT( index < m_data.size() );

        m_data[index].data.sunCropMatrix = sunCropMatrix;
    }

    void DivisionData::setWorldCoord( size_t index, 
        const WorldCoord& worldCoord )
    {
        BW_GUARD;

        MF_ASSERT(worldCoord.isValid());
        m_data[index].worldCoord = worldCoord;
    }

    const WorldCoord& DivisionData::getWorldCoord(size_t index) const
    {
        BW_GUARD;

        return m_data[index].worldCoord;
    }

    void DivisionData::setShadowMapCoord( size_t index, 
        const ShadowMapCoord& shadowMapCoord )
    {
        BW_GUARD;

        MF_ASSERT(shadowMapCoord.isValid());
        m_data[index].shadowMapCoord = shadowMapCoord;
    }

    const ShadowMapCoord& DivisionData::getShadowMapCoord( size_t index ) const
    {
        BW_GUARD;

        return m_data[index].shadowMapCoord;
    }

    //----------------------------------------------------------------------------------------------
}

//-----------------------------------------------------------------------------
//-- WorldDivisionArrangement
//-----------------------------------------------------------------------------

namespace 
{
    //----------------------------------------------------------------------------------------------

    /**
     *  The data structure to display divisions world coordinates and their data in
     *  DivisionData.
     *
     *  Array which arrange division data as it was placed in the world.
     *  It links world's division cells with our division data indices.
     */
    class WorldDivisionArrangement
    {
    public:
        WorldDivisionArrangement();
        ~WorldDivisionArrangement();

        void init( const Moo::IntBoundBox2& divisions );

        /// Returns true if the division with the worldCoord coordinates
        /// has data?
        bool isHasDivisionData(WorldCoord worldCoord) const;

        /// Give the data division that has coordinates worldCoord.
        size_t getDivisionData(WorldCoord worldCoord) const;

        /// Link the world coordinates of division with division data.
        void setDivisionData(WorldCoord worldCoord, size_t divisionData);
        void unsetDivisionData(WorldCoord worldCoord);

        /// The minimum and maximum coordinates of the division space.
        WorldCoord min() const { return m_min; }
        WorldCoord max() const { return m_max; }

    private:
        size_t getIndexByWorldCoord(const WorldCoord& coord) const;
        void validateCoord(const WorldCoord& coord) const;

        // our coord boundaries (min bounds are zeros)
        WorldCoord m_min;
        WorldCoord m_max;

        // data array

        // WorldCoord <=> m_data index
        //
        //               /\ X
        //               |
        //      8  5  2  |
        //      7  4  1  |
        //      6  3  0  |
        // < - - - - - - * 
        // Y

        BW::vector<size_t> m_data;
    };

    //----------------------------------------------------------------------------------------------

    WorldDivisionArrangement::WorldDivisionArrangement()
    {
        Moo::IntBoundBox2 defaultBounds;
        defaultBounds.max_.x = 0;
        defaultBounds.max_.y = 0;
        defaultBounds.min_.x = 0;
        defaultBounds.min_.y = 0;
        init( defaultBounds );
    }

    void WorldDivisionArrangement::init( const Moo::IntBoundBox2& divisions )
    {
        BW_GUARD;

        // division space coord boundaries
        m_min.x = divisions.min_.x;
        m_min.y = divisions.min_.y;
        m_max.x = divisions.max_.x;
        m_max.y = divisions.max_.y;

        MF_ASSERT(m_min.x <= m_max.x);
        MF_ASSERT(m_min.y <= m_max.y);

        int w = m_max.x - m_min.x + 1; 
        int h = m_max.y - m_min.y + 1;

        MF_ASSERT(w > 0 && h > 0);

        m_data.resize((size_t) (w * h));

        for(size_t i = 0; i < m_data.size(); ++i)
        {
            m_data.at(i) = kMaxSize;
        }
    }

    WorldDivisionArrangement::~WorldDivisionArrangement()
    {
    }

    bool WorldDivisionArrangement::isHasDivisionData(WorldCoord worldCoord) const
    {
        BW_GUARD;

        return m_data.at(getIndexByWorldCoord(worldCoord)) != kMaxSize;
    }

    size_t WorldDivisionArrangement::getDivisionData(WorldCoord worldCoord) const
    {
        BW_GUARD;

        size_t ret = m_data.at(getIndexByWorldCoord(worldCoord));
        MF_ASSERT(ret != kMaxSize);
        return ret;
    }

    void WorldDivisionArrangement::setDivisionData(WorldCoord worldCoord, size_t divisionData)
    {
        BW_GUARD;

        MF_ASSERT(divisionData != kMaxSize);
        m_data.at(getIndexByWorldCoord(worldCoord)) = divisionData;
    }

    void WorldDivisionArrangement::unsetDivisionData(WorldCoord worldCoord)
    {
        BW_GUARD;

        m_data.at(getIndexByWorldCoord(worldCoord)) = kMaxSize;
    }

    size_t WorldDivisionArrangement::getIndexByWorldCoord(const WorldCoord& coord) const
    {
        BW_GUARD;

        validateCoord(coord);

        int x = coord.x - m_min.x;
        int y = coord.y - m_min.y;
        int h = m_max.x - m_min.x + 1;

        MF_ASSERT(x >= 0 && y >= 0 && h >= 0);

        int index = h * y + x;

        MF_ASSERT(index >= 0);

        return (size_t) index;
    }

    void WorldDivisionArrangement::validateCoord(const WorldCoord& coord) const
    {
        BW_GUARD;

        MF_ASSERT(coord.isValid());
        MF_ASSERT(m_min.isValid() && m_max.isValid());
        MF_ASSERT(m_min.x <= coord.x && coord.x <= m_max.x);
        MF_ASSERT(m_min.y <= coord.y && coord.y <= m_max.y);
    }

    //----------------------------------------------------------------------------------------------

} // namespace

//-----------------------------------------------------------------------------
//-- CoveredArea
//-----------------------------------------------------------------------------

namespace 
{
    //----------------------------------------------------------------------------------------------

    /**
     *  Class to represent the current the covered area.
     *  Covering of the area - it's a lot of divisions, which are subject to
     *  semi-dynamic shadows.
     *
     *  Holds and updates current coverage area.
     *  Covered area it is an area of divisions which are covered by semi-dynamic
     *  shadows mechanism.
     */
    class CoveredArea
    {
    public:

        struct QualityData
        {
            QualityData() 
                : areaSize( 0 )
                , wc_min( 0, 0 )
                , wc_max( 0, 0 )
            {}

            size_t      areaSize;
            WorldCoord  wc_min;
            WorldCoord  wc_max;
        };

        CoveredArea(size_t lowSize)
            : m_center(kMaxInt, kMaxInt) // max initial values cause the full recalculation 
                                         // of the covered area during first invocation of 
                                         // the update() method
        {
            addQuality( QL_LOW, lowSize );
        }

        ~CoveredArea()
        {
        }

        /// Refresh the current scope if necessary.
        /// Returns true, if the area has been updated.
        /// False otherwise.
        bool update(const WorldDivisionArrangement& worldDivisionArrangement);

        /// Coordinates the division, which is considered the centerpiece.
        /// This division is the one the camera is in.
        const WorldCoord& center() const { return m_center; }

        const WorldCoord& min() const { return m_quality[QL_LOW].wc_min; }
        const WorldCoord& max() const { return m_quality[QL_LOW].wc_max; }

        /// Calculate the priority of this division according to 
        /// its distance from the camera.
        uint32 priority( const WorldCoord & wc ) const
        {
            // Basic distance calculation from the camera
            return U_PRIORITY_DYNAMIC + 
                ( center().x - wc.x ) * ( center().x - wc.x ) + 
                ( center().y - wc.y ) * ( center().y - wc.y );
        }

        /// Calculate the quality level based upon the distance of the
        /// division from the camera.
        uint32 quality( const WorldCoord & wc ) const 
        { 
            for ( int i = 0; i < QL_TOTAL; ++i )
            {
                const QualityData& d = m_quality[i];
                if ( d.areaSize == 0 )
                    continue;

                if( d.wc_min.x <= wc.x && wc.x <= d.wc_max.x && 
                    d.wc_min.y <= wc.y && wc.y <= d.wc_max.y )
                {
                    return i;
                }
            }

            return QL_LOW;
        }

        void addQuality( int quality, size_t size )
        {
            MF_ASSERT( quality < QL_TOTAL );

            m_quality[quality].areaSize = size;
        }

        bool isInArea(const WorldCoord& worldCoord) const;

        void invalidateCenter()
        {
            m_center.x = kMaxInt;
            m_center.y = kMaxInt;
        }

        bool isCenterValid() const
        {
            return m_center.x != kMaxInt && m_center.y != kMaxInt;
        }

    private:
        QualityData  m_quality[QL_TOTAL];
        WorldCoord   m_center;
    };

    //----------------------------------------------------------------------------------------------
    inline int pointToGrid( float point, float gridSize )
    {
        return static_cast<int>( floorf( point / gridSize ) );
    }

    bool CoveredArea::update(const WorldDivisionArrangement& wca)
    {
        BW_GUARD;

        ClientSpacePtr cs = DeprecatedSpaceHelpers::cameraSpace();
        MF_ASSERT(cs);
        Moo::SemiDynamicShadowSceneView* pShadowView = 
            cs->scene().getView<Moo::SemiDynamicShadowSceneView>();

        bool isUpdated = false;

        float gridSize;
        Moo::IntBoundBox2 gridBounds;
        pShadowView->getDivisionConfig( gridBounds, gridSize );

        // Check if the camera is in the current division specified
        // by the center variable.
        Vector3 cameraPos = Moo::rc().invView().applyToOrigin();

        // Ignore the Y coordinate, the bounding box below is always set to
        // center around +-1.0f in Y so it doesn't affect the result.
        cameraPos.y = 0.0f;

        BoundingBox centerBb(
            Vector3(
                gridSize * m_center.x, 
                -1.0f, 
                gridSize * m_center.y
            ),
            Vector3(
                gridSize * (m_center.x + 1), 
                1.0f, 
                gridSize * (m_center.y + 1)
            )
        );
        
        if( !centerBb.intersects( cameraPos, 10.f ) )
        {
            m_center.x = pointToGrid( cameraPos.x, gridSize );
            m_center.y = pointToGrid( cameraPos.z, gridSize );

            for ( int i = 0; i < QL_TOTAL; ++i )
            {
                int d_max = int(m_quality[i].areaSize / 2);
                int d_min = int( m_quality[i].areaSize % 2 == 1 
                    ? (m_quality[i].areaSize / 2)
                    : (m_quality[i].areaSize / 2) - 1 );

                int maxX = m_center.x + d_max;
                int maxY = m_center.y + d_max;
                int minX = m_center.x - d_min;
                int minY = m_center.y - d_min;

                m_quality[i].wc_max.x = std::min(wca.max().x, maxX);
                m_quality[i].wc_max.y = std::min(wca.max().y, maxY);
                m_quality[i].wc_min.x = std::max(wca.min().x, minX);
                m_quality[i].wc_min.y = std::max(wca.min().y, minY);
            }

            isUpdated = true;
        }
    
        return isUpdated;
    }

    bool CoveredArea::isInArea(const WorldCoord& worldCoord) const
    {
        BW_GUARD;

        MF_ASSERT(worldCoord.isValid());

        const QualityData& d = m_quality[QL_LOW];

        return d.wc_min.x <= worldCoord.x && worldCoord.x <= d.wc_max.x &&
               d.wc_min.y <= worldCoord.y && worldCoord.y <= d.wc_max.y;
    }

    //----------------------------------------------------------------------------------------------

} // namespace

//-----------------------------------------------------------------------------
//-- ShadowMapAtlasArrangement
//-----------------------------------------------------------------------------

namespace 
{
    //----------------------------------------------------------------------------------------------

    // Array which arrange division data as it was placed on the shadow map.
    // It links atlas slots on the shadow map with our division data indices.
    // 
    // ShadowMapCoord layout:
    // 
    // * - - - - > x
    // | (0, 0) (1, 0) (2, 0)
    // | (0, 1) (1, 1) ...
    // | (2, 0) ...
    // | ...
    // \/
    // y
    //
    class ShadowMapAtlasArrangement
    {
    public:

        struct ShadowMapQualityArea
        {
            RECT                       atlasRect;
            size_t                     atlasSlotWidth;
            size_t                     atlasSlotHeight;
            BW::vector<size_t>         atlasSlotData;
            BW::deque<ShadowMapCoord> freeCoords;
        };

        ShadowMapAtlasArrangement( size_t atlasW, size_t atlasH );
        ~ShadowMapAtlasArrangement();

        bool isHasDivisionData( ShadowMapCoord shadowMapCoord ) const;
        size_t getDivisionData( ShadowMapCoord shadowMapCoord ) const;

        void setDivisionData(ShadowMapCoord shadowMapCoord, size_t divisionDataIndex);
        void unsetDivisionData(ShadowMapCoord shadowMapCoord);

        ShadowMapCoord findFree( size_t quality );

        void addQuality( 
            size_t quality,
            const RECT& texRect, 
            size_t slotWidth, 
            size_t slotHeight 
        )
        {
            MF_ASSERT( quality < QL_TOTAL );

            ShadowMapQualityArea& ql = m_quality[quality];

            ql.atlasRect    = texRect;
            ql.atlasSlotWidth  = slotWidth;
            ql.atlasSlotHeight = slotHeight;
            ql.atlasSlotData.assign( slotWidth * slotHeight, kMaxSize );

            size_t rtSize = ( texRect.right - texRect.left ) / slotWidth;
            if ( m_rtSize < rtSize )
                m_rtSize = rtSize;
            
            ql.freeCoords.clear();

            ShadowMapCoord coord;
            coord.quality = quality;
            for( coord.x = 0; coord.x < slotWidth; ++coord.x )
            {
                for( coord.y = 0; coord.y < slotHeight; ++coord.y )
                {
                    ql.freeCoords.push_back( coord );
                }
            }
        }

        size_t atlasWidth() const { return m_atlasWidth; }
        size_t atlasHeight() const { return m_atlasHeight; }

        const RECT& atlas( size_t quality ) const
        {
            MF_ASSERT( quality < QL_TOTAL );
            return m_quality[quality].atlasRect;
        }

        size_t slotWidth( size_t quality ) const
        { 
            MF_ASSERT( quality < QL_TOTAL );
            return m_quality[quality].atlasSlotWidth;
        }

        size_t slotHeight( size_t quality ) const 
        { 
            MF_ASSERT( quality < QL_TOTAL );
            return m_quality[quality].atlasSlotHeight;
        }

        size_t rtSize() const 
        {
            return m_rtSize;
        }

    private:
        size_t getIndexByShadowMapCoord(const ShadowMapCoord& coord) const;

        size_t m_rtSize;
        size_t m_atlasWidth;
        size_t m_atlasHeight;
        ShadowMapQualityArea m_quality[QL_TOTAL];
    };

    //----------------------------------------------------------------------------------------------

    ShadowMapAtlasArrangement::ShadowMapAtlasArrangement( 
        size_t atlasW, size_t atlasH )
        : m_atlasWidth( atlasW )
        , m_atlasHeight( atlasH )
        , m_rtSize( 0 )
    {
    }

    ShadowMapAtlasArrangement::~ShadowMapAtlasArrangement()
    {
    }

    bool ShadowMapAtlasArrangement::isHasDivisionData(
        ShadowMapCoord shadowMapCoord) const
    {
        BW_GUARD;

        return m_quality[shadowMapCoord.quality].atlasSlotData.at(
            getIndexByShadowMapCoord(shadowMapCoord)) != kMaxSize;
    }

    size_t ShadowMapAtlasArrangement::getDivisionData(
        ShadowMapCoord shadowMapCoord) const
    {
        BW_GUARD;

        size_t ret = m_quality[shadowMapCoord.quality].atlasSlotData.at(
            getIndexByShadowMapCoord(shadowMapCoord));
        MF_ASSERT(ret != kMaxSize);
        return ret;
    }

    void ShadowMapAtlasArrangement::setDivisionData
        (ShadowMapCoord shadowMapCoord, size_t divisionDataIndex)
    {
        BW_GUARD;

        m_quality[shadowMapCoord.quality].atlasSlotData.at(
            getIndexByShadowMapCoord(shadowMapCoord)) = divisionDataIndex;
    }

    void ShadowMapAtlasArrangement::unsetDivisionData(
        ShadowMapCoord shadowMapCoord)
    {
        BW_GUARD;

        m_quality[shadowMapCoord.quality].atlasSlotData.at(
            getIndexByShadowMapCoord(shadowMapCoord)) = kMaxSize;
        m_quality[shadowMapCoord.quality].freeCoords.push_back( shadowMapCoord );
    }

    size_t ShadowMapAtlasArrangement::getIndexByShadowMapCoord(
        const ShadowMapCoord& coord) const
    {
        BW_GUARD;

        MF_ASSERT( coord.isValid() );
        MF_ASSERT( coord.x < slotWidth( coord.quality ) );
        MF_ASSERT( coord.y < slotHeight( coord.quality ) );

        return coord.y * slotWidth( coord.quality ) + coord.x;
    }

    ShadowMapCoord ShadowMapAtlasArrangement::findFree( size_t quality )
    {
        BW_GUARD;
        MF_ASSERT( quality < QL_TOTAL );

        ShadowMapCoord coord;
        bool success = false;
        for ( size_t i = quality; i < QL_TOTAL; ++i )
        {
            ShadowMapQualityArea& ql = m_quality[i];
            if ( ql.freeCoords.empty() )
                continue;
            
            coord = ql.freeCoords.front();
            ql.freeCoords.pop_front();
            success = true;
            break;
        }

        MF_ASSERT( success && "findFree fail" );
        return coord;
    }

    //----------------------------------------------------------------------------------------------

} // namespace

using namespace Moo;
//-----------------------------------------------------------------------------

struct ShadowMapUpdateData
{
    ShadowMapCoord coord;
    int            priority;
    int            updateFlags;

    bool operator == ( const ShadowMapCoord& other ) const
    {
        return coord == other;
    }

    bool operator < ( const ShadowMapUpdateData& other ) const
    {
        return this->priority < other.priority;
    }
    
    void updatePriority( int pr )
    {
        if ( priority >= U_PRIORITY_DYNAMIC || priority > pr )
        {
            priority = pr;
        }
    }
};


class ShadowMapUpdateQueue 
    : public BW::deque<ShadowMapUpdateData>
{
public:

    void insert( const ShadowMapCoord & coord, int priority )
    {
        insert( coord, UI_OBJECTS | UI_TERRAIN, priority );
    }

    void insert( const ShadowMapCoord & coord, int flags, int priority )
    {
        iterator iter = find( coord );
        if ( iter == end() )
        {
            ShadowMapUpdateData data;
            data.coord       = coord;
            data.updateFlags = flags;
            data.priority    = priority;

            push_back( data );
        }
        else
        {
            ( *iter ).updateFlags |= flags;
            ( *iter ).updatePriority( priority );
        }

        std::sort( begin(), end() );
    }

    bool update( const ShadowMapCoord & coord, int flags, int priority )
    {
        iterator iter = find( coord );
        
        if ( iter != end() )
        {
            ( *iter ).updateFlags |= flags;
            ( *iter ).updatePriority( priority );

            std::sort( begin(), end() );
            return true;
        }

        return false;
    }

    const_iterator find(const ShadowMapCoord & coord) const
    {
        return std::find( begin(), end(), coord );
    }
    
    iterator find(const ShadowMapCoord & coord)
    {
        return std::find( begin(), end(), coord );
    }
};


/**
 *  Represents a queue for updating divisions on demand.
 */
class DivisionUpdateQueue
{
public:
    
    struct UpdateAreaData
    {
        AABB m_updatedArea;

        // Delta of the time (constantly incremented).
        float m_time;
        int   m_flags;

        bool operator == ( const AABB& bb ) const
        {
            return m_updatedArea == bb;
        }
    };

    typedef BW::deque<UpdateAreaData> AreaDataDeque;

    void update( float dTime )
    {
        AreaDataDeque::iterator dataIt = m_queue.begin();
        for ( ; dataIt != m_queue.end(); ++dataIt )
        {
            ( *dataIt ).m_time += dTime;
        }
    }

    bool nextItem( int* flags, AABB* updatedArea )
    {
        if ( !m_queue.empty() )
        {
            UpdateAreaData& data = m_queue.front();
            if ( data.m_time > g_divisionUpdateTime )
            {
                *updatedArea = data.m_updatedArea;
                *flags       = data.m_flags;

                m_queue.pop_front();
                return true;
            }
        }
        return false;
    }

    bool empty() const 
    {
        return m_queue.empty();
    }

    void insert( const AABB& updatedArea, int flags )
    {
        AreaDataDeque::iterator queueIt = 
            std::find( m_queue.begin(), m_queue.end(), updatedArea );
        if ( queueIt == m_queue.end() )
        {
            UpdateAreaData data;
            data.m_updatedArea    = updatedArea;
            data.m_time     = 0.f;
            data.m_flags = flags;
            m_queue.push_back( data );
        }
        else
        {
            ( *queueIt ).m_flags |= flags;
        }
    }

    void clear()
    {
        m_queue.clear();
    }

    AreaDataDeque m_queue;
};

struct SemiDivisionRenderer
{
    enum 
    {
        CullItems = 1,
        CastItems = 1 << 1,

        Completed = CullItems | CastItems,
    };

    size_t       m_frames;
    uint         m_state;
    ShadowMapUpdateData m_updateData;
    size_t       m_currentType;
    size_t       m_index;
    Matrix m_lightProjection;
    IntersectionSet m_castingTerrain;
    IntersectionSet m_castingObjects;


    SemiDivisionRenderer() 
    {
        clear();
    }

    ~SemiDivisionRenderer()
    {
        BW_GUARD;
        clear();
    }

    bool completed() const
    {
        return m_state == Completed;
    }

    void init( const ShadowMapUpdateData & data, size_t countFrame )
    {
        m_currentType = 0;
        m_index = 0;
        m_updateData = data;
        m_frames = countFrame;
        m_state  = 0;

        m_castingTerrain.clear();
        m_castingObjects.clear();
        m_lightProjection.setIdentity();
    }

    void render( const WorldCoord& wc, 
        Moo::DrawContext& drawContext, 
        WorldDivisionArrangement& wca, 
        Moo::SemiDynamicShadowSceneView* pShadowView,
        Moo::EffectMaterialPtr& terrainEM )
    {
        if ( !( m_state & CullItems ) )
            this->cull( wc, pShadowView, wca, Moo::rc().viewProjection() );
        else
            this->cast( drawContext, terrainEM );
    }

    void clear()
    {
        BW_GUARD;

        m_currentType = 0;
        m_index  = 0;
        m_frames = 1;
        m_state  = Completed;

        m_castingTerrain.clear();
        m_castingObjects.clear();
        m_lightProjection.setIdentity();
    }

    const ShadowMapUpdateData* data() const
    {
        return &m_updateData;
    }

    bool isCastingArea( const AABB& bounds )
    {
        if (m_state == Completed)
        {
            return false;
        }

        Outcode oc;
        Outcode combinedOc;
        bounds.calculateOutcode( m_lightProjection, oc, combinedOc );
        return !combinedOc;
    }

private:
    struct RemoveNonShadowAndSeparateTerrainFilter
    {
        RemoveNonShadowAndSeparateTerrainFilter( IntersectionSet& terrainSet ) :
            m_castingTerrain( terrainSet )
        {
        }

        IntersectionSet& m_castingTerrain;

        bool operator()(const SceneObject& obj) const
        {
            if (!obj.flags().isCastingStaticShadow())
            {
                // remove it
                return true;
            }

            // If it is terrain, add to other list and remove it
            if (obj.flags().isTerrain())
            {
                m_castingTerrain.insert( obj );
                return true;
            }
            
            return false;
        }
    };

    void cull( const WorldCoord& wc, 
        Moo::SemiDynamicShadowSceneView* pShadowView,
        WorldDivisionArrangement& wca, const Matrix& projection )
    {
        m_lightProjection = projection;

        BW_SCOPED_DOG_WATCHER( "CullItems" );

        if ( m_state & CullItems )
            return;

        ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();
        Scene& scene = pSpace->scene();

        SceneIntersectContext cullContext( scene );
        DrawHelpers::cull( cullContext, scene, projection, m_castingObjects );

        // Now iterate over all items culled, and sort them into non
        // shadow casters, and terrain objects
        RemoveNonShadowAndSeparateTerrainFilter postFilter( m_castingTerrain );
        for (uint32 typeIndex = 0; typeIndex < m_castingObjects.maxType(); 
            ++typeIndex)
        {
            IntersectionSet::SceneObjectList& objects =
                m_castingObjects.objectsOfType( typeIndex );

            objects.erase(std::remove_if( objects.begin(), objects.end(), 
                postFilter ), objects.end() );
        }

        // Fetch all casters that affect the shadow frustum
        //pShadowView->getCastersAffectingDivision( wc, m_lightProjection,
        //  m_castingObjects );

        m_state |= CullItems;
    }

    bool cast( Moo::DrawContext& drawContext, Moo::EffectMaterialPtr& terrainEM )
    {
        BW_SCOPED_DOG_WATCHER( "CastItems" );

        ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();
        Scene& scene = pSpace->scene();
        SceneDrawContext sceneContext( scene, drawContext );

        PyStickerModel::s_enabledDrawing = false;

        // First iterate through and find all the terrain we wish to render
        if (m_updateData.updateFlags & UI_TERRAIN &&
            m_castingTerrain.numObjects() > 0)
        {
            Moo::rc().push();

            Terrain::BaseTerrainRenderer* pTerrainRenderer = 
                Terrain::BaseTerrainRenderer::instance();
            pTerrainRenderer->clearBlocks();
            
            DrawHelpers::drawSceneVisibilitySet( sceneContext, scene,
                m_castingTerrain );

            // Push terrain batches
            pTerrainRenderer->drawAll( Moo::RENDERING_PASS_SHADOWS, terrainEM ); 

            // Clear all terrain blocks.
            pTerrainRenderer->clearBlocks();

            m_castingTerrain.clear();

            Moo::rc().pop();
        }

#if SPEEDTREE_SUPPORT
        // Now render all objects as well
        if (m_updateData.updateFlags & UI_OBJECTS &&
            m_castingObjects.numObjects() > 0)
        {
            EnviroMinder* em = &DeprecatedSpaceHelpers::cameraSpace()->enviro();
            speedtree::SpeedTreeRenderer::beginFrame( em, 
                Moo::RENDERING_PASS_SHADOWS, Moo::rc().view()); // lod camera is not used
            float oldTreeLodeMode = speedtree::SpeedTreeRenderer::lodMode( 1.0f );

            Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE );
            Moo::rc().setWriteMask( 0, D3DCOLORWRITEENABLE_GREEN );

            Moo::rc().push();

            drawContext.begin( Moo::DrawContext::OPAQUE_CHANNEL_MASK );

            DrawOperation* pDrawOp =
                pSpace->scene().getObjectOperation<DrawOperation>();
            MF_ASSERT( pDrawOp != NULL );
            
            // We need to draw only a few objects per frame. 
            // So we make sure we limit how many we render each time.
            size_t maxObjects = m_castingObjects.numObjects();
            size_t maxObjectsDrawnPerFrame = 
                std::max( 1 + maxObjects / m_frames, g_updateMinItemsPerFrame );

            size_t objectsLeftToDraw = maxObjectsDrawnPerFrame;

            for (;m_currentType < m_castingObjects.maxType() && 
                objectsLeftToDraw > 0; ++m_currentType)
            {
                const IntersectionSet::SceneObjectList& objects =
                    m_castingObjects.objectsOfType( 
                        (SceneTypeSystem::RuntimeTypeID)m_currentType );

                if (objects.empty())
                {
                    continue;
                }

                size_t objectsToDraw = std::min( 
                    objects.size() - m_index, objectsLeftToDraw );
                
                pDrawOp->drawType( sceneContext, 
                    (SceneTypeSystem::RuntimeTypeID)m_currentType,
                    &objects[m_index], objectsToDraw );

                objectsLeftToDraw -= objectsToDraw;
                m_index += objectsToDraw;
                MF_ASSERT( m_index <= objects.size() );
                if (m_index == objects.size())
                {
                    // Going to move onto the next type
                    m_index = 0;
                }
                else
                {
                    // Couldn't do all the items in this iteration.
                    break;
                }
            }

            Moo::rc().pop();

            drawContext.end( Moo::DrawContext::OPAQUE_CHANNEL_MASK );
            drawContext.flush( Moo::DrawContext::OPAQUE_CHANNEL_MASK );

            speedtree::SpeedTreeRenderer::endFrame();
            speedtree::SpeedTreeRenderer::lodMode(oldTreeLodeMode);

            Moo::rc().popRenderState();

            if ( m_currentType >= m_castingObjects.maxType())
            {
                m_state |= CastItems;
                m_castingObjects.clear();
            }
        }
        else
        {
            m_state |= CastItems;
        }
#endif // SPEEDTREE_SUPPORT

        PyStickerModel::s_enabledDrawing = true;

        return true;
    }
};

//-----------------------------------------------------------------------------

// It is expected that the m_divisionSize value will change to be a valid value
// once the updateConfig function has run and fetched the correct configuration 
// for the current space.
const float ARBITRARY_DIVISION_SIZE_INITIAL_VALUE = 100.0f;

/**
 *  Implementation of the SemiDynamicShadows class.
 *  Owns all resources.
 *  Implements the helper methods.
 */
struct SemiDynamicShadow::Impl : 
    public Moo::DeviceCallback,
    public ChangeSceneViewListener
{
public:
    Impl( uint coveredAreaSize, uint shadowMapSize, bool debugMode )
        : m_coveredAreaSize(coveredAreaSize)
        , m_shadowMapSize(shadowMapSize)
        , m_debugMode(debugMode)
        , m_divisionData(m_coveredAreaSize)
        , m_worldDivisionArrangement()
        , m_shadowMapAtlasArrangement( shadowMapSize, shadowMapSize )
        , m_coverageArea(m_coveredAreaSize)
        , m_needsUpdateIndirectionTextures(false)
        , m_atlasInited(false)
        , m_firstComeGPUIndex(-1)
        , m_sunDir( Vector3::ZERO )
        , m_divisionSize( ARBITRARY_DIVISION_SIZE_INITIAL_VALUE )
        , m_currentSpace( 0 )
        , m_recentLoadEvent( false )
#ifdef EDITOR_ENABLED
        , m_generationFinishedCallback( NULL )
#endif
    {
        BW_GUARD;

        MF_ASSERT( m_coveredAreaSize > 0 );

        RECT rect;
        rect.left    = 0;
        rect.top     = 0;
        rect.right   = shadowMapSize / 2;
        rect.bottom  = shadowMapSize / 2;

        m_shadowMapAtlasArrangement.addQuality( QL_LOW, rect, 
            m_coveredAreaSize, m_coveredAreaSize );
        m_coverageArea.addQuality( QL_LOW, m_coveredAreaSize );

        rect.left    = 0;
        rect.top     = 0;
        rect.right   = shadowMapSize / 2;
        rect.bottom  = shadowMapSize / 2;

        m_shadowMapAtlasArrangement.addQuality( QL_HIGH, rect, 
            m_coveredAreaSize / 2, m_coveredAreaSize / 2 );
        m_coverageArea.addQuality( QL_HIGH, m_coveredAreaSize / 2 );

        createManagedObjects();
        createUnmanagedObjects();
    }

    ~Impl()
    {
        BW_GUARD;
        removeSceneListener();

        deleteManagedObjects();
        deleteUnmanagedObjects();
    }

    //-- vars

    const uint m_coveredAreaSize;
    const uint m_shadowMapSize;
    const bool m_debugMode;

    //-- data

    DivisionData m_divisionData;
    WorldDivisionArrangement m_worldDivisionArrangement;
    ShadowMapAtlasArrangement m_shadowMapAtlasArrangement;
    CoveredArea m_coverageArea;

    typedef Moo::ImageTexture<Vector4> IndirectionMap;
    typedef Moo::ImageTexture<DWORD> IndirectionMapArgb;

    static const uint32 NUM_INDIRECTION_STAGING_TEXTURES = 4;
    typedef SmartPointer<IndirectionMap> IndirectionMapPtr;
    typedef SmartPointer<IndirectionMapArgb> IndirectionMapArgbPtr;

    /// Texture stores the parameters to restore crop-matrix in the shader.
    IndirectionMapPtr m_indirectionMapProjStagingPtr[NUM_INDIRECTION_STAGING_TEXTURES];

    /// Texture stores the settings for the recovery of the texture coordinates
    /// of a square slot in the shadow map.
    IndirectionMapArgbPtr m_indirectionMapTcStagingPtr[NUM_INDIRECTION_STAGING_TEXTURES];
    uint32 m_currentIndirectionTextureSet;

    IndirectionMapPtr m_indirectionMapProjPtr;
    IndirectionMapArgbPtr m_indirectionMapTcPtr;

    /// Queued updating squares shadow maps.
    ShadowMapUpdateQueue m_shadowMapUpdateQueue;

    /// Update flag Indirection-textures.
    /// Set when the texture needs to be regenerated.
    /// Are updated both immediately and in full.
    bool m_needsUpdateIndirectionTextures; 
    bool m_atlasInited;
    int m_firstComeGPUIndex;
#ifdef EDITOR_ENABLED
    GenerationFinishedCallback m_generationFinishedCallback;
#endif

    SemiDivisionRenderer m_divisionRenderer;

    /// The divisions are from outside the requested update.
    DivisionUpdateQueue m_divisionUpdateQueue;

    //-- other

    Matrix  m_sunViewMatrix;
    Vector3 m_sunDir;

    //-- materials

    Moo::EffectMaterialPtr m_receiveEM;
    Moo::EffectMaterialPtr m_terrainCastEM;

    //-- resources

    Moo::RenderTargetPtr m_shadowMapAtlas[QL_TOTAL];
    Moo::RenderTargetPtr m_shadowMapRT;
    Moo::RenderTargetPtr m_emptyRT;

    bool m_recentLoadEvent;
    SpaceID m_currentSpace;
    float m_divisionSize;

    Moo::SemiDynamicShadowSceneView* m_pShadowView;

    //-- DeviceCallback implementations
    virtual void deleteUnmanagedObjects();
    virtual void createUnmanagedObjects();
    virtual void createManagedObjects();
    virtual void deleteManagedObjects();
    virtual bool recreateForD3DExDevice() const;

    //-- Change scene view listener implementations
    virtual void onAreaLoaded( const AABB & bb );
    virtual void onAreaUnloaded( const AABB & bb );
    virtual void onAreaChanged( const AABB & bb );

    //-- tick

    /**
     * Handling space changes and layout size changes
     */
    bool updateConfig();
    void removeSceneListener();

    /**
     *  If you want to move the covered area.
     *  If the move occurs, set the update flag indirection-textures.
     */
    void updateCoverageArea();

    /**
     *  Handling bias of the Sun.
     *  If it happened, then you need to convert the data in all of the divisions.
     */
    void updateSunViewMatrix();

    /**
     *  Process the bounding boxes that have been queued for update
     *  via SemiDynamicShadow::updateArea. All divisions are evaluated
     *  to check if the updated area casts upon them, and then are queued for
     *  an update if necessary.
     */
    void updateDivisionQueue( float dTime );

    /**
     *  Put on a queue for updating those squares shadow maps,
     *  divisions which have gone beyond the covered area.
     */
    void updateOutgoingDivisions(); 

    /**
     *  Queue to update those squares shadow map
     *  divisions which were included in the limits of the covered area.
     */
    void updateIngoingDivisions();

    /**
     *  Updating the quality of the division shadow maps.
     */
    void updateQualityDivisions();

    //-- other

    /**
     *  Break all the bindings all data divisions.
     *  Installs all the divisions in the queue for renewal in subsequent call
     *  Method updateIngoingDivisions ().
     */
    void resetAll();

    /**
     *  Drawing and updating one division.
     */
    void renderDivision( Moo::DrawContext& shadowDrawContext );

    /**
     *  The background update.
     */
    void idle();

    void copyRTToAtlas( const ShadowMapCoord & smc );
    RECT rectFromCoord( const ShadowMapCoord & coord );
    void copyAtlasToRT( const ShadowMapCoord & coord );
    void copyAtlasToAtlas( const ShadowMapCoord & dst, const ShadowMapCoord & src );

    void unbindArea( const AABB & bb );

    Vector3 retriveLightDir();

#ifdef EDITOR_ENABLED
    void setGenerationFinishedCallback(
        GenerationFinishedCallback callback );
    void callGenerationFinishedCallback();
#endif
};

//-----------------------------------------------------------------------------
bool SemiDynamicShadow::Impl::recreateForD3DExDevice() const
{
    return true;
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::deleteUnmanagedObjects()
{
    BW_GUARD;

    for ( int i = 0; i < NUM_INDIRECTION_STAGING_TEXTURES; ++i )
    {
        m_indirectionMapProjStagingPtr[i] = NULL;
        m_indirectionMapTcStagingPtr[i]   = NULL;
    }

    m_indirectionMapProjPtr = NULL;
    m_indirectionMapTcPtr   = NULL;

    for ( int i = 0; i < QL_TOTAL; ++i )
    {
        m_shadowMapAtlas[i] = NULL;
    }

    m_shadowMapRT = NULL;
    m_emptyRT = NULL;
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::createUnmanagedObjects()
{
    BW_GUARD;

    bool isOk = true;

    m_currentIndirectionTextureSet = 0;
    for ( int i = 0; i < NUM_INDIRECTION_STAGING_TEXTURES; ++i )
    {
        m_indirectionMapProjStagingPtr[i] = new IndirectionMap(m_coveredAreaSize, 
            m_coveredAreaSize, D3DFMT_A32B32G32R32F, D3DUSAGE_DYNAMIC, D3DPOOL_SYSTEMMEM, 1);
        m_indirectionMapTcStagingPtr[i]   = new IndirectionMapArgb(m_coveredAreaSize,
            m_coveredAreaSize, D3DFMT_A8R8G8B8,  D3DUSAGE_DYNAMIC, D3DPOOL_SYSTEMMEM, 1);
    }

    m_indirectionMapProjPtr = new IndirectionMap(m_coveredAreaSize, 
        m_coveredAreaSize, D3DFMT_A32B32G32R32F, 0, D3DPOOL_DEFAULT, 1);
    m_indirectionMapTcPtr = new IndirectionMapArgb(m_coveredAreaSize,
        m_coveredAreaSize, D3DFMT_A8R8G8B8,  0, D3DPOOL_DEFAULT, 1);

    for ( int i = 0; i < QL_TOTAL; ++i )
    {
        isOk &= createRenderTarget( m_shadowMapAtlas[i], 
            m_shadowMapSize / 2, 
            m_shadowMapSize / 2, 
            D3DFMT_G32R32F, bw_format( "SemiDynamicAtlas_%i", i ).c_str(), true );
    }
    
    isOk &= createRenderTarget( m_shadowMapRT, 
        uint( m_shadowMapAtlasArrangement.rtSize() ), 
        uint( m_shadowMapAtlasArrangement.rtSize() ), 
        D3DFMT_G32R32F, "SemiDynamicRenderTarget", false, false );

    isOk &= createRenderTarget( m_emptyRT, 
        SM_BORDER_SIZE*2 + 2, 
        SM_BORDER_SIZE*2 + 2, 
        D3DFMT_G32R32F, "SemiDynamicEmptyRenderTarget", false, false );

    if(isOk && m_emptyRT->push())
    {
        uint32 clearFlags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
        if (Moo::rc().stencilAvailable())
        {
            clearFlags |= D3DCLEAR_STENCIL;
        }
        Moo::rc().device()->Clear( 0, NULL, clearFlags, 0xFFFFFFFF, 1.f, 0 );
        m_emptyRT->pop();
    }

    MF_ASSERT( isOk && "Not all resources initialized proprly." );

    // m_shadowMapAtlas->clearOnRecreate( true, 0xFFFFFFFF );

    resetAll(); // we should refill all RT's after creating
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::createManagedObjects()
{
    BW_GUARD;

    bool isOk = true;

    isOk &= Moo::createEffect(m_receiveEM, "shaders/shadow/semi_resolve.fx");
    isOk &= Moo::createEffect(m_terrainCastEM, "shaders/shadow/semi_terrain2_cast.fx");

    MF_ASSERT(isOk && "Not all resources initialized proprly.");
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::deleteManagedObjects()
{
    BW_GUARD;

    m_receiveEM = NULL;
    m_terrainCastEM = NULL;
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::removeSceneListener()
{
    // Do this by fetching the old space first and detaching (otherwise
    // it may have already been removed and not exist anymore)
    ClientSpacePtr oldSpace = SpaceManager::instance().space( m_currentSpace );
    if (oldSpace)
    {
        ChangeSceneView* pChangeView = 
            oldSpace->scene().getView<ChangeSceneView>();
        pChangeView->removeListener( this );
    }
}

//-----------------------------------------------------------------------------
bool SemiDynamicShadow::Impl::updateConfig()
{
    bool requiresReset = false;
    bool divisionArrangementChanged = false;
    bool configurationValid = true;

    // Update division dimensions
    ClientSpacePtr cs = DeprecatedSpaceHelpers::cameraSpace();
    if (m_currentSpace != cs->id())
    {
        // This is a new space.
        // Better disconnect from old change view.
        removeSceneListener();
        // Now add a listener to the new space.
        ChangeSceneView* pChangeView = cs->scene().getView<ChangeSceneView>();
        pChangeView->addListener( this );

        requiresReset = true;
    }

    if (!requiresReset && !m_recentLoadEvent)
    {
        // No changes required
        return true;
    }

    // Now to query the layout of the divisions in this space 
    // (possible load event changed it).
    m_pShadowView = 
        cs->scene().getView<Moo::SemiDynamicShadowSceneView>();

    Moo::IntBoundBox2 bounds;
    float divisionSize;
    if (!m_pShadowView->getDivisionConfig( bounds, divisionSize ))
    {
        // Couldn't query the scene to detect the configuration
        configurationValid = false;
    }
    else
    {
        m_divisionSize = divisionSize;

        // Check if the new builds require a rearrangement of the shadow maps
        divisionArrangementChanged = 
            (m_worldDivisionArrangement.max().x != bounds.max_.x ||
            m_worldDivisionArrangement.max().y != bounds.max_.y ||
            m_worldDivisionArrangement.min().x != bounds.min_.x ||
            m_worldDivisionArrangement.min().y != bounds.min_.y);
    }

    if (requiresReset || divisionArrangementChanged)
    {
        // Drop all divisions from the shadow atlas. 
        resetAll();
    }
    
    if (divisionArrangementChanged)
    {
        // Reset all must occur before this re-initialization
        m_worldDivisionArrangement.init( bounds );
    }

    if (configurationValid)
    {
        // Reset the flags that caused us to execute this update if config is
        // ok, otherwise keep checking config until it is valid
        m_recentLoadEvent = false;
        m_currentSpace = cs->id();
    }

    return configurationValid;
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::updateCoverageArea()
{
    BW_GUARD;

    // Check if the camera's position has moved and changed divisions
    if( m_coverageArea.update( m_worldDivisionArrangement ) )
    {
        m_needsUpdateIndirectionTextures = true;

        // Adjust the priorities of all the shadow atlas slots 
        // that are currently queued 
        // (we may have moved closer to them)
        ShadowMapUpdateQueue::iterator smuq_it = m_shadowMapUpdateQueue.begin();
        for ( ; smuq_it != m_shadowMapUpdateQueue.end(); ++smuq_it )
        {
            if ( m_shadowMapAtlasArrangement.isHasDivisionData( (*smuq_it).coord ) )
            {
                size_t divisionDataIndex = 
                    m_shadowMapAtlasArrangement.getDivisionData( (*smuq_it).coord );
                
                // Check if it has been queued with a dynamic priority
                if ( (*smuq_it).priority != U_PRIORITY_LOW )
                {
                    (*smuq_it).updatePriority( 
                        m_coverageArea.priority( 
                            m_divisionData.getWorldCoord( divisionDataIndex ) ) );
                }
            }
        }

        std::sort( m_shadowMapUpdateQueue.begin(), m_shadowMapUpdateQueue.end() );
    }
}

//-----------------------------------------------------------------------------
Vector3 SemiDynamicShadow::Impl::retriveLightDir()
{
    BW_GUARD;

    Vector3 lightDir(0.f, 0.f, 0.f);

    // calculate light's direction and position.
    ClientSpacePtr cs = SpaceManager::instance().space( m_currentSpace );
    if (cs) 
    {
        EnviroMinder& enviro =  cs->enviro();
        lightDir = enviro.timeOfDay()->lighting().mainLightDir();
        lightDir.normalise();
    }

    return lightDir;
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::updateSunViewMatrix()
{
    BW_GUARD;

    // The View matrix.
    // Always looks in the center of the world.
    // Removed from it at g_lightDist meters.
    // Direction coincides with the direction to the Sun.

    static const float e = 0.000001f;
    Vector3 dir = retriveLightDir();

    if ( !almostEqual( m_sunDir, dir, e ) )
    {
        m_sunDir = dir;

        Vector3 eye = - dir * g_lightDist;
        Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
        if((dir.x * dir.x + dir.z * dir.z) < 0.001) 
        {
            up = Vector3(0.f, 0.f, 1.f);
        }

        Matrix newSunViewMatrix;
        newSunViewMatrix.lookAt(eye, dir, up);
        
        m_sunViewMatrix = newSunViewMatrix;

        // If the Sun moved we need to update the shadow for all divisions.
        resetAll();
    }
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::updateDivisionQueue( float dTime )
{
    // Update queue processing divisions.
    BW_GUARD;
    
    m_divisionUpdateQueue.update( dTime );

    int    divisionFlags = false;
    BoundingBox updateBounds;
    if (!m_divisionUpdateQueue.nextItem( &divisionFlags, &updateBounds ))
    {
        return;
    }

    // Only one update is processed per updateDivisionQueue call. (i.e. per tick)
    // This update box is then compared against the shadow frustums of 
    // all divisions, to determine if any of them should be invalidated.

    for(int x = m_coverageArea.min().x; x <= m_coverageArea.max().x; ++x)
    {
        for(int y = m_coverageArea.min().y; y <= m_coverageArea.max().y; ++y)
        {
            WorldCoord wc(x, y);
            
            if( !m_worldDivisionArrangement.isHasDivisionData( wc ) )
            {
                continue;
            }

            size_t divisionDataIndex = m_worldDivisionArrangement.getDivisionData(wc);
            const DivisionData::Data& dd = m_divisionData.getData(divisionDataIndex);

            ShadowMapCoord smc = m_divisionData.getShadowMapCoord(divisionDataIndex);

            // If the division is already in the queue, then just update the drawing options.
            if( m_shadowMapUpdateQueue.update( smc, UI_OBJECTS | UI_TERRAIN, m_coverageArea.priority( wc ) ) )
            {
                if ( divisionFlags & CUQ_BIND )
                {
                    Matrix sunCropMatrix;
                    _calcCropProjectionMatrix( dd.visibilityBox, m_sunViewMatrix, sunCropMatrix );
                    m_divisionData.updateSunCropMatrix( divisionDataIndex, sunCropMatrix );
                    m_needsUpdateIndirectionTextures = true;
                }
                continue;
            }

            // Make sure the divisionData is valid
            MF_ASSERT(!dd.visibilityBox.insideOut());

            Matrix viewCropMatrix;
            viewCropMatrix.setIdentity();
            viewCropMatrix.multiply( m_sunViewMatrix, dd.sunCropMatrix );

            BoundingBox bb = updateBounds;
            if( bb.insideOut() ) // reconsider
            {
                continue;
            }

            bb.calculateOutcode( viewCropMatrix );

            if( !bb.combinedOutcode() )
            {
                if ( divisionFlags & CUQ_BIND )
                {
                    Matrix sunCropMatrix;
                    _calcCropProjectionMatrix( dd.visibilityBox, m_sunViewMatrix, sunCropMatrix );
                    m_divisionData.updateSunCropMatrix( divisionDataIndex, sunCropMatrix );
                    m_needsUpdateIndirectionTextures = true;
                }

                m_shadowMapUpdateQueue.insert( smc, m_coverageArea.priority( wc ) );
            }
        }
    }
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::unbindArea( const AABB & unboundBounds )
{
    BW_GUARD;

    WorldCoord wcaMin = m_worldDivisionArrangement.min();
    WorldCoord wcaMax = m_worldDivisionArrangement.max();

    for(int x = wcaMin.x; x <= wcaMax.x; ++x)
    {
        for(int y = wcaMin.y; y <= wcaMax.y; ++y)
        {
            WorldCoord wc(x, y);

            if( m_coverageArea.isInArea( wc ) && 
                m_worldDivisionArrangement.isHasDivisionData( wc ) )
            {
                // Fetch the bounding box this division is contained in
                size_t divisionDataIndex = m_worldDivisionArrangement.getDivisionData(wc);
                DivisionData::Data dd = m_divisionData.getData( divisionDataIndex );

                if (dd.visibilityBox.intersects( unboundBounds ))
                {
                    // Something in this region was unloaded

                    // No need to query the region for a new set of bounds
                    // The region will be removed from the shadow atlas
                    // and if it needs readding with different size data
                    // it will automatically do that in 
                    // the UpdateIncommingDivisions() call in tick

                    // Check if the renderer is currently casting objects
                    // that are potentially in this area. (unsafe if they've
                    // just been unloaded, so cancel that render and requeue)
                    if ( m_divisionRenderer.isCastingArea( unboundBounds ) )
                    {
                        ShadowMapUpdateData smc = *m_divisionRenderer.data();
                        if (smc.updateFlags & ( UI_OBJECTS | UI_TERRAIN ))
                        {
                            m_shadowMapUpdateQueue.insert( smc.coord, 
                                m_coverageArea.priority( wc ) );
                        }

                        m_divisionRenderer.clear();
                    }

                    // Now remove the shadow map from the atlas
                    ShadowMapCoord smc = m_divisionData.getShadowMapCoord(divisionDataIndex);

                    m_shadowMapAtlasArrangement.unsetDivisionData( smc );
                    m_worldDivisionArrangement.unsetDivisionData( wc );
                    m_divisionData.unbind( divisionDataIndex );

                    // division is deleted, you need to clear the area.
                    m_shadowMapUpdateQueue.insert( smc, UI_CLEAR, U_PRIORITY_HIGH );

                    m_needsUpdateIndirectionTextures = true;
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::updateOutgoingDivisions()
{
    BW_GUARD;

    //-- update divisions which have gone beyond the current coverage area

    WorldCoord wcaMin = m_worldDivisionArrangement.min();
    WorldCoord wcaMax = m_worldDivisionArrangement.max();

    for(int x = wcaMin.x; x <= wcaMax.x; ++x)
    {
        for(int y = wcaMin.y; y <= wcaMax.y; ++y)
        {
            WorldCoord wc(x, y);

            if( !m_coverageArea.isInArea( wc ) && 
                m_worldDivisionArrangement.isHasDivisionData( wc ) )
            {
                size_t divisionDataIndex = 
                    m_worldDivisionArrangement.getDivisionData(wc);
                ShadowMapCoord smc = 
                    m_divisionData.getShadowMapCoord(divisionDataIndex);

                // Remove this division from the shadow atlas
                m_shadowMapAtlasArrangement.unsetDivisionData(smc);
                m_worldDivisionArrangement.unsetDivisionData(wc);
                m_divisionData.unbind(divisionDataIndex);

                // Division is outside the covered area.
                m_shadowMapUpdateQueue.insert( smc, UI_CLEAR, U_PRIORITY_HIGH );

                m_needsUpdateIndirectionTextures = true;
            }
        }
    }
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::updateIngoingDivisions()
{
    BW_GUARD;

    //-- update divisions which have gone into the current coverage area
    //-- ... and currently loaded divisions which are in the area

    WorldCoord caMin = m_coverageArea.min();
    WorldCoord caMax = m_coverageArea.max();

    for ( int x = caMin.x; x <= caMax.x; ++x )
    {
        for ( int y = caMin.y; y <= caMax.y; ++y )
        {
            WorldCoord wc(x, y);

            if (!m_worldDivisionArrangement.isHasDivisionData(wc))
            {
                BoundingBox visibilityBox;
                m_pShadowView->getDivisionBounds( wc, visibilityBox );
                
                //-- TODO(a_cherkes):
                // at this point, all casters of division can be uploaded yet
                // So the list of casters should periodically recompute
                //--

                if (visibilityBox.insideOut()) 
                {
                    // Nothing in this division
                    continue;
                }

                ShadowMapCoord shadowMapCoord = 
                    m_shadowMapAtlasArrangement.findFree( m_coverageArea.quality( wc ) );
                Matrix sunCropMatrix;

                _calcCropProjectionMatrix( visibilityBox, m_sunViewMatrix, sunCropMatrix );

                size_t divisionDataIndex = m_divisionData.bind( visibilityBox, sunCropMatrix );
                m_divisionData.setShadowMapCoord( divisionDataIndex, shadowMapCoord );
                m_divisionData.setWorldCoord( divisionDataIndex, wc );

                m_worldDivisionArrangement.setDivisionData( wc, divisionDataIndex );
                m_shadowMapAtlasArrangement.setDivisionData( shadowMapCoord, divisionDataIndex );

                m_shadowMapUpdateQueue.insert( shadowMapCoord, m_coverageArea.priority( wc ) );
                m_needsUpdateIndirectionTextures = true;
            }
        }
    }
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::updateQualityDivisions()
{
    BW_GUARD;

    //-- update divisions which have gone beyond the current coverage area

    WorldCoord wcaMin = m_worldDivisionArrangement.min();
    WorldCoord wcaMax = m_worldDivisionArrangement.max();

    for(int x = wcaMin.x; x <= wcaMax.x; ++x)
    {
        for(int y = wcaMin.y; y <= wcaMax.y; ++y)
        {
            WorldCoord wc(x, y);

            if( m_worldDivisionArrangement.isHasDivisionData( wc ) )
            {
                size_t divisionDataIndex = m_worldDivisionArrangement.getDivisionData( wc );
                ShadowMapCoord smc = m_divisionData.getShadowMapCoord( divisionDataIndex );

                size_t quality = m_coverageArea.quality( wc );
                if ( smc.quality != quality )
                {
                    if ( smc.quality > quality )
                    {
                        // From low to high resolution.
                        m_shadowMapUpdateQueue.insert( smc, UI_QUALITY, m_coverageArea.priority( wc ) );
                    }
                    else
                    {
                        // From high to low resolution. U_PRIORITY_NORMAL to
                        // release the space shadow map in high resolution.
                        m_shadowMapUpdateQueue.insert( smc, UI_QUALITY, U_PRIORITY_NORMAL );
                    }
                }
            }
        }
    }
}


//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::resetAll()
{
    BW_GUARD;

    WorldCoord wcaMin = m_worldDivisionArrangement.min();
    WorldCoord wcaMax = m_worldDivisionArrangement.max();

    for(int x = wcaMin.x; x <= wcaMax.x; ++x)
    {
        for(int y = wcaMin.y; y <= wcaMax.y; ++y)
        {
            WorldCoord wc(x, y);

            if( m_worldDivisionArrangement.isHasDivisionData( wc ) )
            {
                size_t divisionDataIndex = m_worldDivisionArrangement.getDivisionData(wc);
                ShadowMapCoord smc = m_divisionData.getShadowMapCoord(divisionDataIndex);

                m_shadowMapAtlasArrangement.unsetDivisionData(smc);
                m_worldDivisionArrangement.unsetDivisionData(wc);
                m_divisionData.unbind(divisionDataIndex);
            }
        }
    }

    m_atlasInited = false;
    m_firstComeGPUIndex = -1;
    m_divisionRenderer.clear();
    m_divisionUpdateQueue.clear();
    m_shadowMapUpdateQueue.clear();
    m_coverageArea.invalidateCenter();
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::idle()
{
    ShadowMapUpdateQueue& smuq = m_shadowMapUpdateQueue;
    if ( smuq.empty() && g_enableSemiUpdate )
    {
        // Fill the list for updates.
        const WorldCoord center = m_coverageArea.center();

        if ( !m_debugMode )
        {
            int perimiter = g_updatePerimeter;

            // In sniper mode, perimeter can be increased.
            if ( Moo::rc().lodZoomFactor() > 1.f )
                perimiter += 2;

            for ( int x = -perimiter; x < ( perimiter + 1 ); ++x )
            {
                for ( int y = -perimiter; y < ( perimiter + 1 ); ++y )
                {
                    if ( perimiter > 2 && 
                        ( x > -perimiter && x < perimiter ) && 
                        ( y > -perimiter && y < perimiter ) )
                    {
                        continue;
                    }

                    WorldCoord coord = center;
                    WorldCoord maxC  = m_worldDivisionArrangement.max();
                    WorldCoord minC  = m_worldDivisionArrangement.min();

                    coord.x += x;
                    coord.y += y;

                    if (coord.x > maxC.x || 
                        coord.y > maxC.y || 
                        coord.x < minC.x || 
                        coord.y < minC.y )
                    {
                        continue;
                    }

                    if ( m_worldDivisionArrangement.isHasDivisionData( coord ) )
                    {
                        size_t index = m_worldDivisionArrangement.getDivisionData( coord );
                        const ShadowMapCoord& shadowMapCoord = m_divisionData.getShadowMapCoord( index );
                        int flags = UI_TERRAIN;
                        if ( !g_updateTerrainOnly )
                            flags |= UI_OBJECTS;

                        smuq.insert( shadowMapCoord, flags, U_PRIORITY_LOW );
                    }
                }
            }
        }
        else
        {
            // In debug mode, only update the division the camera is currently in.
            if ( m_worldDivisionArrangement.isHasDivisionData( center ) )
            {
                size_t index = m_worldDivisionArrangement.getDivisionData( center );
                const ShadowMapCoord& shadowMapCoord = m_divisionData.getShadowMapCoord( index );
                
                int flags = UI_TERRAIN;
                if ( !g_updateTerrainOnly )
                    flags |= UI_OBJECTS;

                smuq.insert( shadowMapCoord, flags, U_PRIORITY_LOW );
            }
        }
    }
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::renderDivision( Moo::DrawContext& shadowDrawContext )
{
    BW_GUARD;
    BW_SCOPED_DOG_WATCHER( "Update" );

    Moo::rc().dynamicShadowsScene(true);

    int curSLI_AFRIndex = 0;

    // Clean all the atlases on the first access. NOTE: for each video card!
    if ( !m_atlasInited )
    {
        if( m_shadowMapRT->push() )
        {
            // Failed to update the Atlas when returning to the GPU which started
            if( m_firstComeGPUIndex == curSLI_AFRIndex)
            {
                m_atlasInited = true;
            }
            else
            {
                // Remember the number of the first new GPU group
                if(m_firstComeGPUIndex < 0)
                    m_firstComeGPUIndex = curSLI_AFRIndex;

                uint32 clearFlags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
                if (Moo::rc().stencilAvailable())
                {
                    clearFlags |= D3DCLEAR_STENCIL;
                }
                Moo::rc().device()->Clear( 0, NULL, clearFlags, 0xFFFFFFFF, 1.f, 0 );
                
                for ( int i = 0; i < QL_TOTAL; ++i )
                {
                    Moo::copyRT( m_shadowMapAtlas[i].get(), NULL, 
                        m_shadowMapRT.get(), NULL, D3DTEXF_POINT );
                }
            }

            m_shadowMapRT->pop();
        }
    }

    static ShadowMapCoord smCoord1;
    static ShadowMapCoord smCoord2;
    static bool doCopyRTToAtlas = false;
    static bool doCopyAtlasToAtlas = false;
    static bool doClearRT = false;
    ShadowMapUpdateQueue& smuq = m_shadowMapUpdateQueue;

    // Update m_shadowMapRT on only the first new video card.
    // The rest simply copy the extracted data.
    if(curSLI_AFRIndex == m_firstComeGPUIndex)
    {
        doCopyRTToAtlas = false;
        doCopyAtlasToAtlas = false;
        doClearRT = false;

        this->idle();

        if ( m_shadowMapRT->push() )
        {
            if ( !m_divisionRenderer.completed() || !smuq.empty() ) 
            {
                ShadowMapUpdateData smc;

                if ( m_divisionRenderer.completed() )
                {
                    smc = smuq.front();
                    smuq.pop_front();

                    // Make clean (only bufer Z and stencil).
                    uint32 clearFlags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
                    if (Moo::rc().stencilAvailable())
                    {
                        clearFlags |= D3DCLEAR_STENCIL;
                    }
                    Moo::rc().device()->Clear( 0, NULL, clearFlags, 0xFFFFFFFF, 1.f, 0 );
                }
                else
                {
                    smc = *m_divisionRenderer.data();
                }

                // If the square of the shadow map is not associated with a
                // division it will remain white.
                if( m_shadowMapAtlasArrangement.isHasDivisionData( smc.coord ) && 
                    !( smc.updateFlags & UI_CLEAR ) )
                {
                    size_t divisionDataIndex = m_shadowMapAtlasArrangement.getDivisionData( smc.coord );
                    const DivisionData::Data& dd = m_divisionData.getData( divisionDataIndex );
                    WorldCoord wc  = m_divisionData.getWorldCoord( divisionDataIndex );
                    size_t quality = m_coverageArea.quality( wc );
                    
                    if ( ( smc.updateFlags & UI_QUALITY ) > 0 && smc.coord.quality < quality )
                    {
                        // From a larger to a smaller quality. Just copy the.
                        ShadowMapCoord newCoord = m_shadowMapAtlasArrangement.findFree( quality );

                        smCoord2 = newCoord;
                        doCopyAtlasToAtlas = true;

                        m_divisionData.setShadowMapCoord( divisionDataIndex, newCoord );
                        m_shadowMapAtlasArrangement.unsetDivisionData( smc.coord );
                        m_shadowMapAtlasArrangement.setDivisionData( newCoord, divisionDataIndex );
                        m_needsUpdateIndirectionTextures = true;

                        if ( ( smc.updateFlags & ( UI_OBJECTS | UI_TERRAIN ) ) > 0 )
                        {
                            m_shadowMapUpdateQueue.insert( newCoord, m_coverageArea.priority( wc ) );
                        }

                        // Cleaning the texture more permissions.
                        Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET, 0xFFFFFFFF, 1.f, 0 );
                        doCopyRTToAtlas = true;
                        smCoord1 = smc.coord;
                    }
                    else
                    {
                        if ( smc.coord.quality > quality )
                            smc.updateFlags |= UI_OBJECTS | UI_TERRAIN;
        
                        if ( m_divisionRenderer.completed() )
                        {
                            // 1. Copy of the atlas rendertarget.
                            copyAtlasToRT( smc.coord );

                            // 2. To clean the concrete channel.
                            Vector4 clearColor = Vector4( 1.f, 1.f, 1.f, 1.f );
                            uint32  mask = 0;

                            if (smc.updateFlags & UI_OBJECTS)
                                mask |= D3DCOLORWRITEENABLE_GREEN;

                            if (smc.updateFlags & UI_TERRAIN)
                                mask |= D3DCOLORWRITEENABLE_RED;

                            Moo::rc().fsQuad().clearChannel( clearColor, mask, 
                                m_shadowMapRT->width(), 
                                m_shadowMapRT->height() );
                            m_divisionRenderer.init( smc, 
                                ((smc.updateFlags & UI_OBJECTS)== 0) ? 1 : g_updateDivisionMaxFrames );
                        }

                        // 3. Make a white edge (1px) around each slot
                        // in the atlas (So that sampling does not cause 
                        // bleeding between different atlas slots).
                        Moo::ViewportSetter vs( 
                            0 /*SM_BORDER_SIZE*/, 
                            SM_BORDER_SIZE, 
                            m_shadowMapRT->width()  - ( SM_BORDER_SIZE /** 2*/ ), 
                            m_shadowMapRT->height() - ( SM_BORDER_SIZE /** 2*/ ) 
                        );

                        // 4. retrieve matrices
                        Moo::rc().view( m_sunViewMatrix );
                        Moo::rc().projection( dd.sunCropMatrix );
                        Moo::rc().updateViewTransforms();

                        // 5. update shared constants.
                        Moo::rc().effectVisualContext().updateSharedConstants( Moo::CONSTANTS_PER_VIEW );
                        Moo::rc().effectVisualContext().updateSharedConstants( Moo::CONSTANTS_PER_SCREEN );

                        // while ( !m_divisionRenderer.completed() )
                        m_divisionRenderer.render( wc, shadowDrawContext, 
                            m_worldDivisionArrangement, 
                            m_pShadowView, m_terrainCastEM );

                        if ( m_divisionRenderer.completed() )
                        {
                            if ( smc.coord.quality > quality )
                            {
                                ShadowMapCoord newCoord = m_shadowMapAtlasArrangement.findFree( quality );

                                m_divisionData.setShadowMapCoord( divisionDataIndex, newCoord );
                                m_shadowMapAtlasArrangement.unsetDivisionData( smc.coord );
                                m_shadowMapAtlasArrangement.setDivisionData( newCoord, divisionDataIndex );
                                m_needsUpdateIndirectionTextures = true;

                                smCoord1 = newCoord;
                                
                                // Cleaning the lower resolution textures.
                                doClearRT = true;
                                smCoord2 = smc.coord;
                            }
                            else
                            {
                                smCoord1 = smc.coord;
                            }
                            doCopyRTToAtlas = true;
                        }
                    }
                }
                else
                {
                    Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET, 0xFFFFFFFF, 1.f, 0 );
                    m_divisionRenderer.clear();

                    if ( m_shadowMapAtlasArrangement.isHasDivisionData( smc.coord ) 
                        && ( smc.updateFlags & UI_CLEAR ))
                    {
                        size_t divisionDataIndex = 
                            m_shadowMapAtlasArrangement.getDivisionData( smc.coord );
                        m_shadowMapUpdateQueue.insert( 
                            smc.coord, 
                            m_coverageArea.priority( 
                                m_divisionData.getWorldCoord( divisionDataIndex ) ) 
                        );
                    }

                    doCopyRTToAtlas = true;
                    smCoord1 = smc.coord;
                }
            }

            m_shadowMapRT->pop();
        }
    }

    // Cleaning the lower resolution textures.
    if (doCopyAtlasToAtlas)
        copyAtlasToAtlas( smCoord2, smCoord1 );

    if (doCopyRTToAtlas)
        copyRTToAtlas( smCoord1 );

    if (doClearRT)
    {
        RECT destRect = rectFromCoord( smCoord2 );
        RECT srcRect;

        srcRect.left    = 0; //SM_BORDER_SIZE;
        srcRect.right   = m_emptyRT->width() - ( SM_BORDER_SIZE /** 2*/ );
        srcRect.top     = SM_BORDER_SIZE;
        srcRect.bottom  = m_emptyRT->height() - ( SM_BORDER_SIZE /** 2*/ );

        Moo::copyRT( m_shadowMapAtlas[smCoord2.quality].get(), &destRect, 
            m_emptyRT.get(), &srcRect, D3DTEXF_POINT );
    }

    Moo::rc().dynamicShadowsScene(false);
}

//-----------------------------------------------------------------------------
RECT SemiDynamicShadow::Impl::rectFromCoord( const ShadowMapCoord & coord )
{
    RECT atlas = m_shadowMapAtlasArrangement.atlas( coord.quality );
    RECT rect;

    size_t slotWidth  = m_shadowMapAtlasArrangement.slotWidth( coord.quality );
    size_t slotHeight = m_shadowMapAtlasArrangement.slotHeight( coord.quality );
    size_t texWidth     = ( atlas.right - atlas.left ) / slotWidth;
    size_t texHeight    = ( atlas.bottom - atlas.top ) / slotHeight;

    rect.left   = LONG(coord.x   * texWidth);
    rect.right  = LONG(rect.left + texWidth);
    rect.top    = LONG(coord.y   * texHeight);
    rect.bottom = LONG(rect.top  + texHeight);

    rect.left   += 0; //SM_BORDER_SIZE;
    rect.top    += SM_BORDER_SIZE;
    rect.bottom -= ( SM_BORDER_SIZE /** 2*/ );
    rect.right  -= ( SM_BORDER_SIZE /** 2*/ );

    return rect;
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::copyRTToAtlas( const ShadowMapCoord & smc )
{
    // Copying the final RT in satin, only after rendering
    // division is completed.
    RECT destRect = rectFromCoord( smc );
    RECT srcRect;
    
    srcRect.left    = 0; //SM_BORDER_SIZE;
    srcRect.right   = m_shadowMapRT->width() - ( SM_BORDER_SIZE /** 2*/ );
    srcRect.top     = SM_BORDER_SIZE;
    srcRect.bottom  = m_shadowMapRT->height() - ( SM_BORDER_SIZE /** 2*/ );

    Moo::copyRT( m_shadowMapAtlas[smc.quality].get(), &destRect, 
        m_shadowMapRT.get(), &srcRect, D3DTEXF_POINT );
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::copyAtlasToRT( const ShadowMapCoord & coord )
{
    RECT srcRect = rectFromCoord( coord );
    RECT dstRect;

    dstRect.left    = 0; // SM_BORDER_SIZE;
    dstRect.right   = m_shadowMapRT->width() - ( SM_BORDER_SIZE /** 2*/ );
    dstRect.top     = SM_BORDER_SIZE;
    dstRect.bottom  = m_shadowMapRT->height() - ( SM_BORDER_SIZE /** 2*/ );

    Moo::copyRT( m_shadowMapRT.get(), &dstRect, 
        m_shadowMapAtlas[coord.quality].get(), &srcRect, D3DTEXF_POINT );
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::copyAtlasToAtlas( 
    const ShadowMapCoord & dstCoord, const ShadowMapCoord & srcCoord )
{
    RECT srcRect = rectFromCoord( srcCoord );
    RECT dstRect = rectFromCoord( dstCoord );

    Moo::copyRT( m_shadowMapAtlas[dstCoord.quality].get(), &dstRect, 
        m_shadowMapAtlas[srcCoord.quality].get(), &srcRect, D3DTEXF_POINT );
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::onAreaLoaded( const AABB & bb )
{
    m_divisionUpdateQueue.insert( bb, CUQ_BIND );
    m_recentLoadEvent = true;
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::onAreaUnloaded( const AABB & bb )
{
    // Remove area completely from the shadows.
    unbindArea( bb );
    m_recentLoadEvent = true;
}

//-----------------------------------------------------------------------------
void SemiDynamicShadow::Impl::onAreaChanged( const AABB & bb )
{
    m_divisionUpdateQueue.insert( bb, CUQ_UPDATE );
}

#ifdef EDITOR_ENABLED
void SemiDynamicShadow::Impl::setGenerationFinishedCallback(
    GenerationFinishedCallback callback )
{
    m_generationFinishedCallback = callback;
}

void SemiDynamicShadow::Impl::callGenerationFinishedCallback()
{
    if (m_shadowMapUpdateQueue.empty() && m_generationFinishedCallback != NULL)
    {
        m_generationFinishedCallback();
        m_generationFinishedCallback = NULL;
    }
}
#endif

namespace Moo {
    //-----------------------------------------------------------------------------
    SemiDynamicShadow::SemiDynamicShadow(
        uint coveredAreaSize,
        uint shadowMapSize,
        bool debugMode)
        : m_impl(new Impl(coveredAreaSize, shadowMapSize, debugMode))
    {
        BW_GUARD;

        if (g_isFirstTime)
        {
            g_isFirstTime = false;

            MF_WATCH("Render/Shadows/EnableSemiUpdate",
                g_enableSemiUpdate, Watcher::WT_READ_WRITE,
                "Update divisions per frame mode.");
            MF_WATCH("Render/Shadows/UpdateTerrainOnly",
                g_updateTerrainOnly, Watcher::WT_READ_WRITE,
                "Update terrain only.");
            MF_WATCH("Render/Shadows/UpdateDivisionTimeDelta",
                g_divisionUpdateTime, Watcher::WT_READ_WRITE,
                "Update division time delta.");
            MF_WATCH("Render/Shadows/UpdateCoveragePerimeter",
                g_updatePerimeter, Watcher::WT_READ_WRITE,
                "Update coverage perimeter.");
            MF_WATCH("Render/Shadows/UpdateMinItemsPerFrame",
                g_updateMinItemsPerFrame, Watcher::WT_READ_WRITE,
                "Update (draw) mimimal items per frame.");
            MF_WATCH("Render/Shadows/UpdateDivisionMaxFrames",
                g_updateDivisionMaxFrames, Watcher::WT_READ_WRITE,
                "Max frames count for update division.");
        }
    }

    //-----------------------------------------------------------------------------
    SemiDynamicShadow::~SemiDynamicShadow()
    {
        BW_GUARD;
    }

    //-----------------------------------------------------------------------------
    void SemiDynamicShadow::tick(float dTime)
    {
        BW_GUARD;
        BW_SCOPED_DOG_WATCHER("SemiDynamic");

        if (!m_impl->updateConfig())
        {
            // Invalid configuration, no semi dynamic shadows required.
            return;
        }

        //-- do work
        m_impl->updateSunViewMatrix();
        m_impl->updateCoverageArea();
        m_impl->updateOutgoingDivisions();
        m_impl->updateIngoingDivisions();
        m_impl->updateQualityDivisions();
        m_impl->updateDivisionQueue(dTime);
#ifdef EDITOR_ENABLED
        m_impl->callGenerationFinishedCallback();
#endif
    }

    //-----------------------------------------------------------------------------
    void SemiDynamicShadow::cast(Moo::DrawContext& shadowDrawContext)
    {
        BW_GUARD;
        BW_SCOPED_DOG_WATCHER("SemiDynamic");
        BW_SCOPED_RENDER_PERF_MARKER("Cast Semi-Dynamic Shadows");

        //----------------------------------------------------------------------------------------------
        // Shadow Map
        //----------------------------------------------------------------------------------------------
        m_impl->renderDivision(shadowDrawContext);

        //----------------------------------------------------------------------------------------------
        // Indirection
        //----------------------------------------------------------------------------------------------
        if (m_impl->m_needsUpdateIndirectionTextures && m_impl->m_coverageArea.isCenterValid())
        {
            const CoveredArea& ca = m_impl->m_coverageArea;

            uint32 nextIndirectionTexture = (m_impl->m_currentIndirectionTextureSet + 1)
                % m_impl->NUM_INDIRECTION_STAGING_TEXTURES;

            Impl::IndirectionMap& projMap = *(m_impl->m_indirectionMapProjStagingPtr[nextIndirectionTexture]);
            Impl::IndirectionMapArgb& tcMap = *(m_impl->m_indirectionMapTcStagingPtr[nextIndirectionTexture]);

            projMap.lock(0, D3DLOCK_DISCARD);
            tcMap.lock(0, D3DLOCK_DISCARD);

            tcMap.image().fill(D3DCOLOR_ARGB(0, 0, 1, 0));
            projMap.image().fill(Vector4(0, 0, 1, 1));

            for (int x = ca.min().x; x <= ca.max().x; ++x)
            {
                for (int y = ca.min().y; y <= ca.max().y; ++y)
                {
                    WorldCoord wc(x, y);

                    // texture coords
                    // (x, y) World Space <=> (y, x) Texture Space (!!!)
                    int texX = (wc.x - ca.center().x) + (projMap.width() / 2) - 1;
                    int texY = (wc.y - ca.center().y) + (projMap.height() / 2) - 1;

                    MF_ASSERT(texX >= 0);
                    MF_ASSERT(texY >= 0);

                    if (texX >= (int)projMap.width() ||
                        texY >= (int)projMap.height())
                    {
                        continue;
                    }

                    // clear

                    if (!m_impl->m_worldDivisionArrangement.isHasDivisionData(wc))
                    {
                        projMap.image().set(texX, texY, Vector4(0, 0, 1, 1));
                        tcMap.image().set(texX, texY, D3DCOLOR_ARGB(0, 0, 1, 0));
                        continue;
                    }

                    // fill

                    size_t divisionDataIndex =
                        m_impl->m_worldDivisionArrangement.getDivisionData(wc);

                    // projMap

                    {
                        // This texture stores the parameters on which you can
                        // recover in the shader
                        // Crop-matrix of the division: the center of the projection
                        // (x, y) and its dimensions (z, w).

                        // x, y -- projection center
                        // w, h -- projection 1.f / width and 1.f / height

                        BoundingBox bb(Vector3(-1.f, -1.f, 0.f), Vector3(1.f, 1.f, 1.f));

                        Matrix sunCropInvMatrix = m_impl->m_divisionData.getData(divisionDataIndex).sunCropMatrix;
                        sunCropInvMatrix.invert(); //-- TODO(a_cherkes): we can realize it without invert

                        bb.transformBy(sunCropInvMatrix);

                        float x = (float)bb.centre().x;
                        float y = (float)bb.centre().y;
                        float w = (float)bb.width();
                        float h = (float)bb.height();

                        projMap.image().set(texX, texY,
                            Vector4(x, y, 1.f / w, 1.f / h));
                    }

                    // tcMap

                    {
                        // This texture stores the parameters on which you can
                        // recover in the shader
                        // Texture coordinates of a square slot of the shadow map.

                        // A, R -- left up corner
                        // G    -- quality

                        ShadowMapCoord smc =
                            m_impl->m_divisionData.getShadowMapCoord(divisionDataIndex);
                        tcMap.image().set(texX, texY,
                            D3DCOLOR_ARGB(smc.x, smc.y, smc.quality, 0));
                    }
                }
            }

            projMap.unlock();
            tcMap.unlock();

            Moo::rc().device()->UpdateTexture(projMap.pTexture(), m_impl->m_indirectionMapProjPtr->pTexture());
            Moo::rc().device()->UpdateTexture(tcMap.pTexture(), m_impl->m_indirectionMapTcPtr->pTexture());

            m_impl->m_currentIndirectionTextureSet = nextIndirectionTexture;
            m_impl->m_needsUpdateIndirectionTextures = false;
        }

        //----------------------------------------------------------------------------------------------
    }

    //-----------------------------------------------------------------------------
    void SemiDynamicShadow::receive()
    {
        BW_GUARD;
        BW_SCOPED_DOG_WATCHER("receive semi-dynamic shadows");
        BW_SCOPED_RENDER_PERF_MARKER("Receive Semi-Dynamic Shadows");

        CoveredArea& ca = m_impl->m_coverageArea;

        float minX = ca.center().x * m_impl->m_divisionSize;
        float minY = ca.center().y * m_impl->m_divisionSize;
        float w = m_impl->m_indirectionMapProjPtr->width() * m_impl->m_divisionSize;
        float h = m_impl->m_indirectionMapProjPtr->height() * m_impl->m_divisionSize;

        Vector4 transform(minX, minY, 2.f / w, 2.f / h);

        float invX = 1.f / ((float)m_impl->m_shadowMapAtlas[QL_LOW]->width());
        float invY = 1.f / ((float)m_impl->m_shadowMapAtlas[QL_LOW]->height());
        Vector4 shadowMapSizeInv(invX, invY, invX * 2.f, invY * 2.f);
        Vector4 cropDistances(g_cropZn, g_cropZf, 0.f, 0.f);

        float quality[QL_TOTAL];
        for (int i = 0; i < QL_TOTAL; ++i)
        {
            quality[i] = 1.f / m_impl->m_shadowMapAtlasArrangement.slotWidth(i);
        }

        ID3DXEffect* effect = m_impl->m_receiveEM->pEffect()->pEffect();

        effect->SetVector("g_worldToIndirectionTcTransform", &transform);
        effect->SetTexture("g_indirectionMapProj", m_impl->m_indirectionMapProjPtr->pTexture());
        effect->SetTexture("g_indirectionMapTc", m_impl->m_indirectionMapTcPtr->pTexture());

        effect->SetTexture("g_shadowMapHeight", m_impl->m_shadowMapAtlas[QL_HIGH]->pTexture());
        effect->SetTexture("g_shadowMapLow", m_impl->m_shadowMapAtlas[QL_LOW]->pTexture());

        effect->SetVector("g_shadowMapSizeInv", &shadowMapSizeInv);
        effect->SetVector("g_cropDistances", &cropDistances);

        effect->SetMatrix("g_sunViewMatrix", &m_impl->m_sunViewMatrix);
        effect->SetFloatArray("g_quality", quality, QL_TOTAL + 1);
        effect->SetFloat("g_coverAreaSizeInv", 1.f / m_impl->m_coveredAreaSize);

        effect->CommitChanges();

        Moo::rc().fsQuad().draw(*(m_impl->m_receiveEM));
    }

    //-----------------------------------------------------------------------------
    void SemiDynamicShadow::updateArea(const AABB& area)
    {
        m_impl->onAreaChanged(area);
    }

    //-----------------------------------------------------------------------------
    void SemiDynamicShadow::drawDebugInfo()
    {
        BW_GUARD;
        if (!m_impl->m_debugMode)
            return;

        Moo::drawDebugTexturedRect(Vector2(0.f, 0.f), Vector2(150.f, 150.f),
            m_impl->m_shadowMapAtlas[QL_HIGH]->pTexture());
        Moo::drawDebugTexturedRect(Vector2(150.f, 0.f), Vector2(300.f, 150.f),
            m_impl->m_shadowMapAtlas[QL_LOW]->pTexture());

        Moo::drawDebugTexturedRect(Vector2(300.f, 0.f), Vector2(500.f, 200.f),
            m_impl->m_shadowMapRT->pTexture());
        Moo::drawDebugTexturedRect(Vector2(300.f, 200.f), Vector2(400.f, 300.f),
            m_impl->m_indirectionMapTcPtr->pTexture());
        Moo::drawDebugTexturedRect(Vector2(400.f, 200.f), Vector2(500.f, 300.f),
            m_impl->m_indirectionMapProjPtr->pTexture());
    }

#ifdef EDITOR_ENABLED
    void SemiDynamicShadow::setGenerationFinishedCallback(
        GenerationFinishedCallback callback)
    {
        m_impl->setGenerationFinishedCallback(callback);
    }
#endif

}

BW_END_NAMESPACE

//-----------------------------------------------------------------------------
