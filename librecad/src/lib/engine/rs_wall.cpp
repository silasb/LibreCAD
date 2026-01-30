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
#include <algorithm>
#include "rs_wall.h"
#include "rs_wallopening.h"
#include "rs_door.h"
#include "rs_window.h"
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
    std::vector<RS_Vector> pts;
    pts.push_back(data.startpoint);
    pts.push_back(data.endpoint);
    pts.push_back((data.startpoint + data.endpoint) / 2.0);

    for (auto e : entities) {
        if (!e || e->isUndone()) continue;
        auto* opening = dynamic_cast<RS_WallOpening*>(e);
        if (opening) {
            RS_Vector gp = opening->getGripPoint();
            if (gp.valid) {
                pts.push_back(gp);
            }
        }
    }

    return RS_VectorSolutions(std::move(pts));
}

RS_Vector RS_Wall::getNearestRef(const RS_Vector& coord,
                                  double* dist) const {
    return RS_Entity::getNearestRef(coord, dist);
}

RS_Vector RS_Wall::getNearestSelectedRef(const RS_Vector& coord,
                                         double* dist) const {
    return RS_Entity::getNearestSelectedRef(coord, dist);
}

std::vector<RS_WallOpening*> RS_Wall::getOpenings() const {
    std::vector<RS_WallOpening*> openings;
    for (auto e : entities) {
        if (!e || e->isUndone()) continue;
        auto* opening = dynamic_cast<RS_WallOpening*>(e);
        if (opening) {
            openings.push_back(opening);
        }
    }
    return openings;
}

std::vector<RS_Door*> RS_Wall::getDoors() const {
    std::vector<RS_Door*> doors;
    for (auto e : entities) {
        if (e && e->rtti() == RS2::EntityDoor && !e->isUndone()) {
            doors.push_back(static_cast<RS_Door*>(e));
        }
    }
    return doors;
}

std::vector<RS_Window*> RS_Wall::getWindows() const {
    std::vector<RS_Window*> windows;
    for (auto e : entities) {
        if (e && e->rtti() == RS2::EntityWindow && !e->isUndone()) {
            windows.push_back(static_cast<RS_Window*>(e));
        }
    }
    return windows;
}

void RS_Wall::update() {
    // Preserve opening children — remove only generated geometry
    QList<RS_Entity*> childrenToKeep;
    for (auto e : entities) {
        if (e && dynamic_cast<RS_WallOpening*>(e)) {
            childrenToKeep.append(e);
        }
    }
    for (auto c : childrenToKeep) {
        entities.removeOne(c);
    }
    clear();
    for (auto c : childrenToKeep) {
        addEntity(c);
    }

    if (isUndone()) {
        return;
    }

    double halfThick = data.thickness / 2.0;
    RS_Vector dir = data.endpoint - data.startpoint;
    double wallLength = dir.magnitude();
    if (wallLength < RS_TOLERANCE) return;

    double angle = dir.angle();
    RS_Vector perp = RS_Vector::polar(halfThick, angle + M_PI_2);
    RS_Vector wallUnit = dir / wallLength;

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

    // Collect opening gaps as parametric intervals [t_start, t_end] along wall
    struct Gap {
        double t0, t1;
    };
    std::vector<Gap> gaps;

    auto openings = getOpenings();
    for (auto* op : openings) {
        double pos = op->getPositionAlongWall();
        double halfW = op->getWidth() / 2.0;
        double t0 = (pos - halfW) / wallLength;
        double t1 = (pos + halfW) / wallLength;
        t0 = std::max(0.0, std::min(1.0, t0));
        t1 = std::max(0.0, std::min(1.0, t1));
        if (t1 > t0 + RS_TOLERANCE) {
            gaps.push_back({t0, t1});
        }
    }

    // Sort gaps by t0
    std::sort(gaps.begin(), gaps.end(), [](const Gap& a, const Gap& b) {
        return a.t0 < b.t0;
    });

    // Merge overlapping gaps
    std::vector<Gap> merged;
    for (auto& g : gaps) {
        if (!merged.empty() && g.t0 <= merged.back().t1 + RS_TOLERANCE) {
            merged.back().t1 = std::max(merged.back().t1, g.t1);
        } else {
            merged.push_back(g);
        }
    }

    // Build solid segments [0..gap0.t0], [gap0.t1..gap1.t0], ... [lastGap.t1..1]
    struct Segment {
        double t0, t1;
    };
    std::vector<Segment> segments;
    double cursor = 0.0;
    for (auto& g : merged) {
        if (g.t0 > cursor + RS_TOLERANCE) {
            segments.push_back({cursor, g.t0});
        }
        cursor = g.t1;
    }
    if (cursor < 1.0 - RS_TOLERANCE) {
        segments.push_back({cursor, 1.0});
    }

    // If no openings, single segment [0,1]
    if (segments.empty() && merged.empty()) {
        segments.push_back({0.0, 1.0});
    }

    // Generate offset lines for each segment
    for (auto& seg : segments) {
        RS_Vector segStartLeft, segStartRight, segEndLeft, segEndRight;

        if (seg.t0 < RS_TOLERANCE) {
            segStartLeft = startLeft;
            segStartRight = startRight;
        } else {
            RS_Vector pt = data.startpoint + dir * seg.t0;
            segStartLeft = pt + perp;
            segStartRight = pt - perp;
        }

        if (seg.t1 > 1.0 - RS_TOLERANCE) {
            segEndLeft = endLeft;
            segEndRight = endRight;
        } else {
            RS_Vector pt = data.startpoint + dir * seg.t1;
            segEndLeft = pt + perp;
            segEndRight = pt - perp;
        }

        RS_Line* lineL = new RS_Line(this, segStartLeft, segEndLeft);
        lineL->setLayer(nullptr);
        addEntity(lineL);

        RS_Line* lineR = new RS_Line(this, segStartRight, segEndRight);
        lineR->setLayer(nullptr);
        addEntity(lineR);
    }

    // End caps (only where there's no join and no opening at the edge)
    bool openingAtStart = !merged.empty() && merged.front().t0 < RS_TOLERANCE;
    bool openingAtEnd = !merged.empty() && merged.back().t1 > 1.0 - RS_TOLERANCE;

    RS_DEBUG->print("RS_Wall::update id=%lu: hasStartJoin=%d hasEndJoin=%d openingAtStart=%d openingAtEnd=%d neighborStart=%p neighborEnd=%p start=(%f,%f) end=(%f,%f)",
                    getId(), hasStartJoin, hasEndJoin, openingAtStart, openingAtEnd,
                    (void*)neighborStart, (void*)neighborEnd,
                    data.startpoint.x, data.startpoint.y,
                    data.endpoint.x, data.endpoint.y);
    if (neighborStart) {
        RS_DEBUG->print("  neighborStart id=%lu start=(%f,%f) end=(%f,%f)",
                        neighborStart->getId(),
                        neighborStart->getStartpoint().x, neighborStart->getStartpoint().y,
                        neighborStart->getEndpoint().x, neighborStart->getEndpoint().y);
    }
    if (neighborEnd) {
        RS_DEBUG->print("  neighborEnd id=%lu start=(%f,%f) end=(%f,%f)",
                        neighborEnd->getId(),
                        neighborEnd->getStartpoint().x, neighborEnd->getStartpoint().y,
                        neighborEnd->getEndpoint().x, neighborEnd->getEndpoint().y);
    }

    if (!hasStartJoin && !openingAtStart) {
        RS_Line* cap1 = new RS_Line(this, startRight, startLeft);
        cap1->setLayer(nullptr);
        addEntity(cap1);
    }

    if (!hasEndJoin && !openingAtEnd) {
        RS_Line* cap2 = new RS_Line(this, endRight, endLeft);
        cap2->setLayer(nullptr);
        addEntity(cap2);
    }

    // Update opening child geometry
    for (auto* op : openings) {
        op->update();
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

        if (other->isSelected()) continue;

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

    double halfThickA = data.thickness / 2.0;
    RS_Vector dirA = data.endpoint - data.startpoint;
    double angleA = dirA.angle();
    RS_Vector perpA = RS_Vector::polar(halfThickA, angleA + M_PI_2);

    double halfThickB = neighbor->getThickness() / 2.0;
    RS_Vector dirB = neighbor->getEndpoint() - neighbor->getStartpoint();
    double angleB = dirB.angle();
    RS_Vector perpB = RS_Vector::polar(halfThickB, angleB + M_PI_2);

    double cross = dirA.x * dirB.y - dirA.y * dirB.x;
    if (fabs(cross) < RS_TOLERANCE) {
        return;
    }

    RS_Vector aLeft1 = data.startpoint + perpA;
    RS_Vector aLeft2 = data.endpoint + perpA;
    RS_Vector aRight1 = data.startpoint - perpA;
    RS_Vector aRight2 = data.endpoint - perpA;

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

    double distSame = 1e20, distCross = 1e20;
    if (intLL.valid && intRR.valid)
        distSame = intLL.distanceTo(sharedPoint) + intRR.distanceTo(sharedPoint);
    if (intLR.valid && intRL.valid)
        distCross = intLR.distanceTo(sharedPoint) + intRL.distanceTo(sharedPoint);

    bool useSame;
    if (fabs(distSame - distCross) > RS_TOLERANCE) {
        useSame = (distSame < distCross);
    } else {
        bool sharedIsMyEnd = (data.endpoint.distanceTo(sharedPoint) < RS_TOLERANCE);
        bool sharedIsNeighborEnd = (neighbor->getEndpoint().distanceTo(sharedPoint) < RS_TOLERANCE);
        RS_Vector awayMe = sharedIsMyEnd ? (data.startpoint - data.endpoint)
                                         : (data.endpoint - data.startpoint);
        RS_Vector awayNb = sharedIsNeighborEnd ? (neighbor->getStartpoint() - neighbor->getEndpoint())
                                               : (neighbor->getEndpoint() - neighbor->getStartpoint());

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
    update();
    updateNeighbors();  // update new neighbors after moving
}

void RS_Wall::rotate(const RS_Vector& center, const double& angle) {
      RS_DEBUG->print("silas - rotate");
    updateNeighbors();
    RS_Vector angleVector(angle);
    data.startpoint.rotate(center, angleVector);
    data.endpoint.rotate(center, angleVector);
    update();
    updateNeighbors();
}

void RS_Wall::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    updateNeighbors();
    data.startpoint.rotate(center, angleVector);
    data.endpoint.rotate(center, angleVector);
    update();
    updateNeighbors();
}

void RS_Wall::scale(const RS_Vector& center, const RS_Vector& factor) {
    updateNeighbors();
    data.startpoint.scale(center, factor);
    data.endpoint.scale(center, factor);
    data.thickness *= (fabs(factor.x) + fabs(factor.y)) / 2.0;
    update();
    updateNeighbors();
}

void RS_Wall::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    updateNeighbors();
    data.startpoint.mirror(axisPoint1, axisPoint2);
    data.endpoint.mirror(axisPoint1, axisPoint2);
    update();
    updateNeighbors();
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
        update();
        updateNeighbors();
    }
}

void RS_Wall::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    RS_DEBUG->print("silas - moveref");
    if (ref.distanceTo(data.startpoint) < 1.0e-4) {
        updateNeighbors();
        data.startpoint += offset;
        update();
        updateNeighbors();
    } else if (ref.distanceTo(data.endpoint) < 1.0e-4) {
        updateNeighbors();
        data.endpoint += offset;
        update();
        updateNeighbors();
    } else {
        // Check if the ref matches an opening grip — slide along wall
        RS_Vector wallDir = data.endpoint - data.startpoint;
        double wallLength = wallDir.magnitude();
        if (wallLength < RS_TOLERANCE) return;
        RS_Vector wallUnit = wallDir / wallLength;

        for (auto e : entities) {
            if (!e || e->isUndone()) continue;
            auto* opening = dynamic_cast<RS_WallOpening*>(e);
            if (opening) {
                RS_Vector gp = opening->getGripPoint();
                if (gp.valid && ref.distanceTo(gp) < 1.0e-4) {
                    double slideAmount = offset.x * wallUnit.x + offset.y * wallUnit.y;
                    double newPos = opening->getPositionAlongWall() + slideAmount;
                    double halfW = opening->getWidth() / 2.0;
                    newPos = std::max(halfW, std::min(wallLength - halfW, newPos));
                    opening->setPositionAlongWall(newPos);
                    update();
                    return;
                }
            }
        }
    }
}

std::ostream& operator << (std::ostream& os, const RS_Wall& w) {
    os << " Wall: " << w.data << "\n";
    return os;
}
