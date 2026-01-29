/****************************************************************************
**
** Minimal XRef proof-of-concept for LibreCAD
**
***************************************************************************/

#ifndef RS_XREF_H
#define RS_XREF_H

#include <QString>

class RS_Graphic;
class RS_EntityContainer;

/**
 * Minimal external reference holder (POC).
 * Loads an external DXF/DWG into an internal RS_Graphic and
 * clones its top-level entities into a holder container attached
 * to the host graphic. No transformation or layer overrides yet.
 */
class RS_XRef {
public:
    explicit RS_XRef(const QString& path, RS_Graphic* host);
    ~RS_XRef();

    const QString& path() const { return m_path; }
    bool load();
    bool reload();

    RS_EntityContainer* holder() const { return m_holder; }

private:
    QString m_path;
    RS_Graphic* m_host = nullptr;
    // referenced document (owned)
    RS_Graphic* m_doc = nullptr;
    // container inserted into host that holds cloned entities
    RS_EntityContainer* m_holder = nullptr;
};

#endif
