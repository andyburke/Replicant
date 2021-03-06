#pragma once

#include "Foundation/Inspect/Canvas.h"
#include "Foundation/Inspect/Container.h"
#include "Foundation/Inspect/Interpreter.h"
#include "ReflectFieldInterpreter.h"

namespace Helium
{
    namespace Inspect
    {
        class FOUNDATION_API ReflectInterpreter : public Interpreter
        {
        public:
            ReflectInterpreter (Container* container);

            void Interpret(const std::vector<Reflect::Element*>& instances, i32 includeFlags = 0xFFFFFFFF, i32 excludeFlags = 0x0, bool expandPanel = true);
            void InterpretType(const std::vector<Reflect::Element*>& instances, Container* parent, i32 includeFlags = 0xFFFFFFFF, i32 excludeFlags = 0x0, bool expandPanel = true);

        private:
            std::vector<Reflect::Element*> m_Instances;
            std::vector<Reflect::SerializerPtr> m_Serializers;
            V_ReflectFieldInterpreter m_Interpreters;
        };

        typedef Helium::SmartPtr<ReflectInterpreter> ReflectInterpreterPtr;

        class FOUNDATION_API ReflectFieldInterpreterFactory
        {
        public:
            template< class T >
            static ReflectFieldInterpreterPtr CreateInterpreter(Container* container)
            {
                return new T ( container );
            }

            typedef ReflectFieldInterpreterPtr (*Creator)(Container* container);
            typedef std::vector< std::pair<u32, Creator> > V_Creator;
            typedef std::map< i32, V_Creator > M_Creator;

            static void Register(i32 type, u32 mask, Creator creator);

            template< class T >
            static void Register(i32 type, u32 mask = 0x0)
            {
                Register( type, mask, &CreateInterpreter<T> );
            }

            static void Unregister(i32 type, u32 mask, Creator creator);

            template< class T >
            static void Unregister(i32 type, u32 mask = 0x0)
            {
                Unregister( type, mask, &CreateInterpreter<T> );
            }

            static ReflectFieldInterpreterPtr Create(i32 type, u32 flags, Container* container);
            static void Clear();

        private:
            static M_Creator m_Map;
        };
    }
}