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

RS_Vector RS_Wall::getNearestRef(const RS_Vector& coord,
                                  double* dist) const {
    return RS_Entity::getNearestRef(coord, dist);
}

RS_Vector RS_Wall::getNearestSelectedRef(const RS_Vector& coord,
                                         double* dist) const {
    return RS_Entity::getNearestSelectedRef(coord, dist);
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

    // Raw offset corner points
    RS_Vector startLeft = data.startpoint + perp;
    RS_Vector startRight = data.startpoint - perp;
    RS_Vector endLeft = data.endpoint + perp;
    RS_Vector endRight = data.endpoint - perp;

    bool hasStartJoin = false;
    bool hasEndJoin = false;

    // Check for neighbor at start
    RS_Wall* neighborStart = findNeighborAt(data.startpoint);
    if (neighborStart) {
        RS_Vector jLeft, jRight;
        computeJoinPoints(neighborStart, data.startpoint, jLeft, jRight);
        if (jLeft.valid && jRight.valid) {
            startLeft = jLeft;
            startRight = jRight;
            hasStartJoin = true;
        }
    }

    // Check for neighbor at end
    RS_Wall* neighborEnd = findNeighborAt(data.endpoint);
    if (neighborEnd) {
        RS_Vector jLeft, jRight;
        computeJoinPoints(neighborEnd, data.endpoint, jLeft, jRight);
        if (jLeft.valid && jRight.valid) {
            endLeft = jLeft;
            endRight = jRight;
            hasEndJoin = true;
        }
    }

    // Two offset lines along the wall
    RS_Line* line1 = new RS_Line(this, startLeft, endLeft);
    line1->setLayer(nullptr);
    addEntity(line1);

    RS_Line* line2 = new RS_Line(this, startRight, endRight);
    line2->setLayer(nullptr);
    addEntity(line2);

    // End caps (only where there's no join)
    if (!hasStartJoin) {
        RS_Line* cap1 = new RS_Line(this, startRight, startLeft);
        cap1->setLayer(nullptr);
        addEntity(cap1);
    }

    if (!hasEndJoin) {
        RS_Line* cap2 = new RS_Line(this, endRight, endLeft);
        cap2->setLayer(nullptr);
        addEntity(cap2);
    }

    calculateBorders();
}

RS_Wall* RS_Wall::findNeighborAt(const RS_Vector& point) const {
    RS_EntityContainer* parentContainer = getParent();
    if (!parentContainer) return nullptr;

    for (auto e : *parentContainer) {
        if (!e) continue;
        if (e == this) continue;
        if (e->rtti() != RS2::EntityWall) continue;
        if (e->isUndone()) continue;

        RS_Wall* other = static_cast<RS_Wall*>(e);

        // Must be on same layer
        if (getLayer() != other->getLayer()) continue;

        if (other->getStartpoint().distanceTo(point) < RS_TOLERANCE ||
            other->getEndpoint().distanceTo(point) < RS_TOLERANCE) {
            return other;
        }
    }
    return nullptr;
}

void RS_Wall::computeJoinPoints(const RS_Wall* neighbor,
                                const RS_Vector& sharedPoint,
                                RS_Vector& leftPt,
                                RS_Vector& rightPt) const {
    leftPt = RS_Vector(false);
    rightPt = RS_Vector(false);

    // This wall's geometry
    double halfThickA = data.thickness / 2.0;
    RS_Vector dirA = data.endpoint - data.startpoint;
    double angleA = dirA.angle();
    RS_Vector perpA = RS_Vector::polar(halfThickA, angleA + M_PI_2);

    // Neighbor wall's geometry
    double halfThickB = neighbor->getThickness() / 2.0;
    RS_Vector dirB = neighbor->getEndpoint() - neighbor->getStartpoint();
    double angleB = dirB.angle();
    RS_Vector perpB = RS_Vector::polar(halfThickB, angleB + M_PI_2);

    // Check if walls are parallel
    double cross = dirA.x * dirB.y - dirA.y * dirB.x;
    if (fabs(cross) < RS_TOLERANCE) {
        return;
    }

    // Offset lines for this wall (infinite lines defined by two points)
    RS_Vector aLeft1 = data.startpoint + perpA;
    RS_Vector aLeft2 = data.endpoint + perpA;
    RS_Vector aRight1 = data.startpoint - perpA;
    RS_Vector aRight2 = data.endpoint - perpA;

    // Offset lines for neighbor wall
    RS_Vector bLeft1 = neighbor->getStartpoint() + perpB;
    RS_Vector bLeft2 = neighbor->getEndpoint() + perpB;
    RS_Vector bRight1 = neighbor->getStartpoint() - perpB;
    RS_Vector bRight2 = neighbor->getEndpoint() - perpB;

    auto lineIntersect = [](const RS_Vector& p1, const RS_Vector& p2,
                            const RS_Vector& p3, const RS_Vector& p4) -> RS_Vector {
        double d = (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x);
        if (fabs(d) < 1.0e-10) return RS_Vector(false);
        double t = ((p1.x - p3.x) * (p3.y - p4.y) - (p1.y - p3.y) * (p3.x - p4.x)) / d;
        return p1 + (p2 - p1) * t;
    };

    RS_Vector intLL = lineIntersect(aLeft1, aLeft2, bLeft1, bLeft2);
    RS_Vector intLR = lineIntersect(aLeft1, aLeft2, bRight1, bRight2);
    RS_Vector intRL = lineIntersect(aRight1, aRight2, bLeft1, bLeft2);
    RS_Vector intRR = lineIntersect(aRight1, aRight2, bRight1, bRight2);

    // Two possible pairings of corner points:
    //   "same":  {intLL, intRR} — this-left meets neighbor-left, this-right meets neighbor-right
    //   "cross": {intLR, intRL} — this-left meets neighbor-right, this-right meets neighbor-left
    //
    // Both walls must select the SAME geometric pair. Use total distance to
    // shared point — the correct pair is closer. For 90° equal-thickness walls
    // where distances tie, use a deterministic tiebreaker based on the cross
    // product of away-from-corner directions, ordered consistently by entity ID.

    double distSame = 1e20, distCross = 1e20;
    if (intLL.valid && intRR.valid)
        distSame = intLL.distanceTo(sharedPoint) + intRR.distanceTo(sharedPoint);
    if (intLR.valid && intRL.valid)
        distCross = intLR.distanceTo(sharedPoint) + intRL.distanceTo(sharedPoint);

    bool useSame;
    if (fabs(distSame - distCross) > RS_TOLERANCE) {
        useSame = (distSame < distCross);
    } else {
        // Tiebreaker for 90° walls: use cross product of away-from-corner
        // directions, ordered by angle so the result is drawing-order invariant.
        bool sharedIsMyEnd = (data.endpoint.distanceTo(sharedPoint) < RS_TOLERANCE);
        bool sharedIsNeighborEnd = (neighbor->getEndpoint().distanceTo(sharedPoint) < RS_TOLERANCE);
        RS_Vector awayMe = sharedIsMyEnd ? (data.startpoint - data.endpoint)
                                         : (data.endpoint - data.startpoint);
        RS_Vector awayNb = sharedIsNeighborEnd ? (neighbor->getStartpoint() - neighbor->getEndpoint())
                                               : (neighbor->getEndpoint() - neighbor->getStartpoint());

        // Sort the two away vectors by angle so both walls compute
        // the same cross product regardless of drawing order.
        double angleMe = awayMe.angle();
        double angleNb = awayNb.angle();
        RS_Vector first, second;
        if (angleMe < angleNb) {
            first = awayMe;
            second = awayNb;
        } else {
            first = awayNb;
            second = awayMe;
        }
        double awayCross = first.x * second.y - first.y * second.x;

        // The sign determines which pairing avoids line crossing at the corner.
        useSame = (awayCross < 0);
    }

    if (useSame) {
        leftPt = intLL;
        rightPt = intRR;
    } else {
        leftPt = intLR;
        rightPt = intRL;
    }
}

void RS_Wall::updateNeighbors() {
    RS_EntityContainer* parentContainer = getParent();
    if (!parentContainer) return;

    for (auto e : *parentContainer) {
        if (!e || e == this) continue;
        if (e->rtti() != RS2::EntityWall) continue;
        if (e->isUndone()) continue;

        RS_Wall* other = static_cast<RS_Wall*>(e);
        if (getLayer() != other->getLayer()) continue;

        // Check if this neighbor shares an endpoint with us
        if (other->getStartpoint().distanceTo(data.startpoint) < RS_TOLERANCE ||
            other->getStartpoint().distanceTo(data.endpoint) < RS_TOLERANCE ||
            other->getEndpoint().distanceTo(data.startpoint) < RS_TOLERANCE ||
            other->getEndpoint().distanceTo(data.endpoint) < RS_TOLERANCE) {
            other->update();
        }
    }
}

void RS_Wall::move(const RS_Vector& offset) {
    RS_DEBUG->print("silas - move");
    updateNeighbors();  // update former neighbors before moving
    data.startpoint.move(offset);
    data.endpoint.move(offset);
    updateNeighbors();  // update new neighbors after moving
    update();
}

void RS_Wall::rotate(const RS_Vector& center, const double& angle) {
      RS_DEBUG->print("silas - rotate");
    updateNeighbors();
    RS_Vector angleVector(angle);
    data.startpoint.rotate(center, angleVector);
    data.endpoint.rotate(center, angleVector);
    updateNeighbors();
    update();
}

void RS_Wall::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    updateNeighbors();
    data.startpoint.rotate(center, angleVector);
    data.endpoint.rotate(center, angleVector);
    updateNeighbors();
    update();
}

void RS_Wall::scale(const RS_Vector& center, const RS_Vector& factor) {
    updateNeighbors();
    data.startpoint.scale(center, factor);
    data.endpoint.scale(center, factor);
    data.thickness *= (fabs(factor.x) + fabs(factor.y)) / 2.0;
    updateNeighbors();
    update();
}

void RS_Wall::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    updateNeighbors();
    data.startpoint.mirror(axisPoint1, axisPoint2);
    data.endpoint.mirror(axisPoint1, axisPoint2);
    updateNeighbors();
    update();
}

void RS_Wall::stretch(const RS_Vector& firstCorner,
                      const RS_Vector& secondCorner,
                      const RS_Vector& offset) {
    if (getMin().isInWindow(firstCorner, secondCorner) &&
        getMax().isInWindow(firstCorner, secondCorner)) {
        move(offset);
    } else {
        updateNeighbors();
        if (data.startpoint.isInWindow(firstCorner, secondCorner)) {
            data.startpoint.move(offset);
        }
        if (data.endpoint.isInWindow(firstCorner, secondCorner)) {
            data.endpoint.move(offset);
        }
        updateNeighbors();
        update();
    }
}

void RS_Wall::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
      RS_DEBUG->print("silas - moveref");
    if (ref.distanceTo(data.startpoint) < 1.0e-4) {
        updateNeighbors();
        data.startpoint += offset;
        updateNeighbors();
        update();
    } else if (ref.distanceTo(data.endpoint) < 1.0e-4) {
        updateNeighbors();
        data.endpoint += offset;
        updateNeighbors();
        update();
    }
}

std::ostream& operator << (std::ostream& os, const RS_Wall& w) {
    os << " Wall: " << w.data << "\n";
    return os;
}
