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

#ifndef RS_ACTIONDRAWDOOR_H
#define RS_ACTIONDRAWDOOR_H

#include "rs_previewactioninterface.h"
#include "rs_vector.h"

class RS_Wall;

/**
 * Action for drawing AEC door entities on walls.
 */
class RS_ActionDrawDoor : public RS_PreviewActionInterface {
    Q_OBJECT
public:
    enum Status {
        SelectWall,
        SetPosition
    };

    RS_ActionDrawDoor(RS_EntityContainer& container,
                      RS_GraphicView& graphicView);
    ~RS_ActionDrawDoor() override;

    void init(int status = 0) override;
    void trigger() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void commandEvent(RS_CommandEvent* e) override;
    QStringList getAvailableCommands() override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

    double getWidth() const { return doorWidth; }
    void setWidth(double w) { doorWidth = w; }

    bool isSwingLeft() const { return swingLeft; }
    void setSwingLeft(bool left) { swingLeft = left; }

private:
    void ensureDoorLayer();
    RS_Wall* findNearestWall(const RS_Vector& coord);
    double projectOnWall(RS_Wall* wall, const RS_Vector& coord);

    RS_Wall* selectedWall = nullptr;
    double doorWidth = 36.0;
    bool swingLeft = true;
    bool hingeReversed = false;
};

#endif
