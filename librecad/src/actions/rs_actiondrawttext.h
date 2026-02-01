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
#ifndef RS_ACTIONDRAWTTEXT_H
#define RS_ACTIONDRAWTTEXT_H

#include <memory>
#include "rs_previewactioninterface.h"

struct RS_TTextData;

class RS_ActionDrawTText : public RS_PreviewActionInterface {
    Q_OBJECT
public:
    enum Status { ShowDialog, SetPos };

    RS_ActionDrawTText(RS_EntityContainer& container,
                       RS_GraphicView& graphicView);
    ~RS_ActionDrawTText() override;

    void init(int status = 0) override;
    void trigger() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void coordinateEvent(RS_CoordinateEvent* e) override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

private:
    std::unique_ptr<RS_TTextData> data;
};

#endif
