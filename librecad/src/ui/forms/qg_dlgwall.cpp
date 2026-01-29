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
#include "qg_dlgwall.h"

#include "rs_wall.h"
#include "rs_graphic.h"
#include "rs_math.h"

QG_DlgWall::QG_DlgWall(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);
}

void QG_DlgWall::languageChange()
{
    retranslateUi(this);
}

void QG_DlgWall::setWall(RS_Wall& w) {
    wall = &w;

    RS_Graphic* graphic = wall->getGraphic();
    if (graphic) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = wall->getLayer(true);
    if (lay) {
        cbLayer->setLayer(*lay);
    }

    RS_Pen wallPen = wall->getPen(false);
    RS_Pen wallResolvedPen = wall->getPen(true);

    RS_Color originalColor = wallPen.getColor();
    RS_Color resolvedColor = wallResolvedPen.getColor();
    resolvedColor.applyFlags(originalColor);
    wallResolvedPen.setColor(resolvedColor);

    wPen->setPen(wallResolvedPen, lay, "Pen");

    QString s;
    s.setNum(wall->getStartpoint().x);
    leStartX->setText(s);
    s.setNum(wall->getStartpoint().y);
    leStartY->setText(s);
    s.setNum(wall->getEndpoint().x);
    leEndX->setText(s);
    s.setNum(wall->getEndpoint().y);
    leEndY->setText(s);
    s.setNum(wall->getThickness());
    leThickness->setText(s);
    lId->setText(QString("ID: %1").arg(wall->getId()));
}

void QG_DlgWall::updateWall() {
    RS_WallData wd(
        RS_Vector(RS_Math::eval(leStartX->text()),
                  RS_Math::eval(leStartY->text())),
        RS_Vector(RS_Math::eval(leEndX->text()),
                  RS_Math::eval(leEndY->text())),
        RS_Math::eval(leThickness->text())
    );

    wall->setData(wd);
    wall->setPen(wPen->getPen());
    wall->setLayer(cbLayer->currentText());
    wall->update();
}
