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

#ifndef RS_WALLOPENING_H
#define RS_WALLOPENING_H

#include "rs_entitycontainer.h"

/**
 * Holds the data common to all wall opening entities (doors, windows).
 */
struct RS_WallOpeningData {
    RS_WallOpeningData();
    RS_WallOpeningData(double positionAlongWall, double width);

    /** Distance from wall startpoint along centerline */
    double positionAlongWall = 0.0;
    /** Opening width */
    double width = 36.0;
};

std::ostream& operator << (std::ostream& os, const RS_WallOpeningData& od);

/**
 * Abstract base class for wall opening entities (doors, windows).
 *
 * A wall opening is a parametric entity that lives as a child of an RS_Wall.
 * It stores position and width data, and provides a grip point for sliding
 * along the wall centerline. Subclasses implement update() to generate
 * type-specific geometry.
 */
class RS_WallOpening : public RS_EntityContainer {
public:
    RS_WallOpening(RS_EntityContainer* parent, const RS_WallOpeningData& d);
    ~RS_WallOpening() override = default;

    RS_WallOpeningData getOpeningData() const {
        return openingData;
    }

    void setOpeningData(const RS_WallOpeningData& d) {
        openingData = d;
    }

    double getPositionAlongWall() const {
        return openingData.positionAlongWall;
    }

    void setPositionAlongWall(double p) {
        openingData.positionAlongWall = p;
    }

    double getWidth() const {
        return openingData.width;
    }

    void setWidth(double w) {
        openingData.width = w;
    }

    /** Returns the grip point on the wall centerline at the opening position. */
    RS_Vector getGripPoint() const;

protected:
    RS_WallOpeningData openingData;
};

#endif
