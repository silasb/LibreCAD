#include "rs_xref.h"

#include "rs_graphic.h"
#include "rs_fileio.h"
#include "rs_entitycontainer.h"
#include "rs_debug.h"
#include "rs_layer.h"

RS_XRef::RS_XRef(const QString& path, RS_Graphic* host)
    : m_path(path), m_host(host)
{
}

RS_XRef::~RS_XRef()
{
    // remove holder from host if present
    if (m_holder && m_host) {
        // RS_EntityContainer::removeEntity will delete the entity
        // if the container owns its entities (default for documents).
        // Avoid double-delete by not deleting the holder here.
        m_host->removeEntity(m_holder);
        m_holder = nullptr;
    }
    if (m_doc) {
        delete m_doc;
        m_doc = nullptr;
    }
}

bool RS_XRef::load()
{
    if (!m_host) return false;

    RS_DEBUG->print("RS_XRef: loading %s", m_path.toLatin1().data());

    // create a new RS_Graphic and try to open the referenced file
    m_doc = new RS_Graphic(nullptr);
    // try autodetect format
    RS2::FormatType t = RS_FileIO::detectFormat(m_path, true);
    if (!m_doc->open(m_path, t)) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_XRef: failed to open %s\n", m_path.toLatin1().data());
        delete m_doc;
        m_doc = nullptr;
        return false;
    }

    // create a holder container parented to the host document
    m_holder = new RS_EntityContainer(m_host, true);

    // clone top-level entities from referenced doc into holder
    const QList<RS_Entity*>& ents = m_doc->getEntityList();
    for (RS_Entity* e: ents) {
        if (e) {
            RS_Entity* c = e->clone();
            if (c) {
                // set the cloned entity's parent to the holder without
                // touching its internal sub-entity parents. Using
                // `setParent()` keeps sub-entities parented to the
                // cloned container itself and avoids corrupting the
                // containment hierarchy.
                c->setParent(m_holder);
                // remap layer names to the host graphic so layers resolve to host layers
                if (e->getLayer()) {
                    c->setLayer(e->getLayer()->getName());
                }
                m_holder->addEntity(c);
            }
        }
    }

    // add holder to host so it will be drawn. Call the
    // RS_EntityContainer::addEntity implementation directly to
    // avoid RS_Graphic::addEntity flattening container children
    // into the host's entity list.
    if (m_host) {
        m_host->RS_EntityContainer::addEntity(m_holder);
    }

    RS_DEBUG->print("RS_XRef: loaded %s (entities: %d)", m_path.toLatin1().data(), m_holder->count());
    return true;
}

bool RS_XRef::reload()
{
    RS_DEBUG->print("RS_XRef: reload %s", m_path.toLatin1().data());

    if (!m_host) return false;

    // Load the new referenced file into temporary structures first.
    RS_Graphic* newDoc = new RS_Graphic(nullptr);
    RS2::FormatType t = RS_FileIO::detectFormat(m_path, true);
    if (!newDoc->open(m_path, t)) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_XRef: failed to open %s on reload\n", m_path.toLatin1().data());
        delete newDoc;
        return false;
    }

    RS_EntityContainer* newHolder = new RS_EntityContainer(m_host, true);
    const QList<RS_Entity*>& ents = newDoc->getEntityList();
    for (RS_Entity* e: ents) {
        if (e) {
            RS_Entity* c = e->clone();
            if (c) {
                c->setParent(newHolder);
                if (e->getLayer()) {
                    c->setLayer(e->getLayer()->getName());
                }
                newHolder->addEntity(c);
            }
        }
    }

    // Add the new holder into the host first to keep the host in a
    // consistent state while we remove the old holder/doc.
    // Temporarily disable automatic border updates while we swap
    // holders so calculateBorders() is not triggered on a partially
    // updated entity list (avoids accessing freed/dangling pointers).
    m_host->setAutoUpdateBorders(false);

    // Add the new holder into the host using the base
    // RS_EntityContainer implementation to avoid flattening.
    m_host->RS_EntityContainer::addEntity(newHolder);

    // Remove and delete the old holder (if any).
    if (m_holder) {
        // remove using base implementation as well (removeEntity
        // is not overridden in RS_Graphic but call explicitly for symmetry)
        m_host->RS_EntityContainer::removeEntity(m_holder);
        m_holder = nullptr;
    }

    // Delete the old referenced document.
    if (m_doc) {
        delete m_doc;
        m_doc = nullptr;
    }

    // Commit the new structures.
    m_doc = newDoc;
    m_holder = newHolder;

    // Re-enable border updates and refresh once.
    m_host->setAutoUpdateBorders(true);
    m_host->calculateBorders();

    RS_DEBUG->print("RS_XRef: reloaded %s (entities: %d)", m_path.toLatin1().data(), m_holder->count());
    return true;
}
