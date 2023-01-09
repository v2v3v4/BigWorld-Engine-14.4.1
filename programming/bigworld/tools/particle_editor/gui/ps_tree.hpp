#ifndef PS_TREE_HPP
#define PS_TREE_HPP

#include "particle/meta_particle_system.hpp"
#include "gui/controls/tree_control.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class   MetaNode;
class   MetaParticleSystem;
typedef SmartPointer<MetaParticleSystem> MetaParticleSystemPtr;

//
// This class is a TreeControl that displays meta-particle systems.
//
class PSTree : public TreeControl
{
public:
    //
    // Constructor.
    //
    PSTree();    

    //
    // Load at the given directory root.
    //
    // @param dir           The directory to load from.
    //
    bool Load(BW::string const &dir);

    //
    // Get the selected MetaParticleSystem.
    //
    // @returns             The selected meta-particle system.  This traverses
    //                      up the tree if necessary.
    //
    MetaParticleSystemPtr GetSelectedMetaParticleSystem() const;

    //
    // Get the selected ParticleSystem.
    //
    // @returns             The selected particle system.  This traverses up the
    //                      tree if necessary.
    //
    ParticleSystemPtr GetSelectedParticleSystem() const;

    //
    // Get the selected action.
    //
    // @returns             Teh selected ParticleSystemAction
    //
    ParticleSystemActionPtr GetSelectedAction() const;

    //
    // Select a meta particle system.
    // 
    // @param name          The name of the system to select.
    // @returns             True if the system was selected, false otherwise.
    //
    bool SelectMetaParticleSystem(BW::string const &name);

	//
	// Sets all the checkboxes for the meta particle system
	//
	// @param metaNode		The MetaNode of the meta particle system to update the checkboxes.
	//
	void updateCheckBoxes( MetaNode *metaNode );

	// Sets the enabled state for the meta particle system from the checkboxes
	//
	// @param metaNode		The MetaNode of the meta particle system to update the enabled state.
	//
	void updateEnabledState( MetaNode *metaNode );

    //
    // Set the display filter.
    //
    void SetFilter(BW::string const &filter);

    //
    // Get the particle system directory.
    //
    // @returns             The directory containing the meta-particle systems.
    //
    BW::string const &GetDirectory() const;

protected:
    //
    // Called upon item expansion.  This helper deals with virtual nodes.
    //
    // @param nmhdr         The notification data.
    // @param result        The return LRESULT.
    // @returns             FALSE (allow processing by parent).
    //
    afx_msg BOOL OnSelection(NMHDR* nmhdr, LRESULT* result);

    //
    // Called when a keypress is done.
    //
    // @param key           The key pressed down.
    // @param repcnt        The repeat count.
    // @param flags         Shift status etc.
    //
    afx_msg void OnKeyDown(UINT key, UINT repcnt, UINT flags);

    //
    // Called when the right mouse button is pressed.
    //
    // @param flags         The shift key status etc.
    // @param point         The cursor location.
    //
    afx_msg void OnRButtonDown(UINT flags, CPoint point);

    //
    // Called to do painting.
    //
    afx_msg void OnPaint();

    DECLARE_MESSAGE_MAP()

private:
    BW::string             m_dir;
};

BW_END_NAMESPACE

#endif // PS_TREE_HPP
