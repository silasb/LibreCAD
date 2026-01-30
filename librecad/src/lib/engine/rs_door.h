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

#ifndef RS_DOOR_H
#define RS_DOOR_H

#include <cmath>
#include "rs_entitycontainer.h"

/**
 * Holds the data that defines a door entity.
 */
struct RS_DoorData {
    RS_DoorData();
    RS_DoorData(double positionAlongWall,
                double width,
                double swingAngle,
                bool swingLeft,
                bool hingeReversed = false);

    /** Distance from wall startpoint along centerline */
    double positionAlongWall = 0.0;
    /** Opening width */
    double width = 36.0;
    /** Swing angle in radians (default 90 degrees) */
    double swingAngle = M_PI_2;
    /** Which side of the wall the door swings to */
    bool swingLeft = true;
    /** When true, hinge is on the opposite end of the opening */
    bool hingeReversed = false;
};

std::ostream& operator << (std::ostream& os, const RS_DoorData& dd);

/**
 * Class for AEC door entities.
 *
 * A door is a parametric entity that lives as a child of an RS_Wall.
 * It generates a leaf line and swing arc as child geometry.
 * The wall's update() method uses door positions to create gaps
 * in the wall offset lines.
 */
class RS_Door : public RS_EntityContainer {
public:
    RS_Door(RS_EntityContainer* parent, const RS_DoorData& d);
    ~RS_Door() override = default;

    RS_Entity* clone() const override;

    RS2::EntityType rtti() const override {
        return RS2::EntityDoor;
    }

    RS_DoorData getData() const {
        return data;
    }

    void setData(const RS_DoorData& d) {
        data = d;
    }

    double getPositionAlongWall() const {
        return data.positionAlongWall;
    }

    void setPositionAlongWall(double p) {
        data.positionAlongWall = p;
    }

    double getWidth() const {
        return data.width;
    }

    void setWidth(double w) {
        data.width = w;
    }

    double getSwingAngle() const {
        return data.swingAngle;
    }

    bool isSwingLeft() const {
        return data.swingLeft;
    }

    void setSwingLeft(bool left) {
        data.swingLeft = left;
    }

    bool isHingeReversed() const {
        return data.hingeReversed;
    }

    void setHingeReversed(bool rev) {
        data.hingeReversed = rev;
    }

    void update() override;

    /** Returns the grip point on the wall centerline at the door position. */
    RS_Vector getGripPoint() const;

    friend std::ostream& operator << (std::ostream& os, const RS_Door& d);

protected:
    RS_DoorData data;
};

#endif
