#pragma once

#include "Foundation/API.h"
#include "Foundation/Inspect/Interpreters/Reflect/ReflectFieldInterpreter.h"

#include "Foundation/FileDialog.h"

namespace Helium
{
    namespace Inspect
    {
        // Forwards
        class Button;
        class List;
        struct FilteredDropTargetArgs;

        class FOUNDATION_API PathContainerInterpreter : public ReflectFieldInterpreter
        {
        public:
            PathContainerInterpreter (Container* container);

            virtual void InterpretField(const Reflect::Field* field, const std::vector<Reflect::Element*>& instances, Container* parent);

        private:
            // callbacks
            void OnAdd( const ButtonClickedArgs& args );
            void OnAddFile( const ButtonClickedArgs& args );
            void OnFindFile( const ButtonClickedArgs& args );
            void OnEdit( const ButtonClickedArgs& args );
            void OnRemove( const ButtonClickedArgs& args );
            void OnMoveUp( const ButtonClickedArgs& args );
            void OnMoveDown( const ButtonClickedArgs& args );

            FileDialogSignature::Delegate d_OpenFileDialog;
        private:
            Inspect::List* m_List;
        };

        typedef Helium::SmartPtr<PathContainerInterpreter> PathContainerInterpreterPtr;
        typedef std::vector< PathContainerInterpreterPtr > V_PathContainerInterpreterSmartPtr;
    }
}