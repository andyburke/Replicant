#pragma once

#include "Foundation/Math/Color3.h"
#include "Foundation/Math/Color4.h"
#include "Foundation/Reflect/Class.h"

#include "Foundation/API.h"
#include "Foundation/Inspect/Control.h"

namespace Helium
{
    namespace Inspect
    {
        class FOUNDATION_API ColorPicker : public Control
        {
        public:
            REFLECT_DECLARE_CLASS( ColorPicker, Control );

            ColorPicker();

        protected:
            virtual void SetDefaultAppearance( bool def ) HELIUM_OVERRIDE;
            void SetToDefault( const ContextMenuEventArgs& event );

        public:
            Attribute< bool >             a_Highlight;

            Attribute< bool >             a_Alpha;
            Attribute< Math::Color3 >     a_Color3;
            Attribute< Math::Color4 >     a_Color4;
        };

        typedef Helium::SmartPtr< ColorPicker > ColorPickerPtr;
    }
}
