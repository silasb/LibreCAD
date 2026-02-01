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
#include "rs_wallopening.h"
#include "rs_wall.h"

RS_WallOpeningData::RS_WallOpeningData()
    : positionAlongWall(0.0)
    , width(36.0)
{
}

RS_WallOpeningData::RS_WallOpeningData(double _positionAlongWall, double _width)
    : positionAlongWall(_positionAlongWall)
    , width(_width)
{
}

std::ostream& operator << (std::ostream& os, const RS_WallOpeningData& od) {
    os << "(pos=" << od.positionAlongWall
       << ",width=" << od.width << ")";
    return os;
}

RS_WallOpening::RS_WallOpening(RS_EntityContainer* parent,
                               const RS_WallOpeningData& d)
    : RS_EntityContainer(parent)
    , openingData(d)
{
    calculateBorders();
}

RS_Vector RS_WallOpening::getGripPoint() const {
    RS_EntityContainer* parentEntity = getParent();
    if (!parentEntity || parentEntity->rtti() != RS2::EntityWall)
        return RS_Vector(false);

    RS_Wall* wall = static_cast<RS_Wall*>(parentEntity);
    RS_Vector wallStart = wall->getStartpoint();
    RS_Vector wallEnd = wall->getEndpoint();
    RS_Vector wallDir = wallEnd - wallStart;
    double wallLength = wallDir.magnitude();
    if (wallLength < RS_TOLERANCE) return RS_Vector(false);

    return wallStart + wallDir * (openingData.positionAlongWall / wallLength);
}

RS_Vector RS_WallOpening::getNearestEndpoint(const RS_Vector& coord,
                                             double* dist) const {
    RS_EntityContainer* parentEntity = getParent();
    if (!parentEntity || parentEntity->rtti() != RS2::EntityWall)
        return RS_Vector(false);

    RS_Wall* wall = static_cast<RS_Wall*>(parentEntity);
    RS_Vector wallDir = wall->getEndpoint() - wall->getStartpoint();
    double wallLength = wallDir.magnitude();
    if (wallLength < RS_TOLERANCE) return RS_Vector(false);

    RS_Vector wallUnit = wallDir / wallLength;
    double halfW = openingData.width / 2.0;
    RS_Vector edgeA = wall->getStartpoint() + wallUnit * (openingData.positionAlongWall - halfW);
    RS_Vector edgeB = wall->getStartpoint() + wallUnit * (openingData.positionAlongWall + halfW);

    double distA = (edgeA - coord).squared();
    double distB = (edgeB - coord).squared();
    if (dist)
        *dist = std::sqrt(std::min(distA, distB));
    return (distA < distB) ? edgeA : edgeB;
}

RS_Vector RS_WallOpening::getNearestMiddle(const RS_Vector& coord,
                                           double* dist,
                                           int /*middlePoints*/) const {
    RS_Vector gp = getGripPoint();
    if (dist && gp.valid)
        *dist = gp.distanceTo(coord);
    return gp;
}

RS_Vector RS_WallOpening::getNearestCenter(const RS_Vector& coord,
                                           double* dist) const {
    RS_Vector gp = getGripPoint();
    if (dist && gp.valid)
        *dist = gp.distanceTo(coord);
    return gp;
}

RS_Vector RS_WallOpening::getNearestPointOnEntity(const RS_Vector& coord,
                                                  bool onEntity,
                                                  double* dist,
                                                  RS_Entity** entity) const {
    if (entity)
        *entity = const_cast<RS_WallOpening*>(this);

    RS_EntityContainer* parentEntity = getParent();
    if (!parentEntity || parentEntity->rtti() != RS2::EntityWall) {
        if (dist) *dist = RS_MAXDOUBLE;
        return RS_Vector(false);
    }

    RS_Wall* wall = static_cast<RS_Wall*>(parentEntity);
    RS_Vector wallDir = wall->getEndpoint() - wall->getStartpoint();
    double wallLength = wallDir.magnitude();
    if (wallLength < RS_TOLERANCE) {
        if (dist) *dist = RS_MAXDOUBLE;
        return RS_Vector(false);
    }

    RS_Vector wallUnit = wallDir / wallLength;
    double halfW = openingData.width / 2.0;
    RS_Vector edgeA = wall->getStartpoint() + wallUnit * (openingData.positionAlongWall - halfW);
    RS_Vector edgeB = wall->getStartpoint() + wallUnit * (openingData.positionAlongWall + halfW);

    // Project coord onto the segment edgeA-edgeB
    RS_Vector segDir = edgeB - edgeA;
    RS_Vector vpc = coord - edgeA;
    double a = segDir.squared();
    if (a < RS_TOLERANCE * RS_TOLERANCE) {
        RS_Vector mid = (edgeA + edgeB) / 2.0;
        if (dist) *dist = mid.distanceTo(coord);
        return mid;
    }

    double t = RS_Vector::dotP(vpc, segDir) / a;
    if (onEntity) {
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;
    }

    RS_Vector ret = edgeA + segDir * t;
    if (dist)
        *dist = ret.distanceTo(coord);
    return ret;
}

RS_Vector RS_WallOpening::getNearestDist(double distance,
                                         const RS_Vector& coord,
                                         double* dist) const {
    RS_EntityContainer* parentEntity = getParent();
    if (!parentEntity || parentEntity->rtti() != RS2::EntityWall)
        return RS_Vector(false);

    RS_Wall* wall = static_cast<RS_Wall*>(parentEntity);
    RS_Vector wallDir = wall->getEndpoint() - wall->getStartpoint();
    double wallLength = wallDir.magnitude();
    if (wallLength < RS_TOLERANCE) return RS_Vector(false);

    RS_Vector wallUnit = wallDir / wallLength;
    double halfW = openingData.width / 2.0;
    RS_Vector edgeA = wall->getStartpoint() + wallUnit * (openingData.positionAlongWall - halfW);
    RS_Vector edgeB = wall->getStartpoint() + wallUnit * (openingData.positionAlongWall + halfW);

    RS_Vector dv = wallUnit * distance;
    RS_Vector ret;
    if ((coord - edgeA).squared() < (coord - edgeB).squared())
        ret = edgeA + dv;
    else
        ret = edgeB - dv;

    if (dist)
        *dist = coord.distanceTo(ret);
    return ret;
}
