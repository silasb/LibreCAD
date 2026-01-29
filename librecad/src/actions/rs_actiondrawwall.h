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

#ifndef RS_ACTIONDRAWWALL_H
#define RS_ACTIONDRAWWALL_H

#include "rs_previewactioninterface.h"
#include "rs_vector.h"

struct RS_WallData;

/**
 * Action for drawing AEC wall entities.
 */
class RS_ActionDrawWall : public RS_PreviewActionInterface {
    Q_OBJECT
public:
    enum Status {
        SetStartpoint,
        SetEndpoint,
        SetThickness
    };

    RS_ActionDrawWall(RS_EntityContainer& container,
                      RS_GraphicView& graphicView);
    ~RS_ActionDrawWall() override;

    void init(int status = 0) override;
    void trigger() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void coordinateEvent(RS_CoordinateEvent* e) override;
    void commandEvent(RS_CommandEvent* e) override;
    QStringList getAvailableCommands() override;

    void showOptions() override;
    void hideOptions() override;
    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

    double getThickness() const { return thickness; }
    void setThickness(double t) { thickness = t; }

private:
    void ensureWallLayer();
    void close();

    RS_Vector startpoint;
    RS_Vector chainStartpoint{false};
    int wallCount = 0;
    double thickness = 3.5;
    Status lastStatus = SetStartpoint;
};

#endif
