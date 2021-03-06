#include "Document.h"

#include "Platform/Assert.h"
#include "Foundation/Log.h"
#include "Foundation/RCS/RCS.h"

using namespace Helium;

///////////////////////////////////////////////////////////////////////////////
// Constructor
// 
Document::Document( const tstring& path )
: m_Path( path )
, m_IsModified( false )
, m_AllowChanges( false )
, m_Revision( -1 )
{
    UpdateFileInfo();
}

void Document::UpdateFileInfo()
{
    m_Revision = -1;

    if ( !m_Path.Get().empty() )
    {
        if ( RCS::PathIsManaged( m_Path.Get() ) )
        {
            RCS::File rcsFile( m_Path.Get() );

            try
            {
                rcsFile.GetInfo();
            }
            catch ( Helium::Exception& ex )
            {
                tstringstream str;
                str << TXT( "Unable to get info for '" ) << m_Path.Get() << TXT( "': " ) << ex.What();
                Log::Warning( TXT("%s\n"), str.str().c_str() );
            }

            m_Revision = rcsFile.m_LocalRevision;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Destructor
// 
Document::~Document()
{
}

///////////////////////////////////////////////////////////////////////////////
// Sets the path to this file.  The name of the file is also updated.  Notifies
// any interested listeners about this event.
// 
void Document::SetPath( const Helium::Path& newPath )
{
    Helium::Path oldPath = m_Path.Get();

    m_Path = newPath;
    UpdateFileInfo();

    m_PathChanged.Raise( DocumentPathChangedArgs( this, oldPath ) );
}

///////////////////////////////////////////////////////////////////////////////
// Returns the revision number of the file when it was instanciated
// 
int Document::GetRevision() const
{
    return m_Revision;
}

///////////////////////////////////////////////////////////////////////////////
// Returns true if the user has specified that they want to make changes to
// this file even if it is not checked out by them.
// 
bool Document::AllowChanges() const
{
    return m_AllowChanges;
}

///////////////////////////////////////////////////////////////////////////////
// Sets whether to allow changes regardless of file check out state.
// 
void Document::SetAllowChanges( bool allowChanges )
{
    m_AllowChanges = allowChanges;
}

///////////////////////////////////////////////////////////////////////////////
// Returns true if the file has been modified.  Note, this independent of 
// whether or not the file is checked out.
// 
bool Document::IsModified() const
{
    return m_IsModified;
}

///////////////////////////////////////////////////////////////////////////////
// Sets the internal flag indicating the the file has been modified (thus it
// should probably be saved before closing).
// 
void Document::SetModified( bool modified )
{
    if ( m_IsModified != modified )
    {
        m_IsModified = modified;

        m_Modified.Raise( DocumentChangedArgs( this ) );
    }
}
