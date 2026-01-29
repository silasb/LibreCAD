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

#ifndef RS_WALL_H
#define RS_WALL_H

#include "rs_entitycontainer.h"

/**
 * Holds the data that defines a wall entity.
 */
struct RS_WallData {
    RS_WallData();
    RS_WallData(const RS_Vector& startpoint,
                const RS_Vector& endpoint,
                double thickness);

    /** Centerline start point */
    RS_Vector startpoint;
    /** Centerline end point */
    RS_Vector endpoint;
    /** Wall thickness */
    double thickness = 6.0;
};

std::ostream& operator << (std::ostream& os, const RS_WallData& wd);

/**
 * Class for AEC wall entities.
 *
 * A wall is a parametric entity that stores a centerline and thickness,
 * and generates offset lines and end caps as child entities.
 */
class RS_Wall : public RS_EntityContainer {
public:
    RS_Wall(RS_EntityContainer* parent, const RS_WallData& d);
    ~RS_Wall() override = default;

    RS_Entity* clone() const override;

    RS2::EntityType rtti() const override {
        return RS2::EntityWall;
    }

    RS_WallData getData() const {
        return data;
    }

    RS_Vector getStartpoint() const {
        return data.startpoint;
    }

    RS_Vector getEndpoint() const {
        return data.endpoint;
    }

    double getThickness() const {
        return data.thickness;
    }

    void setThickness(double t) {
        data.thickness = t;
    }

    RS_VectorSolutions getRefPoints() const override;

    void update() override;

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, const double& angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    void stretch(const RS_Vector& firstCorner,
                 const RS_Vector& secondCorner,
                 const RS_Vector& offset) override;
    void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;

    RS_Wall* findNeighborAt(const RS_Vector& point) const;
    void updateNeighbors();

    friend std::ostream& operator << (std::ostream& os, const RS_Wall& w);

protected:
    RS_WallData data;

private:
    void computeJoinPoints(const RS_Wall* neighbor,
                           const RS_Vector& sharedPoint,
                           RS_Vector& leftPt,
                           RS_Vector& rightPt) const;
};

#endif
