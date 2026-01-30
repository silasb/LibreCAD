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
#include "rs_door.h"
#include "rs_wall.h"
#include "rs_line.h"
#include "rs_arc.h"
#include "rs_debug.h"

RS_DoorData::RS_DoorData()
    : positionAlongWall(0.0)
    , width(36.0)
    , swingAngle(M_PI_2)
    , swingLeft(true)
    , hingeReversed(false)
{
}

RS_DoorData::RS_DoorData(double _positionAlongWall,
                         double _width,
                         double _swingAngle,
                         bool _swingLeft,
                         bool _hingeReversed)
    : positionAlongWall(_positionAlongWall)
    , width(_width)
    , swingAngle(_swingAngle)
    , swingLeft(_swingLeft)
    , hingeReversed(_hingeReversed)
{
}

std::ostream& operator << (std::ostream& os, const RS_DoorData& dd) {
    os << "(pos=" << dd.positionAlongWall
       << ",width=" << dd.width
       << ",swing=" << dd.swingAngle
       << ",left=" << dd.swingLeft
       << ",hingeRev=" << dd.hingeReversed << ")";
    return os;
}

RS_Door::RS_Door(RS_EntityContainer* parent, const RS_DoorData& d)
    : RS_EntityContainer(parent)
    , data(d)
{
    calculateBorders();
}

RS_Entity* RS_Door::clone() const {
    RS_Door* d = new RS_Door(*this);
    d->setOwner(isOwner());
    d->initId();
    d->detach();
    return d;
}

void RS_Door::update() {
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

    // Hinge point is at one edge of the opening along the wall centerline.
    // The door opening is centered at positionAlongWall.
    //
    // Default (hingeReversed=false):
    //   swingLeft:  hinge on start side of opening
    //   swingRight: hinge on end side of opening
    // Flipped (hingeReversed=true): hinge on the opposite end.
    double halfWidth = data.width / 2.0;
    RS_Vector hingePoint;
    double closedAngle;

    bool hingeOnStartSide = data.swingLeft ^ data.hingeReversed;

    if (hingeOnStartSide) {
        double hingePos = data.positionAlongWall - halfWidth;
        hingePoint = wallStart + wallUnit * hingePos;
        closedAngle = wallAngle;  // leaf points toward wall endpoint
    } else {
        double hingePos = data.positionAlongWall + halfWidth;
        hingePoint = wallStart + wallUnit * hingePos;
        closedAngle = wallAngle + M_PI;  // leaf points toward wall startpoint
    }

    // Open position: leaf perpendicular to wall, away from wall body
    double openAngle;
    if (hingeOnStartSide) {
        // Swing away from wall: left or right depending on swingLeft
        openAngle = data.swingLeft
            ? wallAngle + data.swingAngle
            : wallAngle - data.swingAngle;
    } else {
        openAngle = data.swingLeft
            ? wallAngle + M_PI + data.swingAngle
            : wallAngle + M_PI - data.swingAngle;
    }

    RS_Vector leafEnd = hingePoint + RS_Vector::polar(data.width, openAngle);

    // Door leaf line (shows the door in open position)
    RS_Line* leaf = new RS_Line(this, hingePoint, leafEnd);
    leaf->setLayer(nullptr);
    addEntity(leaf);

    // Swing arc from closed to open position.
    // Use the shorter arc between closedAngle and openAngle.
    // Normalize both to [0, 2*PI) and pick direction for shorter arc.
    double a1 = closedAngle;
    double a2 = openAngle;
    // Compute CCW sweep from a1 to a2
    double ccwSweep = fmod(a2 - a1 + 4 * M_PI, 2 * M_PI);
    double arcStartAngle, arcEndAngle;
    bool reversed = false;
    if (ccwSweep <= M_PI) {
        // CCW is the short way
        arcStartAngle = a1;
        arcEndAngle = a2;
        reversed = false;
    } else {
        // CW is the short way
        arcStartAngle = a2;
        arcEndAngle = a1;
        reversed = false;
    }

    RS_ArcData arcData(hingePoint, data.width,
                       arcStartAngle, arcEndAngle, reversed);
    RS_Arc* arc = new RS_Arc(this, arcData);
    arc->setLayer(nullptr);
    addEntity(arc);

    calculateBorders();
}

RS_Vector RS_Door::getGripPoint() const {
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

std::ostream& operator << (std::ostream& os, const RS_Door& d) {
    os << " Door: " << d.data << "\n";
    return os;
}
