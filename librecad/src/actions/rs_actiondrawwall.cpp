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
#include "rs_actiondrawwall.h"
#include "rs_wall.h"
#include "rs_line.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_layer.h"

RS_ActionDrawWall::RS_ActionDrawWall(RS_EntityContainer& container,
                                     RS_GraphicView& graphicView)
    : RS_PreviewActionInterface("Draw wall", container, graphicView)
    , startpoint(false)
{
    actionType = RS2::ActionDrawWall;
}

RS_ActionDrawWall::~RS_ActionDrawWall() = default;

void RS_ActionDrawWall::init(int status) {
    RS_PreviewActionInterface::init(status);
    startpoint = RS_Vector(false);
}

void RS_ActionDrawWall::ensureWallLayer() {
    RS_Graphic* graphic = container->getGraphic();
    if (!graphic) return;

    RS_Layer* wallLayer = graphic->findLayer("A - Wall");
    if (!wallLayer) {
        wallLayer = new RS_Layer("A - Wall");
        wallLayer->setPen(RS_Pen(RS_Color(255, 255, 255),
                                 RS2::Width01,
                                 RS2::SolidLine));
        graphic->addLayer(wallLayer);
    }
    graphic->activateLayer(wallLayer);
}

void RS_ActionDrawWall::trigger() {
    RS_PreviewActionInterface::trigger();

    ensureWallLayer();

    RS_Vector mouse = graphicView->getRelativeZero();
    RS_WallData wallData(startpoint, mouse, thickness);
    RS_Wall* wall = new RS_Wall(container, wallData);
    wall->setLayerToActive();
    wall->setPenToActive();
    wall->update();
    container->addEntity(wall);

    if (document) {
        document->startUndoCycle();
        document->addUndoable(wall);
        document->endUndoCycle();
    }

    graphicView->redraw(RS2::RedrawDrawing);

    RS_DEBUG->print("RS_ActionDrawWall::trigger(): wall added: %lu",
                    wall->getId());
}

void RS_ActionDrawWall::mouseMoveEvent(QMouseEvent* e) {
    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
    case SetStartpoint:
        break;

    case SetEndpoint:
        if (startpoint.valid) {
            deletePreview();

            RS_WallData wallData(startpoint, mouse, thickness);
            RS_Wall* wall = new RS_Wall(preview.get(), wallData);
            wall->update();
            preview->addEntity(wall);

            drawPreview();
        }
        break;

    default:
        break;
    }
}

void RS_ActionDrawWall::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button() == Qt::RightButton) {
        deletePreview();
        init(getStatus() - 1);
    }
}

void RS_ActionDrawWall::coordinateEvent(RS_CoordinateEvent* e) {
    if (!e) return;

    RS_Vector pos = e->getCoordinate();

    switch (getStatus()) {
    case SetStartpoint:
        startpoint = pos;
        graphicView->moveRelativeZero(pos);
        setStatus(SetEndpoint);
        break;

    case SetEndpoint:
        if ((pos - startpoint).squared() > RS_TOLERANCE2) {
            graphicView->moveRelativeZero(pos);
            trigger();
            // Continue drawing: start next wall from this endpoint
            startpoint = pos;
        }
        break;

    default:
        break;
    }
}

void RS_ActionDrawWall::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetThickness: {
        bool ok;
        double t = RS_Math::eval(c, &ok);
        if (ok && t > RS_TOLERANCE) {
            thickness = t;
        } else {
            RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        RS_DIALOGFACTORY->requestOptions(this, true, true);
        setStatus(lastStatus);
    }
    break;

    default:
        lastStatus = (Status)getStatus();
        if (checkCommand("thickness", c)) {
            setStatus(SetThickness);
            e->accept();
        }
        break;
    }
}

QStringList RS_ActionDrawWall::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetStartpoint:
    case SetEndpoint:
        cmd += command("thickness");
        break;
    default:
        break;
    }

    return cmd;
}

void RS_ActionDrawWall::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetStartpoint:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify wall start point"),
            tr("Cancel"));
        break;
    case SetEndpoint:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify wall end point"),
            tr("Back"));
        break;
    case SetThickness:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Enter wall thickness:"), "");
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void RS_ActionDrawWall::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

void RS_ActionDrawWall::showOptions() {
    RS_ActionInterface::showOptions();
    RS_DIALOGFACTORY->requestOptions(this, true, true);
}

void RS_ActionDrawWall::hideOptions() {
    RS_ActionInterface::hideOptions();
    RS_DIALOGFACTORY->requestOptions(this, false);
}

// EOF
