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
#include "rs_wall.h"
#include "rs_line.h"
#include "rs_debug.h"

RS_WallData::RS_WallData()
    : startpoint(false)
    , endpoint(false)
    , thickness(6.0)
{
}

RS_WallData::RS_WallData(const RS_Vector& _startpoint,
                         const RS_Vector& _endpoint,
                         double _thickness)
    : startpoint(_startpoint)
    , endpoint(_endpoint)
    , thickness(_thickness)
{
}

std::ostream& operator << (std::ostream& os, const RS_WallData& wd) {
    os << "(" << wd.startpoint << "," << wd.endpoint
       << ",thickness=" << wd.thickness << ")";
    return os;
}

RS_Wall::RS_Wall(RS_EntityContainer* parent, const RS_WallData& d)
    : RS_EntityContainer(parent)
    , data(d)
{
    calculateBorders();
}

RS_Entity* RS_Wall::clone() const {
    RS_Wall* w = new RS_Wall(*this);
    w->setOwner(isOwner());
    w->initId();
    w->detach();
    return w;
}

RS_VectorSolutions RS_Wall::getRefPoints() const {
    return RS_VectorSolutions({
        data.startpoint,
        data.endpoint,
        (data.startpoint + data.endpoint) / 2.0
    });
}

void RS_Wall::update() {
    clear();

    if (isUndone()) {
        return;
    }

    double halfThick = data.thickness / 2.0;
    RS_Vector dir = data.endpoint - data.startpoint;
    double angle = dir.angle();
    RS_Vector perp = RS_Vector::polar(halfThick, angle + M_PI_2);

    // Two offset lines along the wall
    RS_Line* line1 = new RS_Line(this,
        data.startpoint + perp,
        data.endpoint + perp);
    line1->setLayer(nullptr);
    addEntity(line1);

    RS_Line* line2 = new RS_Line(this,
        data.startpoint - perp,
        data.endpoint - perp);
    line2->setLayer(nullptr);
    addEntity(line2);

    // End caps
    RS_Line* cap1 = new RS_Line(this,
        data.startpoint - perp,
        data.startpoint + perp);
    cap1->setLayer(nullptr);
    addEntity(cap1);

    RS_Line* cap2 = new RS_Line(this,
        data.endpoint - perp,
        data.endpoint + perp);
    cap2->setLayer(nullptr);
    addEntity(cap2);

    calculateBorders();
}

void RS_Wall::move(const RS_Vector& offset) {
    data.startpoint.move(offset);
    data.endpoint.move(offset);
    update();
}

void RS_Wall::rotate(const RS_Vector& center, const double& angle) {
    RS_Vector angleVector(angle);
    data.startpoint.rotate(center, angleVector);
    data.endpoint.rotate(center, angleVector);
    update();
}

void RS_Wall::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    data.startpoint.rotate(center, angleVector);
    data.endpoint.rotate(center, angleVector);
    update();
}

void RS_Wall::scale(const RS_Vector& center, const RS_Vector& factor) {
    data.startpoint.scale(center, factor);
    data.endpoint.scale(center, factor);
    data.thickness *= (fabs(factor.x) + fabs(factor.y)) / 2.0;
    update();
}

void RS_Wall::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    data.startpoint.mirror(axisPoint1, axisPoint2);
    data.endpoint.mirror(axisPoint1, axisPoint2);
    update();
}

void RS_Wall::stretch(const RS_Vector& firstCorner,
                      const RS_Vector& secondCorner,
                      const RS_Vector& offset) {
    if (getMin().isInWindow(firstCorner, secondCorner) &&
        getMax().isInWindow(firstCorner, secondCorner)) {
        move(offset);
    } else {
        if (data.startpoint.isInWindow(firstCorner, secondCorner)) {
            data.startpoint.move(offset);
        }
        if (data.endpoint.isInWindow(firstCorner, secondCorner)) {
            data.endpoint.move(offset);
        }
        update();
    }
}

void RS_Wall::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    if (ref.distanceTo(data.startpoint) < 1.0e-4) {
        data.startpoint += offset;
        update();
    } else if (ref.distanceTo(data.endpoint) < 1.0e-4) {
        data.endpoint += offset;
        update();
    }
}

std::ostream& operator << (std::ostream& os, const RS_Wall& w) {
    os << " Wall: " << w.data << "\n";
    return os;
}
