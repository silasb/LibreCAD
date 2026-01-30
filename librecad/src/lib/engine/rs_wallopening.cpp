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
