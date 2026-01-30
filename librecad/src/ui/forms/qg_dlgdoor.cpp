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
#include "qg_dlgdoor.h"

#include "rs_door.h"
#include "rs_wall.h"
#include "rs_graphic.h"
#include "rs_math.h"

QG_DlgDoor::QG_DlgDoor(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);
}

void QG_DlgDoor::languageChange()
{
    retranslateUi(this);
}

void QG_DlgDoor::setDoor(RS_Door& d) {
    door = &d;

    RS_Graphic* graphic = door->getGraphic();
    if (graphic) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = door->getLayer(true);
    if (lay) {
        cbLayer->setLayer(*lay);
    }

    RS_Pen doorPen = door->getPen(false);
    RS_Pen doorResolvedPen = door->getPen(true);

    RS_Color originalColor = doorPen.getColor();
    RS_Color resolvedColor = doorResolvedPen.getColor();
    resolvedColor.applyFlags(originalColor);
    doorResolvedPen.setColor(resolvedColor);

    wPen->setPen(doorResolvedPen, lay, "Pen");

    QString s;
    s.setNum(door->getWidth());
    leWidth->setText(s);
    s.setNum(door->getPositionAlongWall());
    lePosition->setText(s);

    cbSwingLeft->setChecked(door->isSwingLeft());
    cbHingeReversed->setChecked(door->isHingeReversed());

    lId->setText(QString("ID: %1").arg(door->getId()));
}

void QG_DlgDoor::updateDoor() {
    RS_WallOpeningData od(
        RS_Math::eval(lePosition->text()),
        RS_Math::eval(leWidth->text())
    );
    RS_DoorData dd(
        M_PI_2,
        cbSwingLeft->isChecked(),
        cbHingeReversed->isChecked()
    );

    door->setOpeningData(od);
    door->setData(dd);
    door->setPen(wPen->getPen());
    door->setLayer(cbLayer->currentText());

    // Update the parent wall to regenerate geometry with new door data
    RS_EntityContainer* parent = door->getParent();
    if (parent && parent->rtti() == RS2::EntityWall) {
        static_cast<RS_Wall*>(parent)->update();
    } else {
        door->update();
    }
}
