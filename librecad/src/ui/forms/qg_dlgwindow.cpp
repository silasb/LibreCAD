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
#include "qg_dlgwindow.h"

#include "rs_window.h"
#include "rs_wall.h"
#include "rs_graphic.h"
#include "rs_math.h"

QG_DlgWindow::QG_DlgWindow(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);
}

void QG_DlgWindow::languageChange()
{
    retranslateUi(this);
}

void QG_DlgWindow::setWindow(RS_Window& w) {
    window = &w;

    RS_Graphic* graphic = window->getGraphic();
    if (graphic) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = window->getLayer(true);
    if (lay) {
        cbLayer->setLayer(*lay);
    }

    RS_Pen windowPen = window->getPen(false);
    RS_Pen windowResolvedPen = window->getPen(true);

    RS_Color originalColor = windowPen.getColor();
    RS_Color resolvedColor = windowResolvedPen.getColor();
    resolvedColor.applyFlags(originalColor);
    windowResolvedPen.setColor(resolvedColor);

    wPen->setPen(windowResolvedPen, lay, "Pen");

    QString s;
    s.setNum(window->getWidth());
    leWidth->setText(s);
    s.setNum(window->getPositionAlongWall());
    lePosition->setText(s);

    lId->setText(QString("ID: %1").arg(window->getId()));
}

void QG_DlgWindow::updateWindow() {
    RS_WindowData wd(
        RS_Math::eval(lePosition->text()),
        RS_Math::eval(leWidth->text())
    );

    window->setData(wd);
    window->setPen(wPen->getPen());
    window->setLayer(cbLayer->currentText());

    // Update the parent wall to regenerate geometry with new window data
    RS_EntityContainer* parent = window->getParent();
    if (parent && parent->rtti() == RS2::EntityWall) {
        static_cast<RS_Wall*>(parent)->update();
    } else {
        window->update();
    }
}
