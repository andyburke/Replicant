#pragma once

#include <set>

#include "API.h"
#include "Foundation/Atomic.h"

namespace Helium
{
    namespace Reflect
    {
        class Structure;
        class Field;

        class FOUNDATION_API Visitor : public Helium::AtomicRefCountBase
        {
        public:
            virtual bool VisitElement(Element* element)
            {
                return true;
            }

            virtual bool VisitPointer(ElementPtr& pointer)
            {
                return true;
            }

            virtual bool VisitField(Element* element, const Field* field)
            {
                return true; 
            }
        };

        typedef Helium::SmartPtr<Visitor> VisitorPtr;

        class FOUNDATION_API FindByTypeVisitor : public Visitor
        {
        public:
            i32 m_Type;

            V_Element& m_Found;

            FindByTypeVisitor(i32 type, V_Element& found);

            virtual bool VisitElement(Element* element) HELIUM_OVERRIDE;
        };

        class FOUNDATION_API FindByTypeSetVisitor : public Visitor
        {
        public:
            const std::set< i32 >& m_Types;

            V_Element& m_Found;

            FindByTypeSetVisitor(const std::set< i32 >& types, V_Element& found);

            virtual bool VisitElement(Element* element) HELIUM_OVERRIDE;
        };
    }
}