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

RS_Window::RS_Window(RS_EntityContainer* parent, const RS_WallOpeningData& d)
    : RS_WallOpening(parent, d)
{
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
    double halfWidth = openingData.width / 2.0;

    RS_Vector openStart = wallStart + wallUnit * (openingData.positionAlongWall - halfWidth);
    RS_Vector openEnd = wallStart + wallUnit * (openingData.positionAlongWall + halfWidth);

    RS_Vector perp = RS_Vector::polar(halfThick * 0.25, wallAngle + M_PI_2);

    RS_Line* glass1 = new RS_Line(this, openStart + perp, openEnd + perp);
    glass1->setLayer(nullptr);
    addEntity(glass1);

    RS_Line* glass2 = new RS_Line(this, openStart - perp, openEnd - perp);
    glass2->setLayer(nullptr);
    addEntity(glass2);

    calculateBorders();
}

std::ostream& operator << (std::ostream& os, const RS_Window& w) {
    os << " Window: " << w.getOpeningData() << "\n";
    return os;
}
