#ifndef CHUNK_LINK_HPP
#define CHUNK_LINK_HPP

#include "chunk/chunk_item.hpp"


BW_BEGIN_NAMESPACE

/**
 *  This represents a link between two chunk items.
 */
class ChunkLink : public ChunkItem
{
public:
    /**
     *  Direction bitfield.
     */
    enum Direction
    {
        DIR_NONE        = 0,
        DIR_START_END   = 1,
        DIR_END_START   = 2,
        DIR_BOTH        = DIR_END_START | DIR_START_END
    };

    ChunkLink();
    ~ChunkLink();

    ChunkItemPtr startItem() const;
    void startItem(ChunkItemPtr item);

    ChunkItemPtr endItem() const;
    void endItem(ChunkItemPtr item);

    Direction direction() const;
    void direction(Direction dir);

#ifdef EDITOR_ENABLED
    virtual bool isValid(BW::string &failureMsg) const;

    virtual void makeDirty();
#endif

private:
    ChunkLink(ChunkLink const &);
    ChunkLink &operator=(ChunkLink const &);

private:
    ChunkItemPtr    startItem_;
    ChunkItemPtr    endItem_;
    Direction       direction_;
};


typedef SmartPointer<ChunkLink> ChunkLinkPtr;

BW_END_NAMESPACE

#endif // CHUNK_LINK_HPP
