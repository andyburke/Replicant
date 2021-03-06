#include "Foundation/Inspect/Control.h"
#include "Foundation/Inspect/Canvas.h"

using namespace Helium;
using namespace Helium::Inspect;

#ifdef PROFILE_ACCUMULATION
Profile::Accumulator Inspect::g_RealizeAccumulator( "Inspect Realize Accumulator" );
Profile::Accumulator Inspect::g_UnrealizeAccumulator( "Inspect Unrealize Accumulator" );
#endif

Control::Control()
: a_IsEnabled( true )
, a_IsReadOnly( false )
, a_IsFrozen( false )
, a_IsHidden( false )
, a_ForegroundColor( 0 )
, a_BackgroundColor( 0 )
, a_IsFixedWidth( false )
, a_IsFixedHeight( false )
, a_ProportionalWidth( 0.f )
, a_ProportionalHeight( 0.f )
, m_Canvas( NULL )
, m_Parent( NULL )
, m_IsWriting( false )
, m_IsRealized( false )
{

}

Control::~Control()
{
    if (m_BoundData)
    {
        m_BoundData->RemoveChangedListener( DataChangedSignature::Delegate ( this, &Control::DataChanged ) );
    }

    m_IsRealized = false;
}

int Control::GetDepth()
{
    int depth = 0;
    Control* parent = m_Parent;

    while (parent != NULL)
    {
        depth++;
        parent = parent->m_Parent;
    }

    return depth;
}

void Control::SetCanvas( Canvas* canvas )
{
    if ( m_Canvas != canvas )
    {
        HELIUM_ASSERT( (!m_Canvas && canvas) || (m_Canvas && !canvas) );
        m_Canvas = canvas;
    }
}

void Control::SetParent( Container* parent )
{
    if ( m_Parent != parent )
    {
        Container* oldParent = m_Parent;

        m_Parent = parent;

        if ( oldParent )
        {
            oldParent->RemoveChild( this );
        }

        if ( m_Parent )
        {
            m_Parent->AddChild( this );
        }
    }
}

void Control::Bind(const DataPtr& data)
{
    if ( !m_BoundData.ReferencesObject() || !data.ReferencesObject() )
    {
        if ( m_BoundData.ReferencesObject() )
        {
            m_BoundData->RemoveChangedListener( DataChangedSignature::Delegate ( this, &Control::DataChanged ) );
        }

        m_BoundData = data;

        if ( m_BoundData.ReferencesObject() )
        {
            m_BoundData->AddChangedListener( DataChangedSignature::Delegate ( this, &Control::DataChanged ) );
        }
    }
}

bool Control::IsDefault() const
{
    if (a_Default.Get().empty() || m_BoundData == NULL)
    {
        return false;
    }

    StringData* data = CastData<StringData, DataTypes::String>( m_BoundData );
    if ( data )
    {
        tstring val;
        data->Get(val);
        return a_Default.Get() == val;
    }

    HELIUM_BREAK(); // you need to HELIUM_OVERRIDE this, your control is using custom data
    return false;
}

bool Control::SetDefault()
{
    if (!a_Default.Get().empty())
    {
        return WriteStringData( a_Default.Get() );
    }
    else
    {
        return false;
    }
}

const ContextMenuPtr& Control::GetContextMenu()
{
    return m_ContextMenu;
}

void Control::SetContextMenu(const ContextMenuPtr& contextMenu)
{
    m_ContextMenu = contextMenu;
}

bool Control::Process(const tstring& key, const tstring& value)
{
    if ( key == ATTR_HELPTEXT )
    {
        a_HelpText.Set(value);

        return true;
    }

    return false;
}

bool Control::IsRealized()
{
    return m_IsRealized;
}

void Control::Realize(Canvas* canvas)
{
    PROFILE_SCOPE_ACCUM( g_RealizeAccumulator );

    if ( !m_IsRealized )
    {
        m_Canvas = canvas;
        m_Canvas->RealizeControl( this );

        m_IsRealized = true;
        e_Realized.Raise(this);
    }
}

void Control::Unrealize()
{
    PROFILE_SCOPE_ACCUM( g_UnrealizeAccumulator );

    if ( m_IsRealized )
    {
        m_Canvas->UnrealizeControl( this );
        m_Canvas = NULL;

        m_IsRealized = false;
        e_Unrealized.Raise(this);
    }
}

void Control::Read()
{
    if ( m_Widget )
    {
        m_Widget->Read();
    }

    SetDefaultAppearance( IsDefault() );
}

bool Control::ReadStringData(tstring& str) const
{
    StringData* data = CastData<StringData, DataTypes::String>( m_BoundData );
    if (data)
    {
        str.clear();
        data->Get( str );
        return true;
    }

    HELIUM_BREAK(); // you should not call this, your control is using custom data
    return false;
}

bool Control::ReadAllStringData(std::vector< tstring >& strs) const
{
    StringData* data = CastData<StringData, DataTypes::String>( m_BoundData );
    if ( data )
    {
        strs.clear();
        data->GetAll( strs );
        return true;
    }

    HELIUM_BREAK(); // you should not call this, your control is using custom data
    return false;
}

void Control::DataChanged(const DataChangedArgs& args)
{
    if ( !m_IsWriting )
    {
        Read();

        e_ControlChanged.Raise( this );
    }
}

bool Control::PreWrite( Reflect::Serializer* newValue, bool preview )
{
    ControlChangingArgs args (this, newValue, preview);
    e_ControlChanging.Raise( args );

    // check to see if a event handler bound to this control bypasses the write
    if ( args.m_Veto )
    {
        return false;
    }

    return true;
}

bool Control::Write()
{
    if ( m_Widget )
    {
        return m_Widget->Write();
    }

    return true;
}

bool Control::WriteStringData(const tstring& str, bool preview)
{
    StringData* data = CastData<StringData, DataTypes::String>( m_BoundData );

    return WriteTypedData(str, data, preview);
}

bool Control::WriteAllStringData(const std::vector< tstring >& strs, bool preview)
{
    StringData* data = CastData<StringData, DataTypes::String>( m_BoundData );
    if (data)
    {
        std::vector< tstring > currentValues;
        data->GetAll( currentValues );

        if ( strs == currentValues )
        {
            return true;
        }

        Reflect::SerializerPtr serializer = Reflect::AssertCast< Reflect::Serializer >( Reflect::Serializer::Create< std::vector< tstring > >() );
        serializer->ConnectData( const_cast< std::vector< tstring >* >( &strs ) );
        if ( !PreWrite( serializer, preview ) )
        {
            Read();
            return false;
        }

        m_IsWriting = true;

        bool result = data->SetAll( strs );
        m_IsWriting = false;

        if (result)
        {
            PostWrite();
            return true;
        }
    }

    HELIUM_BREAK(); // you should not call this, your control is using custom data
    return false;
}

void Control::PostWrite()
{
    SetDefaultAppearance( IsDefault() );

    // callback to our interpreter that we changed
    e_ControlChanged.Raise( this );

    // data validator could change our value, so re-read the value
    Read();
}
