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
#include <cmath>
#include "rs_actiondrawdoor.h"
#include "rs_wall.h"
#include "rs_door.h"
#include "rs_line.h"
#include "rs_arc.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_layer.h"

RS_ActionDrawDoor::RS_ActionDrawDoor(RS_EntityContainer& container,
                                     RS_GraphicView& graphicView)
    : RS_PreviewActionInterface("Draw door", container, graphicView)
{
    actionType = RS2::ActionDrawDoor;
}

RS_ActionDrawDoor::~RS_ActionDrawDoor() = default;

void RS_ActionDrawDoor::init(int status) {
    RS_PreviewActionInterface::init(status);
    selectedWall = nullptr;
}

void RS_ActionDrawDoor::ensureDoorLayer() {
    RS_Graphic* graphic = container->getGraphic();
    if (!graphic) return;

    RS_Layer* doorLayer = graphic->findLayer("A - Door");
    if (!doorLayer) {
        doorLayer = new RS_Layer("A - Door");
        doorLayer->setPen(RS_Pen(RS_Color(0, 255, 255),
                                 RS2::Width01,
                                 RS2::SolidLine));
        graphic->addLayer(doorLayer);
    }
    graphic->activateLayer(doorLayer);
}

RS_Wall* RS_ActionDrawDoor::findNearestWall(const RS_Vector& coord) {
    RS_Wall* nearest = nullptr;
    double minDist = RS_MAXDOUBLE;

    for (auto e : *container) {
        if (!e || e->isUndone()) continue;
        if (e->rtti() != RS2::EntityWall) continue;

        RS_Wall* wall = static_cast<RS_Wall*>(e);
        RS_Entity* tmp = nullptr;
        double dist = wall->getDistanceToPoint(coord, &tmp);
        if (dist < minDist) {
            minDist = dist;
            nearest = wall;
        }
    }

    // Only select if reasonably close
    if (nearest && minDist < graphicView->toGraphDX(30)) {
        return nearest;
    }
    return nullptr;
}

double RS_ActionDrawDoor::projectOnWall(RS_Wall* wall, const RS_Vector& coord) {
    RS_Vector wallStart = wall->getStartpoint();
    RS_Vector wallEnd = wall->getEndpoint();
    RS_Vector wallDir = wallEnd - wallStart;
    double wallLength = wallDir.magnitude();
    if (wallLength < RS_TOLERANCE) return 0.0;

    RS_Vector toCoord = coord - wallStart;
    double dot = toCoord.x * wallDir.x + toCoord.y * wallDir.y;
    double t = dot / (wallLength * wallLength);

    // Clamp to valid range ensuring door fits
    double halfW = doorWidth / 2.0;
    double minPos = halfW;
    double maxPos = wallLength - halfW;
    if (minPos > maxPos) {
        return wallLength / 2.0;  // wall too short, center it
    }

    double pos = t * wallLength;
    return std::max(minPos, std::min(maxPos, pos));
}

void RS_ActionDrawDoor::trigger() {
    RS_PreviewActionInterface::trigger();

    if (!selectedWall) return;

    ensureDoorLayer();

    RS_Vector mouse = graphicView->getRelativeZero();
    double posAlongWall = projectOnWall(selectedWall, mouse);

    RS_DoorData doorData(posAlongWall, doorWidth, M_PI_2, swingLeft, hingeReversed);
    RS_Door* door = new RS_Door(selectedWall, doorData);
    door->setLayerToActive();
    door->setPenToActive();
    selectedWall->addEntity(door);

    // Regenerate wall geometry with door gap
    selectedWall->update();

    if (document) {
        document->startUndoCycle();
        document->addUndoable(door);
        document->endUndoCycle();
    }

    graphicView->redraw(RS2::RedrawDrawing);

    RS_DEBUG->print("RS_ActionDrawDoor::trigger(): door added to wall %lu",
                    selectedWall->getId());

    // Go back to wall selection for placing more doors
    selectedWall = nullptr;
    setStatus(SelectWall);
}

void RS_ActionDrawDoor::mouseMoveEvent(QMouseEvent* e) {
    RS_Vector mouse = snapFree(e);

    switch (getStatus()) {
    case SelectWall: {
        deletePreview();
        RS_Wall* wall = findNearestWall(mouse);
        if (wall) {
            // Highlight wall by drawing its centerline in preview
            RS_Line* cl = new RS_Line(preview.get(),
                                      wall->getStartpoint(),
                                      wall->getEndpoint());
            preview->addEntity(cl);
            drawPreview();
        }
        break;
    }

    case SetPosition: {
        if (selectedWall) {
            deletePreview();

            double pos = projectOnWall(selectedWall, mouse);

            // Create a temporary door on the wall to generate preview geometry
            RS_DoorData tmpData(pos, doorWidth, M_PI_2, swingLeft, hingeReversed);
            RS_Door tmpDoor(selectedWall, tmpData);
            tmpDoor.update();

            // Copy the door's generated geometry into the preview
            for (RS_Entity* child = tmpDoor.firstEntity(RS2::ResolveNone);
                 child; child = tmpDoor.nextEntity(RS2::ResolveNone)) {
                preview->addEntity(child->clone());
            }

            // Show door opening gap on wall
            RS_Vector wallStart = selectedWall->getStartpoint();
            RS_Vector wallEnd = selectedWall->getEndpoint();
            RS_Vector wallDir = wallEnd - wallStart;
            double wallLength = wallDir.magnitude();
            if (wallLength > RS_TOLERANCE) {
                double wallAngle = wallDir.angle();
                RS_Vector wallUnit = wallDir / wallLength;
                double halfW = doorWidth / 2.0;
                RS_Vector gapStart = wallStart + wallUnit * (pos - halfW);
                RS_Vector gapEnd = wallStart + wallUnit * (pos + halfW);
                double halfThick = selectedWall->getThickness() / 2.0;
                RS_Vector perpVec = RS_Vector::polar(halfThick,
                                                     wallAngle + M_PI_2);
                RS_Line* gapLine1 = new RS_Line(preview.get(),
                    gapStart + perpVec, gapStart - perpVec);
                preview->addEntity(gapLine1);
                RS_Line* gapLine2 = new RS_Line(preview.get(),
                    gapEnd + perpVec, gapEnd - perpVec);
                preview->addEntity(gapLine2);
            }

            drawPreview();
        }
        break;
    }

    default:
        break;
    }
}

void RS_ActionDrawDoor::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        RS_Vector mouse = snapFree(e);

        switch (getStatus()) {
        case SelectWall: {
            RS_Wall* wall = findNearestWall(mouse);
            if (wall) {
                selectedWall = wall;
                graphicView->moveRelativeZero(mouse);
                setStatus(SetPosition);
            } else {
                RS_DIALOGFACTORY->commandMessage(
                    tr("No wall found. Click near a wall."));
            }
            break;
        }

        case SetPosition: {
            graphicView->moveRelativeZero(mouse);
            trigger();
            break;
        }

        default:
            break;
        }
    } else if (e->button() == Qt::RightButton) {
        deletePreview();
        if (getStatus() == SetPosition) {
            selectedWall = nullptr;
            setStatus(SelectWall);
        } else {
            init(getStatus() - 1);
        }
    }
}

void RS_ActionDrawDoor::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    if (c == tr("width") || c == "width" || c == "w") {
        RS_DIALOGFACTORY->commandMessage(tr("Enter door width:"));
        e->accept();
        // Next command input will be parsed as width
        // For simplicity, handle inline
    } else if (c == tr("swing") || c == "swing" || c == "s") {
        swingLeft = !swingLeft;
        RS_DIALOGFACTORY->commandMessage(
            tr("Swing direction: %1").arg(swingLeft ? tr("left") : tr("right")));
        e->accept();
    } else if (c == tr("flip") || c == "flip" || c == "f") {
        hingeReversed = !hingeReversed;
        RS_DIALOGFACTORY->commandMessage(
            tr("Hinge side: %1").arg(hingeReversed ? tr("reversed") : tr("normal")));
        e->accept();
    } else {
        // Try to parse as width value
        bool ok;
        double val = RS_Math::eval(c, &ok);
        if (ok && val > RS_TOLERANCE) {
            doorWidth = val;
            RS_DIALOGFACTORY->commandMessage(
                tr("Door width set to: %1").arg(doorWidth));
            e->accept();
        }
    }
}

QStringList RS_ActionDrawDoor::getAvailableCommands() {
    QStringList cmd;
    cmd += tr("width");
    cmd += tr("swing");
    cmd += tr("flip");
    return cmd;
}

void RS_ActionDrawDoor::updateMouseButtonHints() {
    switch (getStatus()) {
    case SelectWall:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Click on a wall to place door"),
            tr("Cancel"));
        break;
    case SetPosition:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Click position along wall or [%1/%2/%3]")
                .arg(tr("width")).arg(tr("swing")).arg(tr("flip")),
            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void RS_ActionDrawDoor::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}
