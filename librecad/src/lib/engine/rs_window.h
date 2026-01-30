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

#ifndef RS_WINDOW_H
#define RS_WINDOW_H

#include "rs_entitycontainer.h"

/**
 * Holds the data that defines a window entity.
 */
struct RS_WindowData {
    RS_WindowData();
    RS_WindowData(double positionAlongWall,
                  double width);

    /** Distance from wall startpoint along centerline */
    double positionAlongWall = 0.0;
    /** Opening width */
    double width = 36.0;
};

std::ostream& operator << (std::ostream& os, const RS_WindowData& wd);

/**
 * Class for AEC window entities.
 *
 * A window is a parametric entity that lives as a child of an RS_Wall.
 * It generates two parallel lines across the wall opening as child geometry.
 * The wall's update() method uses window positions to create gaps
 * in the wall offset lines.
 */
class RS_Window : public RS_EntityContainer {
public:
    RS_Window(RS_EntityContainer* parent, const RS_WindowData& d);
    ~RS_Window() override = default;

    RS_Entity* clone() const override;

    RS2::EntityType rtti() const override {
        return RS2::EntityWindow;
    }

    RS_WindowData getData() const {
        return data;
    }

    void setData(const RS_WindowData& d) {
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

    void update() override;

    /** Returns the grip point on the wall centerline at the window position. */
    RS_Vector getGripPoint() const;

    friend std::ostream& operator << (std::ostream& os, const RS_Window& w);

protected:
    RS_WindowData data;
};

#endif
