/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024-2025 LibreCAD contributors
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include <QMouseEvent>
#include "rs_actiondrawttext.h"
#include "rs_ttext.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"

RS_ActionDrawTText::RS_ActionDrawTText(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
    : RS_PreviewActionInterface("Draw TrueType Text", container, graphicView)
{
    actionType = RS2::ActionDrawTText;
}

RS_ActionDrawTText::~RS_ActionDrawTText() = default;

void RS_ActionDrawTText::init(int status) {
    RS_PreviewActionInterface::init(status);

    if (status == ShowDialog || !data) {
        // Show dialog to get text parameters
        RS_TTextData newData;
        newData.height = 10.0;
        newData.angle = 0.0;
        newData.letterSpacing = 1.0;
        data = std::make_unique<RS_TTextData>(newData);

        RS_TText tmp(nullptr, *data);
        if (RS_DIALOGFACTORY->requestTTextDialog(&tmp)) {
            *data = tmp.getData();
            setStatus(SetPos);
        } else {
            finish(false);
        }
    }
}

void RS_ActionDrawTText::trigger() {
    RS_PreviewActionInterface::trigger();

    if (!data) return;

    RS_TText* ttext = new RS_TText(container, *data);
    ttext->setLayerToActive();
    ttext->setPenToActive();
    container->addEntity(ttext);

    if (document) {
        document->startUndoCycle();
        document->addUndoable(ttext);
        document->endUndoCycle();
    }

    graphicView->redraw(RS2::RedrawDrawing);

    RS_DEBUG->print("RS_ActionDrawTText::trigger(): ttext added");
}

void RS_ActionDrawTText::mouseMoveEvent(QMouseEvent* e) {
    if (getStatus() == SetPos) {
        RS_Vector mouse = snapPoint(e);
        if (data) {
            deletePreview();
            RS_TTextData previewData = *data;
            previewData.insertionPoint = mouse;
            RS_TText* ttext = new RS_TText(preview.get(), previewData);
            preview->addEntity(ttext);
            drawPreview();
        }
    }
}

void RS_ActionDrawTText::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button() == Qt::RightButton) {
        deletePreview();
        init(getStatus() - 1);
    }
}

void RS_ActionDrawTText::coordinateEvent(RS_CoordinateEvent* e) {
    if (!e) return;

    if (getStatus() == SetPos && data) {
        data->insertionPoint = e->getCoordinate();
        trigger();
        setStatus(SetPos);  // allow placing again
    }
}

void RS_ActionDrawTText::updateMouseButtonHints() {
    switch (getStatus()) {
    case ShowDialog:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Enter TrueType text parameters"), tr("Cancel"));
        break;
    case SetPos:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify insertion point"), tr("Cancel"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void RS_ActionDrawTText::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}
