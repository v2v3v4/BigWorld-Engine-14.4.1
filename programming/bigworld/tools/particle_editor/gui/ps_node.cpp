#include "pch.hpp"
#include "ps_node.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE

namespace
{
    const BW::StringRef PARTICLE_SYS_EXT = "xml";
}


MetaNode::MetaNode(BW::string const &dir, BW::string const &filename) :
    TreeNode(),
    m_metaParticleSystem(NULL),
    m_directory(dir),
    m_createdChildren(false),
	m_readOnly(false)
{
	BW_GUARD;

    BW::string label = BWResource::getFilename( BWResource::removeExtension( filename ) ).to_string();
    SetLabel(label.c_str());
    m_lastName = label;
}


/*virtual*/ MetaNode::~MetaNode()
{
	BW_GUARD;

    if (m_metaParticleSystem != NULL)
    {
        m_metaParticleSystem->detach();
        m_metaParticleSystem = NULL;
    }
}


/*virtual*/ void MetaNode::SetLabel(BW::string const &label)
{
	BW_GUARD;

    // Rename the underlying file:
    BW::string oldFilename = GetFilename();
    if (!oldFilename.empty())
    {
        BW::string oldFullPath = fullpath(GetFilename());
        BW::string newFullPath = fullpath(label + '.' + PARTICLE_SYS_EXT);
        if (oldFullPath != newFullPath)
            rename(oldFullPath.c_str(), newFullPath.c_str());
    }

    // Change the actual label:
    TreeNode::SetLabel(label);
}


/*virtual*/ bool MetaNode::CanEditLabel() const
{
	BW_GUARD;

    EnsureLoaded();
    return m_metaParticleSystem != NULL;
}


/*virtual*/ DROPEFFECT MetaNode::CanDragDrop() const
{
	BW_GUARD;

    EnsureLoaded();
    if (m_metaParticleSystem != NULL)
      return DROPEFFECT_COPY;
    else
        return DROPEFFECT_NONE;
}


MetaParticleSystemPtr MetaNode::GetMetaParticleSystem() const
{
    return m_metaParticleSystem;
}


void MetaNode::SetMetaParticleSystem(MetaParticleSystemPtr system)
{
    m_metaParticleSystem = system;
}

void MetaNode::SetReadOnly( bool readOnly )
{
	m_readOnly = readOnly;
}

bool MetaNode::IsReadOnly() const
{
	return m_readOnly;
}

void MetaNode::EnsureLoaded() const
{
	BW_GUARD;

    if (m_metaParticleSystem == NULL)
    {
        m_metaParticleSystem = new MetaParticleSystem();
        bool ok = m_metaParticleSystem->load(GetFilename(), m_directory);
        if (!ok)
        {
            m_metaParticleSystem = NULL;
        }
		else
		{
			BW::wstring fullPath = BWResource::resolveFilenameW( m_directory + GetFilename() );
			DWORD att = GetFileAttributes( fullPath.c_str() );
			if ( att & FILE_ATTRIBUTE_READONLY )
			{
				static bool s_messageBoxShowed = false;
				if (!s_messageBoxShowed)
				{
					s_messageBoxShowed = true;
					::MessageBox( AfxGetMainWnd()->GetSafeHwnd(), 
						Localise(L"`RCS_IDS_READONLY", fullPath ),
						Localise(L"`RCS_IDS_READONLYTITLE"),
						MB_ICONEXCLAMATION | MB_OK );
				}

				ERROR_MSG( "%s\n", LocaliseUTF8( L"`RCS_IDS_READONLY", fullPath ).c_str() );
				m_readOnly = true;
			}
		}
    }
}


BW::string MetaNode::GetFilename() const
{
	BW_GUARD;

    return GetFilename(GetLabel());
}


/*static*/ BW::string MetaNode::GetFilename(BW::string const &file)
{
	BW_GUARD;

    return file + '.' + PARTICLE_SYS_EXT;
}


BW::string const &MetaNode::GetDirectory() const
{
    return m_directory;
}


BW::string MetaNode::GetFullpath() const
{
	BW_GUARD;

    return fullpath(GetFilename());
}


bool MetaNode::DeleteFile()
{
	BW_GUARD;

    BW::string file = GetFullpath();
    return unlink(file.c_str()) == 0;
}


void MetaNode::FlagChildrenReady()
{
    m_createdChildren = true;
}


void MetaNode::onSave()
{
	BW_GUARD;

    m_lastName = GetLabel();
}


void MetaNode::onNotSave()
{
	BW_GUARD;

    if (m_lastName != GetLabel())
    {
        BW::string name = m_lastName;        
        SetLabel(name);
        m_lastName = name;
    }
}


void MetaNode::onRename()
{
    //m_lastName = GetLabel();
}


/*virtual*/ TreeNode *
MetaNode::Serialise
(
    DataSectionPtr  data, 
    bool            load
)
{
	BW_GUARD;

    TreeNode *result = TreeNode::Serialise(data, load);
    if (load)
    {
        m_lastName = data->readString("lastSaveName", GetLabel());
    }
    else
    {
        data->writeString("lastSaveName", m_lastName);
    }
    return result;
}


/*static*/ bool MetaNode::IsMetaParticleFile(BW::StringRef const &filename)
{
	BW_GUARD;

    BW::StringRef extension = BWResource::getExtension(filename);
	// TODO: compare_lower
    return extension.equals_lower( PARTICLE_SYS_EXT );
}


/*virtual*/ void MetaNode::OnExpand()
{
	BW_GUARD;

	TreeControl *tree = GetTreeControl();
	tree->SetSelectedNode(this);
    // If already expanded do nothing:
    if (m_createdChildren)
        return;

    EnsureLoaded();

    if (m_metaParticleSystem == NULL)
        return;

    

    // Add the particle systems:
    BW::vector<ParticleSystemPtr> &partileSyss = m_metaParticleSystem->systemSet();
    for (size_t i = 0; i < partileSyss.size(); ++i)
    {
        ParticleSystemPtr partSys = partileSyss[i];
        PSNode *psnode = (PSNode *)tree->AddNode(new PSNode(partSys), this);
		tree->SetCheck( *psnode, psnode->getParticleSystem()->enabled() ? BST_CHECKED : BST_UNCHECKED );
        psnode->addChildren();
    }
    m_createdChildren = true;
}


/*virtual*/ bool MetaNode::IsVirtualNode() const
{
    return true;
}


/*virtual*/ bool MetaNode::DeleteNeedsConfirm() const
{
    return true;
}


/*virtual*/ void MetaNode::OnEditLabel(BW::string const &/*newLabel*/)
{
    // The parent needs to receive the notification, verify the model and
    // make the change via RenameFile.
}


BW::string MetaNode::fullpath(BW::string const &filename) const
{
	BW_GUARD;

    BW::string fullpath = m_directory + filename;
    if (!fullpath.empty() && (fullpath[0] == '/' || fullpath[0] == '\\'))
        fullpath.erase(fullpath.begin());
    fullpath = BWResource::resolveFilename(fullpath);
    return fullpath;
}


/*explicit*/ PSNode::PSNode(ParticleSystemPtr ps) :
    TreeNode(),
    m_particleSystem(ps)
{
	BW_GUARD;

    if (ps != NULL)
        SetLabel(m_particleSystem->name().c_str());
}


/*virtual*/ void PSNode::SetLabel(BW::string const &label)
{
	BW_GUARD;

    m_particleSystem->name(label);
    TreeNode::SetLabel(label);
}


/*virtual*/ DROPEFFECT PSNode::CanDragDrop() const
{
    return DROPEFFECT_COPY;
}


void PSNode::addChildren()
{
	BW_GUARD;

    TreeControl *tree = GetTreeControl();
	TreeNode *newNode;
    // Add the system properties node:
    newNode = tree->AddNode
    (
        new ActionNode(NULL, LocaliseUTF8(L"PARTICLEEDITOR/GUI/SYSTEM_PROP"), ActionNode::AT_SYS_PROP), 
        this
    );
	tree->SetItemState( *newNode, 0, TVIS_STATEIMAGEMASK ); 
    // Add the renderer properties nodes:
    newNode = tree->AddNode
    (
        new ActionNode(NULL, LocaliseUTF8(L"PARTICLEEDITOR/GUI/RENDERER_PROP"), ActionNode::AT_REND_PROP), 
        this
    ); 
	tree->SetItemState( *newNode, 0, TVIS_STATEIMAGEMASK ); 
    // Add the action nodes:
    BW::vector<ParticleSystemActionPtr> &actions = m_particleSystem->actionSet();
    typedef BW::map<BW::string, int> NameCountMap;
    NameCountMap names;
    for (size_t j = 0; j < actions.size(); ++j)
    {
        ParticleSystemActionPtr psa = actions[j];
        // Generate a name for the action if it doesn't have one:
        BW::string name = psa->name();
        if (name.empty())
        {
            NameCountMap::iterator it = names.find(psa->nameID());
            if (it == names.end())
                names[psa->nameID()] = 0;
            static const uint32 bufferSize = 512;
            char buffer[bufferSize];
            bw_snprintf
            (
                buffer, 
                bufferSize, 
                "%s %i",
                psa->nameID().c_str(),
                ++names[psa->nameID()]
            );
            psa->name(buffer);
            name = buffer;
        }
        newNode = tree->AddNode(new ActionNode(psa, name), this);   
		ActionNode *actionNode = dynamic_cast<ActionNode*>(newNode);
		if ( actionNode != NULL )
		{
			tree->SetCheck( *actionNode, actionNode->getAction()->enabled() ? BST_CHECKED : BST_UNCHECKED );
		}
    }
}


ParticleSystemPtr PSNode::getParticleSystem() const
{
    return m_particleSystem;
}


/*virtual*/ bool PSNode::DeleteNeedsConfirm() const
{
    return true;
}


ActionNode::ActionNode
(
    ParticleSystemActionPtr action, 
    BW::string                  const &name,
    ActionType              actionType  /*= AT_ACTION*/
) :
    TreeNode(),
    m_action(action),
    m_actionType(actionType)
{
	BW_GUARD;

    SetLabel(name.c_str());
}


/*virtual*/ DROPEFFECT ActionNode::CanDragDrop() const
{
    return DROPEFFECT_COPY;
}


ParticleSystemActionPtr ActionNode::getAction()
{
    return m_action;
}


ActionNode::ActionType ActionNode::getActionType()
{
    return m_actionType;
}


 /*virtual*/ bool ActionNode::CanEditLabel() const
{
    return false;
}

BW_END_NAMESPACE

