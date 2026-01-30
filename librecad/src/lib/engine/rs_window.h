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

#include "rs_wallopening.h"

/**
 * Class for AEC window entities.
 *
 * A window is a parametric entity that lives as a child of an RS_Wall.
 * It generates two parallel lines across the wall opening as child geometry.
 * The wall's update() method uses window positions to create gaps
 * in the wall offset lines.
 */
class RS_Window : public RS_WallOpening {
public:
    RS_Window(RS_EntityContainer* parent, const RS_WallOpeningData& d);
    ~RS_Window() override = default;

    RS_Entity* clone() const override;

    RS2::EntityType rtti() const override {
        return RS2::EntityWindow;
    }

    void update() override;

    friend std::ostream& operator << (std::ostream& os, const RS_Window& w);
};

#endif
