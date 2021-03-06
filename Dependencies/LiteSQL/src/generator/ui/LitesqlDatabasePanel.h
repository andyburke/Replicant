#ifndef __LitesqlDatabasePanel__
#define __LitesqlDatabasePanel__

/**
@file
Subclass of DatabasePanel, which is generated by wxFormBuilder.
*/

#include "ui.h"

namespace xml {
  class Database;
};

namespace ui {

  /** Implementing DatabasePanel */
  class LitesqlDatabasePanel : public DatabasePanel
  {
  public:
    /** Constructor */
    LitesqlDatabasePanel( wxWindow* parent, xml::Database* db );

  private:
    xml::Database* m_database;
  };

}
#endif // __LitesqlDatabasePanel__