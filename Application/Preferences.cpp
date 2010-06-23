#include "Preferences.h"

#include "Foundation/Environment.h"

bool Application::GetPreferencesDirectory( Nocturnal::Path& preferencesDirectory )
{
    std::string prefDirectory;

    if ( !Nocturnal::GetEnvVar( "APPDATA", prefDirectory ) )
    {
        return false;
    }

    prefDirectory += "/Nocturnal/";
    
    preferencesDirectory.Set( prefDirectory );
    return true;
}