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

#include <cmath>
#include <iostream>
#include "rs_window.h"
#include "rs_wall.h"
#include "rs_line.h"
#include "rs_debug.h"

RS_WindowData::RS_WindowData()
    : positionAlongWall(0.0)
    , width(36.0)
{
}

RS_WindowData::RS_WindowData(double _positionAlongWall,
                             double _width)
    : positionAlongWall(_positionAlongWall)
    , width(_width)
{
}

std::ostream& operator << (std::ostream& os, const RS_WindowData& wd) {
    os << "(pos=" << wd.positionAlongWall
       << ",width=" << wd.width << ")";
    return os;
}

RS_Window::RS_Window(RS_EntityContainer* parent, const RS_WindowData& d)
    : RS_EntityContainer(parent)
    , data(d)
{
    calculateBorders();
}

RS_Entity* RS_Window::clone() const {
    RS_Window* w = new RS_Window(*this);
    w->setOwner(isOwner());
    w->initId();
    w->detach();
    return w;
}

void RS_Window::update() {
    clear();

    if (isUndone()) {
        return;
    }

    // Get the parent wall to compute geometry
    RS_EntityContainer* parentEntity = getParent();
    if (!parentEntity) return;

    RS_Wall* wall = nullptr;
    if (parentEntity->rtti() == RS2::EntityWall) {
        wall = static_cast<RS_Wall*>(parentEntity);
    }
    if (!wall) return;

    RS_Vector wallStart = wall->getStartpoint();
    RS_Vector wallEnd = wall->getEndpoint();
    RS_Vector wallDir = wallEnd - wallStart;
    double wallLength = wallDir.magnitude();
    if (wallLength < RS_TOLERANCE) return;

    double wallAngle = wallDir.angle();
    RS_Vector wallUnit = wallDir / wallLength;
    double halfThick = wall->getThickness() / 2.0;
    double halfWidth = data.width / 2.0;

    // Window center point on wall centerline
    RS_Vector centerPt = wallStart + wallUnit * data.positionAlongWall;

    // Opening edge points on centerline
    RS_Vector openStart = wallStart + wallUnit * (data.positionAlongWall - halfWidth);
    RS_Vector openEnd = wallStart + wallUnit * (data.positionAlongWall + halfWidth);

    // Perpendicular offset for glass pane lines (slightly inset from wall faces)
    // Draw two parallel lines representing the glass, offset by 1/4 of wall thickness
    RS_Vector perp = RS_Vector::polar(halfThick * 0.25, wallAngle + M_PI_2);

    // Glass pane line 1
    RS_Line* glass1 = new RS_Line(this, openStart + perp, openEnd + perp);
    glass1->setLayer(nullptr);
    addEntity(glass1);

    // Glass pane line 2
    RS_Line* glass2 = new RS_Line(this, openStart - perp, openEnd - perp);
    glass2->setLayer(nullptr);
    addEntity(glass2);

    calculateBorders();
}

RS_Vector RS_Window::getGripPoint() const {
    RS_EntityContainer* parentEntity = getParent();
    if (!parentEntity || parentEntity->rtti() != RS2::EntityWall)
        return RS_Vector(false);

    RS_Wall* wall = static_cast<RS_Wall*>(parentEntity);
    RS_Vector wallStart = wall->getStartpoint();
    RS_Vector wallEnd = wall->getEndpoint();
    RS_Vector wallDir = wallEnd - wallStart;
    double wallLength = wallDir.magnitude();
    if (wallLength < RS_TOLERANCE) return RS_Vector(false);

    return wallStart + wallDir * (data.positionAlongWall / wallLength);
}

std::ostream& operator << (std::ostream& os, const RS_Window& w) {
    os << " Window: " << w.data << "\n";
    return os;
}
