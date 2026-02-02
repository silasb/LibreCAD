/****************************************************************************
**
** Minimal XRef proof-of-concept for LibreCAD
**
***************************************************************************/

#ifndef RS_XREF_H
#define RS_XREF_H

#include <QString>

class RS_Entity;
class RS_Graphic;
class RS_EntityContainer;
class RS_Layer;

/**
 * Minimal external reference holder (POC).
 * Loads an external DXF/DWG into an internal RS_Graphic and
 * clones its top-level entities into a holder container attached
 * to the host graphic.  Layers from the referenced file are
 * imported into the host with the prefix  xrefs-<filename>-
 * so they sort together and don't collide with host layers.
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
    /** Return "xrefs-<basename>-<layerName>" for the current path. */
    QString prefixedLayerName(const QString& layerName) const;
    /** Ensure a prefixed layer exists in the host, creating it from
     *  the source layer's pen if necessary. */
    void ensureHostLayer(const QString& prefixedName, RS_Layer* srcLayer);
    /** Recursively remap layers and pens on clone to match src. */
    void remapEntityLayers(RS_Entity* src, RS_Entity* clone);

    QString m_path;
    RS_Graphic* m_host = nullptr;
    // referenced document (owned)
    RS_Graphic* m_doc = nullptr;
    // container inserted into host that holds cloned entities
    RS_EntityContainer* m_holder = nullptr;
};

#endif
