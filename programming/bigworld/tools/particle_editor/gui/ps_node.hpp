#ifndef PS_NODE_HPP
#define PS_NODE_HPP

#include "resmgr/datasection.hpp"
#include "particle/meta_particle_system.hpp"
#include "particle/particle_system.hpp"
#include "particle/actions/particle_system_action.hpp"
#include "controls/tree_control.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

//
// This represents a metaparticle system node in a TreeControl.
//
class MetaNode : public TreeNode
{
public:
    //
    // Explicit constructor.
    //
    // @param dir               The directory to load from.
    // @param filename          The name of the file containing the 
    //                          meta-particle system.
    //
    MetaNode(BW::string const &dir, BW::string const &filename);

    //
    // Destructor.
    //
    /*virtual*/ ~MetaNode();

    //
    // Set the label.
    //
    // @param label     The new label.  The underlying file is renamed if
    //                  appropriate.
    //
    /*virtual*/ void SetLabel(BW::string const &label);

    //
    // MetaNode's labels are editable if the underlying system is a meta
    // particle system.
    //
    // @returns                 false.
    //
    /*virtual*/ bool CanEditLabel() const;

    //
    // MetaNode's can only be copied.
    //
    // @returns                 DROPEFFECT_COPY or DROPEFFECT_NONE.
    //
    /*virtual*/ DROPEFFECT CanDragDrop() const;

    //
    // Get the meta-particle system. 
    //
    // @returns                 The meta particle system.
    //
    MetaParticleSystemPtr GetMetaParticleSystem() const;

    //
    // Set the meta-particle system.
    //
    // @param system            The meta-particle system to use.
    //
    void SetMetaParticleSystem(MetaParticleSystemPtr system);

	//
    // Set the read only state.
    //
    // @param readOnly            The read only state.
    //
    void SetReadOnly(bool readOnly);

	//
	// Is the meta-particle system read only
	//
	// @returns					Whether the meta-particle system is read only.
	//
	bool IsReadOnly() const;

    //
    // Make sure that the meta-particle system is in memory.
    //
    void EnsureLoaded() const;

    //
    // Get the filename.
    //
    // @returns                 The underlying filename.
    //
    BW::string GetFilename() const;

    //
    // Get the filename if the node were to be renamed.
    //
    // @param name              The new name.
    // @returns                 The file it would be saved to.
    //
    static BW::string GetFilename(BW::string const &file);

    //
    // Get the directory.
    //
    // @returns                 The directory.
    //
    BW::string const &GetDirectory() const;

    //
    // Get the fullpath of the underlying file.
    //
    // @returns                 The fullpath include the directory of the
    //                          underlying meta-particle system.
    //
    BW::string GetFullpath() const;

    //
    // Delete the underlying file.  The node should not be used beyond this
    // point.
    //
    // @returns                 True if the file was deleted.
    // 
    bool DeleteFile();

    //
    // Flag that children have been added manually and so do not load
    // upon expansion.
    //
    void FlagChildrenReady();

    //
    // Called when the meta-particle system is saved.  The saving of the 
    // meta-particle system is done elsewhere.  This allows for undoing
    // a rename to work effectively.
    //
    void onSave();

    //
    // Called when the meta-particle system is not saved and loses focus.  This
    // allows us to rename it back to its original name if that is what was
    // done to it.
    //
    void onNotSave();

    //
    // Called just before the meta-particle system is renamed.
    //
    void onRename();

    //
    // Serialize expansion/contraction data etc to the given data section.
    //
    // @param data      The DataSection to load/save the data.
    // @param load      True if loading.
    // @returns         The selected child (can be NULL).
    //
    /*virtual*/ TreeNode *
    Serialise
    (
        DataSectionPtr  data, 
        bool            load
    );

    //
    // Is the given file likely to be a meta-particle system?
    //
    // @param filename          The filename to test.
    // @returns                 True if the filename corresponds with a 
    //                          particle system filename.
    //
    static bool IsMetaParticleFile(BW::StringRef const &filename);

    //
    // Called when the node is expanded.  If the meta-particle system is not
    // loaded then we do so and then we add nodes corresponding to the
    // particle systems and their actions.
    //
    /*virtual*/ void OnExpand();

protected:
    //
    // Nodes are added upon expansion.
    //
    // @returns                 true.
    //
    /*virtual*/ bool IsVirtualNode() const;

    //
    // Deletion does require confirmation.
    //
    // @returns                 true.
    //
    /*virtual*/ bool DeleteNeedsConfirm() const;

    //
    // Sync the label with the model.
    //
    /*virtual*/ void OnEditLabel(BW::string const &newLabel);

    //
    // Get the full path of the given file, taking into account the 
    // directory.
    //
    // @param filename          The name of the file to get.
    // @returns                 The full location of filename.
    //
    BW::string fullpath(BW::string const &filename) const;

private:
    mutable MetaParticleSystemPtr   m_metaParticleSystem;
    BW::string                     m_directory;
    bool                            m_createdChildren;
    BW::string                     m_lastName;
	mutable bool					m_readOnly;
};

//
// This class represents a particle system node in a TreeControl.
//
class PSNode : public TreeNode
{
public:
    //
    // Default constructor.
    //
    // @param ps                The particle system.
    //
    explicit PSNode(ParticleSystemPtr ps);

    //
    // Set the new label, and update the name of the underlying particle system.
    //
    // @param label             The new name of the particle system.
    //
    /*virtual*/ void SetLabel(BW::string const &label);

    //
    // Return the allowed drag-and-drop operations.
    //
    // @returns                 DROPEFFECT_COPY.
    //
    /*virtual*/ DROPEFFECT CanDragDrop() const;

    //
    // Add the children nodes for the system, renderer properties and for the
    // actions.
    //
    void addChildren();

    //
    // Get the underlying particle system.
    //
    ParticleSystemPtr getParticleSystem() const;

protected:
    //
    // Deletion does require confirmation.
    //
    // @returns                 true.
    //
    /*virtual*/ bool DeleteNeedsConfirm() const;

private:
    ParticleSystemPtr           m_particleSystem;
};

//
// This class represents an Action on a particle system.
//
class ActionNode : public TreeNode
{
public:
    enum ActionType
    {
        AT_ACTION,              // This represents an actual action.
        AT_SYS_PROP,            // This represents system properties.
        AT_REND_PROP,           // This represents render properties.
		AT_META_PS,				// This represents meta particle system.
		AT_PS					// This represents particle system.
    };

    //
    // Default constructor.
    //
    // @param action            The particle system action.
    // @param name              The action's name.
    // @param type              The type of the node.
    //
    ActionNode
    (
        ParticleSystemActionPtr action, 
        BW::string             const &name,
        ActionType              actionType  = AT_ACTION
    );

    //
    // Return the allowed drag-and-drop operations.
    //
    // @returns                 DROPEFFECT_COPY.
    //
    /*virtual*/ DROPEFFECT CanDragDrop() const;

    //
    // Get the underlying action.
    //
    ParticleSystemActionPtr getAction();

    //
    // The action type.
    //
    ActionType getActionType();

    //
    // ActionNode's labels are not editable.
    //
    // @returns                 false.
    //
    /*virtual*/ bool CanEditLabel() const;

private:
    ParticleSystemActionPtr     m_action;
    int                         m_id;
    ActionType                  m_actionType;
};

BW_END_NAMESPACE

#endif // PS_NODE_HPP
