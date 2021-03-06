#pragma once

#include "ReflectFieldInterpreter.h"

#include "Foundation/Inspect/Canvas.h"
#include "Foundation/Inspect/Container.h"

namespace Helium
{
    namespace Inspect
    {
        class Button;
        class Container;

        class FOUNDATION_API ReflectSetInterpreter : public ReflectFieldInterpreter
        {
        public:
            ReflectSetInterpreter( Container* container );

            virtual void InterpretField( const Reflect::Field* field, const std::vector<Reflect::Element*>& instances, Container* parent ) HELIUM_OVERRIDE;

        private:
            // callbacks
            void OnAdd( const ButtonClickedArgs& args );
            void OnRemove( const ButtonClickedArgs& args );
        };

        typedef Helium::SmartPtr<ReflectSetInterpreter> ReflectSetInterpreterPtr;
        typedef std::vector< ReflectSetInterpreterPtr > V_ReflectSetInterpreterSmartPtr;
    }
}