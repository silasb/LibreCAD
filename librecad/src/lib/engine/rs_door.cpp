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
    : swingAngle(M_PI_2)
    , swingLeft(true)
    , hingeReversed(false)
{
}

RS_DoorData::RS_DoorData(double _swingAngle,
                         bool _swingLeft,
                         bool _hingeReversed)
    : swingAngle(_swingAngle)
    , swingLeft(_swingLeft)
    , hingeReversed(_hingeReversed)
{
}

std::ostream& operator << (std::ostream& os, const RS_DoorData& dd) {
    os << "(swing=" << dd.swingAngle
       << ",left=" << dd.swingLeft
       << ",hingeRev=" << dd.hingeReversed << ")";
    return os;
}

RS_Door::RS_Door(RS_EntityContainer* parent,
                 const RS_WallOpeningData& od,
                 const RS_DoorData& d)
    : RS_WallOpening(parent, od)
    , data(d)
{
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

    double halfWidth = openingData.width / 2.0;
    RS_Vector hingePoint;
    double closedAngle;

    bool hingeOnStartSide = data.swingLeft ^ data.hingeReversed;

    if (hingeOnStartSide) {
        double hingePos = openingData.positionAlongWall - halfWidth;
        hingePoint = wallStart + wallUnit * hingePos;
        closedAngle = wallAngle;
    } else {
        double hingePos = openingData.positionAlongWall + halfWidth;
        hingePoint = wallStart + wallUnit * hingePos;
        closedAngle = wallAngle + M_PI;
    }

    double openAngle;
    if (hingeOnStartSide) {
        openAngle = data.swingLeft
            ? wallAngle + data.swingAngle
            : wallAngle - data.swingAngle;
    } else {
        openAngle = data.swingLeft
            ? wallAngle + M_PI + data.swingAngle
            : wallAngle + M_PI - data.swingAngle;
    }

    RS_Vector leafEnd = hingePoint + RS_Vector::polar(openingData.width, openAngle);

    RS_Line* leaf = new RS_Line(this, hingePoint, leafEnd);
    leaf->setLayer(nullptr);
    addEntity(leaf);

    double a1 = closedAngle;
    double a2 = openAngle;
    double ccwSweep = fmod(a2 - a1 + 4 * M_PI, 2 * M_PI);
    double arcStartAngle, arcEndAngle;
    bool reversed = false;
    if (ccwSweep <= M_PI) {
        arcStartAngle = a1;
        arcEndAngle = a2;
        reversed = false;
    } else {
        arcStartAngle = a2;
        arcEndAngle = a1;
        reversed = false;
    }

    RS_ArcData arcData(hingePoint, openingData.width,
                       arcStartAngle, arcEndAngle, reversed);
    RS_Arc* arc = new RS_Arc(this, arcData);
    arc->setLayer(nullptr);
    addEntity(arc);

    calculateBorders();
}

std::ostream& operator << (std::ostream& os, const RS_Door& d) {
    os << " Door: " << d.openingData << " " << d.data << "\n";
    return os;
}
