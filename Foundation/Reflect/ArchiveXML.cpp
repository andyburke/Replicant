#include "ArchiveXML.h"

#include "Element.h"
#include "Registry.h"
#include "Serializers.h"

#include "Foundation/Log.h"

#include <strstream>
#include <expat.h>

using namespace Helium::Reflect;

char Indent<char>::m_Space = ' ';
wchar_t Indent<wchar_t>::m_Space = L' ';

// uncomment to display parse stack progress
//#define REFLECT_DISPLAY_PARSE_STACK

const u32 ArchiveXML::CURRENT_VERSION                               = 3;
const u32 ArchiveXML::FIRST_VERSION_WITH_POINTER_SERIALIZER         = 2; 
const u32 ArchiveXML::FIRST_VERSION_WITH_NAMESPACE_SUPPORT          = 3; 

ArchiveXML::ArchiveXML(StatusHandler* status)
: Archive(status)
, m_Version (CURRENT_VERSION)
, m_Target (&m_Spool)
{
    m_Parser = XML_ParserCreate( Helium::GetEncoding().c_str() );

    // set the user data used in callbacks
    XML_SetUserData(m_Parser, (void*)this);

    // attach callbacks, will call back to 'this' via user data pointer
    XML_SetStartElementHandler(m_Parser, &StartElementHandler);
    XML_SetEndElementHandler(m_Parser, &EndElementHandler);
    XML_SetCharacterDataHandler(m_Parser, &CharacterDataHandler);
}

ArchiveXML::~ArchiveXML()
{
    XML_ParserFree( m_Parser );
    m_Parser = NULL;
}

void ArchiveXML::OpenFile( const tstring& file, bool write )
{
    m_Path.Set( file );

#ifdef REFLECT_ARCHIVE_VERBOSE
    Debug(TXT("Opening file '%s'\n"), file.c_str());
#endif

    Reflect::TCharStreamPtr stream = new FileStream<tchar>(file, write);
    OpenStream( stream, write );
}

void ArchiveXML::OpenStream( TCharStream* stream, bool write )
{
    // save the mode here, so that we safely refer to it later.
    m_Mode = (write) ? ArchiveModes::Write : ArchiveModes::Read; 

    // open the stream, this is "our interface" 
    stream->Open(); 

    // Set precision
    stream->SetPrecision(32);

    // Setup stream
    m_Stream = stream; 

    // Header
    if (write)
    {
        Start();
    }
}

void ArchiveXML::Close()
{
    if (m_Mode == ArchiveModes::Write)
    {
        Finish(); 
    }

    m_Stream->Close(); 
    m_Stream = NULL; 
}

void ArchiveXML::Read()
{
    REFLECT_SCOPE_TIMER(( "Reflect - XML Read" ));

    if (m_Status != NULL)
    {
        StatusInfo info (*this, ArchiveStates::Starting);
        m_Status->ArchiveStatus(info);
    }

    m_Abort = false;

    // determine the size of the input stream
    m_Stream->SeekRead(0, std::ios_base::end);
    std::streamsize size = m_Stream->TellRead();
    m_Stream->SeekRead(0, std::ios_base::beg);

    // fail on an empty input stream
    if ( size == 0 )
    {
        throw Reflect::StreamException( TXT( "Input stream is empty" ) );
    }

    // setup visitors
    PreDeserialize();

    // while there is data, parse buffer
    long step = 0;
    const unsigned bufferSizeInBytes = 4096;
    while (!m_Stream->Fail() && !m_Abort)
    {
        m_Progress = (int)(((float)(step++ * bufferSizeInBytes) / (float)size) * 100.0f);

        tchar* pszBuffer = (tchar*)XML_GetBuffer(m_Parser, bufferSizeInBytes); // REQUEST
        HELIUM_ASSERT(pszBuffer != NULL);

        // divide by the character size so wide char builds don't override the allocation
        //  stream objects read characters, not byte-by-byte
        m_Stream->ReadBuffer(pszBuffer, bufferSizeInBytes / sizeof(tchar));

        int last_read = static_cast<int>(m_Stream->ElementsRead());
        if (!XML_ParseBuffer(m_Parser, last_read * sizeof(tchar), last_read == 0) != 0)
        {
            throw Reflect::DataFormatException( TXT( "XML parsing failure, buffer contents:\n%s" ), (const tchar*)pszBuffer);
        }
    }

    if (m_Status != NULL)
    {
        StatusInfo info (*this, ArchiveStates::ElementProcessed);
        info.m_Progress = 100;
        m_Status->ArchiveStatus(info);
    }

    // tell visitors to process append
    PostDeserialize(m_Append);

    if (m_Status != NULL)
    {
        StatusInfo info (*this, ArchiveStates::Complete);
        m_Status->ArchiveStatus(info);
    }
}

void ArchiveXML::Write()
{
    REFLECT_SCOPE_TIMER(( "Reflect - XML Write" ));

    if (m_Status != NULL)
    {
        StatusInfo info (*this, ArchiveStates::Starting);
        m_Status->ArchiveStatus(info);
    }

    // setup visitors
    PreSerialize();

    // serialize main file elements
    Serialize(m_Spool, ArchiveFlags::Status);

    // tell visitors to generate append
    V_Element append;
    PostSerialize(append);

    // serialize appended file elements
    if (!append.empty())
    {
        m_Indent.Push();
        m_Indent.Get( *m_Stream );
        *m_Stream << "<Append/>\n";
        m_Indent.Pop();

        Serialize(append);
    }

    if (m_Status != NULL)
    {
        StatusInfo info (*this, ArchiveStates::Complete);
        m_Status->ArchiveStatus(info);
    }
}

void ArchiveXML::Start()
{
#ifdef UNICODE
    u16 feff = 0xfeff;
    m_Stream->Write( &feff ); // byte order mark
#endif

    *m_Stream << "<?xml version=\"1.0\" encoding=\"" << Helium::GetEncoding() << "\"?>\n";
    *m_Stream << "<Reflect FileFormatVersion=\"" << m_Version << "\">\n";
}

void ArchiveXML::Finish()
{
    *m_Stream << "</Reflect>\n";
}

void ArchiveXML::Serialize(const ElementPtr& element)
{
    PreSerialize(element);

    {
        REFLECT_SCOPE_TIMER_INST( ("PreSerialize %s", element->GetClass()->m_ShortName.c_str()) );

        element->PreSerialize();
    }

    SerializeHeader(element);

    if (element->HasType(Reflect::GetType<Serializer>()))
    {
        Serializer* s = DangerousCast<Serializer>(element);

        s->Serialize(*this);
    }
    else
    {
        SerializeFields(element);
    }

    SerializeFooter(element);

    {
        REFLECT_SCOPE_TIMER_INST( ("PostSerialize %s", element->GetClass()->m_ShortName.c_str()) );

        element->PostSerialize();
    }
}

void ArchiveXML::Serialize(const V_Element& elements, u32 flags)
{
    m_FieldNames.push( tstring () );

    V_Element::const_iterator itr = elements.begin();
    V_Element::const_iterator end = elements.end();
    for (int index = 0; itr != end; ++itr, ++index )
    {
        Serialize(*itr);

        if (flags & ArchiveFlags::Status && m_Status != NULL)
        {
            StatusInfo info (*this, ArchiveStates::ElementProcessed);
            info.m_Progress = (int)(((float)(index) / (float)elements.size()) * 100.0f);
            m_Status->ArchiveStatus(info);
        }
    }

    if (flags & ArchiveFlags::Status && m_Status != NULL)
    {
        StatusInfo info (*this, ArchiveStates::ElementProcessed);
        info.m_Progress = 100;
        m_Status->ArchiveStatus(info);
    }

    m_FieldNames.pop();
}

void ArchiveXML::SerializeFields(const ElementPtr& element)
{
    //
    // Serialize fields
    //

    const Class* type = element->GetClass();
    HELIUM_ASSERT(type != NULL);

    M_FieldIDToInfo::const_iterator iter = type->m_FieldIDToInfo.begin();
    M_FieldIDToInfo::const_iterator end  = type->m_FieldIDToInfo.end();
    for ( ; iter != end; ++iter )
    {
        SerializeField(element, iter->second);
    }
}

void ArchiveXML::SerializeField(const ElementPtr& element, const Field* field)
{
    // don't write no write fields
    if ( field->m_Flags & FieldFlags::Discard )
    {
        return;
    }

    // set current field name
    m_FieldNames.push(field->m_Name);

    // construct serialization object
    ElementPtr e;
    m_Cache.Create( field->m_SerializerID, e );

    HELIUM_ASSERT( e != NULL );

    // downcast serializer
    SerializerPtr serializer = ObjectCast<Serializer>(e);

    if (!serializer.ReferencesObject())
    {
        // this should never happen, the type id in the rtti data is bogus
        throw Reflect::TypeInformationException( TXT( "Invalid type id for field '%s'" ), field->m_Name.c_str() );
    }
    else
    {
        // set data pointer
        serializer->ConnectField(element.Ptr(), field);

        // bool for test results
        bool serialize = true;

        // check for equality
        if ( serialize && field->m_Default.ReferencesObject() )
        {
            bool force = (field->m_Flags & FieldFlags::Force) != 0;
            if (!force && field->m_Default->Equals(serializer))
            {
                serialize = false;
            }
        }

        // don't write empty containers
        if ( serialize &&  e->HasType( Reflect::GetType<ContainerSerializer>() ) )
        {
            ContainerSerializerPtr container = DangerousCast<ContainerSerializer>(e);

            if ( container->GetSize() == 0 )
            {
                serialize = false;
            }
        }

        // last chance to not write, call through virtual API
        if (serialize)
        {
            PreSerialize(element, field);

            // process
            Serialize( serializer );
        }

        // disconnect
        serializer->Disconnect();
    }

    m_FieldNames.pop();
}

void ArchiveXML::SerializeHeader(const ElementPtr& element)
{
    //
    // Start header
    //

    m_Indent.Push();
    m_Indent.Get( *m_Stream );
    *m_Stream << "<Element Type=\"" << element->GetClass()->m_ShortName << "\"";

    //
    // Field name
    //

    if (!m_FieldNames.empty() && !m_FieldNames.top().empty())
    {
        // our link back to the field we are nested in
        *m_Stream << " Name=\"" << m_FieldNames.top() << "\"";
    }

    //
    // End header
    //

    if ( element->IsCompact() )
    {
        *m_Stream << ">";
    }
    else
    {
        *m_Stream << ">\n";
    }
}

void ArchiveXML::SerializeFooter(const ElementPtr& element)
{
    if ( !element->IsCompact() )
    {
        m_Indent.Get(*m_Stream);
    }

    *m_Stream << "</Element>\n";

    m_Indent.Pop();
}

void ArchiveXML::Deserialize(ElementPtr& element)
{
    if (m_Components.size() == 1)
    {
        element = m_Components.front();
        m_Components.clear();
    }
    else
    {
        // xml doesn't work this way
        HELIUM_BREAK();
        throw Reflect::LogisticException( TXT( "Internal Error: Missing element" ) );
    }
}

void ArchiveXML::Deserialize(V_Element& elements, u32 flags)
{
    if (!m_Components.empty())
    {
        if ( !(flags & ArchiveFlags::Sparse) )
        {
            m_Components.erase( std::remove( m_Components.begin(), m_Components.end(), ElementPtr () ), m_Components.end() );
        }

        elements = m_Components;

        m_Components.clear();
    }
}

void ArchiveXML::OnStartElement(const XML_Char *pszName, const XML_Char **papszAttrs)
{
    if (m_Abort)
    {
        return;
    }

    if ( !_tcscmp(pszName, TXT( "Reflect" ) ) )
    {
        if ( papszAttrs[0] && papszAttrs[1] && papszAttrs[0] && _tcsicmp(papszAttrs[0], TXT( "FileFormatVersion" ) ) == 0 )
        {
            tistringstream str ( papszAttrs[1] );
            str >> m_Version;
            if (str.fail())
            {
                throw Reflect::DataFormatException( TXT( "Unable to read file format version" ) );
            }
        }
        else
        {
            m_Version = 1;
        }

        return;
    }

    if (!_tcscmp(pszName, TXT( "Append" )))
    {
        m_Target = &m_Append;
        return;
    }

    //
    // Find element type
    //

    tstring elementType;

    if ( m_Version < FIRST_VERSION_WITH_NAMESPACE_SUPPORT )
    {
        bool converted = Helium::ConvertString( pszName, elementType );
        HELIUM_ASSERT( converted );
    }

    if ( elementType.empty() )
    {
        for (int i=0; papszAttrs[i]; i+=2)
        {
            if ( !_tcscmp( papszAttrs[i], TXT( "Type" ) ) )
            {
                bool converted = Helium::ConvertString( papszAttrs[ i + 1 ], elementType );
                HELIUM_ASSERT( converted );
            }
        }
    }

    if ( elementType.empty() )
    {
        HELIUM_BREAK();
        throw Reflect::DataFormatException( TXT( "Unable to find element type attribute" ) );
    }

    // 
    // We use a stack to track the state of parsing, this will be the new state
    //

    ParsingStatePtr newState = new ParsingState (elementType.c_str());
    ParsingStatePtr topState = m_StateStack.empty() ? NULL : m_StateStack.top();

    //
    // First pass at creation:
    //  Check parent for a serializer matching this element... handles serializers and field elements
    //

    if ( topState && topState->m_Element )
    {
        // pointer to the parent element below which we are nested
        ElementPtr parentElement = topState->m_Element;

        // retrieve the type information for our parent structure
        const Class* parentTypeDefinition = Registry::GetInstance()->GetClass( parentElement->GetType() );

        if ( parentTypeDefinition )
        {
            // look for the field name in the attributes
            const tchar* fieldName = NULL;
            for (int i=0; papszAttrs[i]; i+=2)
            {
                if ( !_tcscmp( papszAttrs[i], TXT( "Name" ) ) )
                {
                    fieldName = papszAttrs[i+1];
                }
            }

            if ( fieldName )
            {
                newState->m_Field = parentTypeDefinition->FindFieldByName( fieldName );
            }

            // we have found a fieldinfo into our parent's definition
            if (newState->m_Field != NULL)
            {
                // this is our new element
                ElementPtr element = NULL;

                // create the object
                m_Cache.Create(newState->m_Field->m_SerializerID, element);

                // if we are a serializer
                if (element->HasType(Reflect::GetType<Serializer>()))
                {
                    // connect the current instance to the serializer
                    DangerousCast<Serializer>(element)->ConnectField(parentElement.Ptr(), newState->m_Field);
                }

                if (element != NULL)
                {
                    // flag this as a field
                    newState->SetFlag( ParsingState::kField, true );

                    // use this element to Parse with
                    newState->m_Element = element;
                }
            }
        }
    }

    //
    // Second pass at creation:
    //  Try and get a creator for a new element to store the data
    //

    if (newState->m_Element == NULL)
    {
        //
        // Attempt creation of element via short name
        //

        m_Cache.Create(elementType, newState->m_Element);

        if (newState->m_Element == NULL)
        {
            Debug( TXT( "Unable to create element with short name: %s\n" ), elementType);
        }
    }

    //
    // Do callbacks
    //

    if (newState->m_Element)
    {
        REFLECT_SCOPE_TIMER_INST( ("PreDeserialize %s", newState->m_Element->GetClass()->m_ShortName.c_str()) );

        newState->m_Element->PreDeserialize();
    }

    //
    // Push state
    //

    m_StateStack.push( newState );

#ifdef REFLECT_DISPLAY_PARSE_STACK
    m_Indent.Push();
    m_Indent.Get(std::cout);
    Log::Print("<Element>\n");
#endif 
}

void ArchiveXML::OnCharacterData(const XML_Char *pszData, int nLength)
{
    if (m_Abort)
    {
        return;
    }

    ParsingStatePtr topState = m_StateStack.empty() ? NULL : m_StateStack.top();
    if ( topState && topState->m_Element )
    {
        topState->m_Buffer.append( pszData, nLength );
    }
}

void ArchiveXML::OnEndElement(const XML_Char *pszName)
{
    if (m_Abort)
    {
        return;
    }

    if (!_tcscmp(pszName, TXT( "Reflect" ) ))
    {
        return;
    }

    if (!_tcscmp(pszName, TXT( "Append" ) ))
    {
        return;
    }

    // this should never happen, an element just ended
    HELIUM_ASSERT( !m_StateStack.empty() );
    ParsingStatePtr topState = m_StateStack.top();

    // we own this state, do pop it off the stack
    m_StateStack.pop();

#ifdef REFLECT_DISPLAY_PARSE_STACK
    m_Indent.Get(std::cout);
    Log::Print("</Element>\n");
    m_Indent.Pop();
#endif

    if ( topState->m_Element )
    {
        // do Serializer logic
        if ( topState->m_Element->HasType(Reflect::GetType<Serializer>()) && !topState->m_Buffer.empty())
        {
            Serializer* serializer = DangerousCast<Serializer>(topState->m_Element);

            tstringstream stream (topState->m_Buffer);

            ArchiveXML xml (m_Status);
            xml.m_Stream = new Reflect::TCharStream (&stream); 
            xml.m_Components = topState->m_Components;
            serializer->Deserialize(xml);
        }

        // do callbacks
        if ( topState->m_Element )
        {
            REFLECT_SCOPE_TIMER_INST( ("PostDeserialize %s", topState->m_Element->GetClass()->m_ShortName.c_str()) );

            if ( !TryElementCallback( topState->m_Element, &Element::PostDeserialize ) )
            {
                topState->m_Element = NULL; // discard the object
            }
        }

        if ( topState->m_Element )
        {
            PostDeserialize( topState->m_Element );

            // are we nested within another element?
            ParsingStatePtr parentState = m_StateStack.empty() ? NULL : m_StateStack.top();

            // if we are we should see if it's being processed and perhaps be added as a component
            if ( parentState != NULL )
            {
                // see if we should process this element as a as a field, or as a component
                if ( topState->GetFlag( ParsingState::kField ) )
                {
                    SerializerPtr serializer = ObjectCast<Serializer>(topState->m_Element);
                    if ( serializer.ReferencesObject() )
                    {
                        // disconnect our serializer for neatness
                        serializer->Disconnect();

                        // send it back to the free store
                        m_Cache.Free(topState->m_Element);
                    }

                    PostDeserialize( parentState->m_Element, topState->m_Field );
                }
                else
                {
                    ElementPtr container = parentState->m_Element;

                    // we are a component, so send us up to be processed by container
                    if (container && !container->ProcessComponent(topState->m_Element, topState->m_Field->m_Name))
                    {
                        Debug( TXT( "%s did not process %s, discarding\n" ), container->GetClass()->m_ShortName.c_str(), topState->m_Element->GetClass()->m_ShortName.c_str());
                    }
                }
            }
        }
    }

    // if this is a top level element push the result into the target (even if its null)
    if ( !m_StateStack.empty() )
    {
        ParsingStatePtr parentState = m_StateStack.top();

        parentState->m_Components.push_back(topState->m_Element);
    }
    else if ( topState->m_Element != NULL )
    {
        // we've reached the top of the processed stack, send off to client for processing
        m_Target->push_back( topState->m_Element );

        if (m_Status != NULL)
        {
            StatusInfo info (*this, ArchiveStates::ElementProcessed);
            info.m_Progress = m_Progress;
            m_Status->ArchiveStatus(info);

            m_Abort |= info.m_Abort;
        }
    }
}

void ArchiveXML::ToString(const ElementPtr& element, tstring& xml, StatusHandler* status)
{
    V_Element elements(1);
    elements[0] = element;
    return ToString( elements, xml, status );
}

ElementPtr ArchiveXML::FromString(const tstring& xml, int searchType, StatusHandler* status)
{
    if (searchType == Reflect::ReservedTypes::Any)
    {
        searchType = Reflect::GetType<Element>();
    }

    ArchiveXML archive (status);
    archive.m_SearchType = searchType;

    tstringstream strStream;
    strStream << "<?xml version=\"1.0\"?><Reflect FileFormatVersion=\""<<ArchiveXML::CURRENT_VERSION<<"\">" << xml << "</Reflect>";
    archive.m_Stream = new Reflect::TCharStream(&strStream); 
    archive.Read();

    V_Element::iterator itr = archive.m_Spool.begin();
    V_Element::iterator end = archive.m_Spool.end();
    for ( ; itr != end; ++itr )
    {
        if ((*itr)->HasType(searchType))
        {
            return *itr;
        }
    }

    return NULL;
}

void ArchiveXML::ToString(const V_Element& elements, tstring& xml, StatusHandler* status)
{
    ArchiveXML archive (status);
    tstringstream strStream;

    archive.m_Stream = new Reflect::TCharStream(&strStream); 
    archive.m_Spool  = elements;
    archive.Write();

    xml = strStream.str();
}

void ArchiveXML::FromString(const tstring& xml, V_Element& elements, StatusHandler* status)
{
    ArchiveXML archive (status);
    tstringstream strStream;
    strStream << "<?xml version=\"1.0\"?><Reflect FileFormatVersion=\""<<ArchiveXML::CURRENT_VERSION<<"\">" << xml << "</Reflect>";

    archive.m_Stream = new Reflect::TCharStream(&strStream); 
    archive.Read();

    elements = archive.m_Spool;
}
