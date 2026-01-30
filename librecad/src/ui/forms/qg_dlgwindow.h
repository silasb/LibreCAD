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
#ifndef QG_DLGWINDOW_H
#define QG_DLGWINDOW_H

class RS_Window;

#include "ui_qg_dlgwindow.h"

class QG_DlgWindow : public QDialog, public Ui::QG_DlgWindow
{
    Q_OBJECT

public:
    QG_DlgWindow(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = {});

public slots:
    virtual void setWindow(RS_Window& w);
    virtual void updateWindow();

protected slots:
    virtual void languageChange();

private:
    RS_Window* window = nullptr;
};

#endif // QG_DLGWINDOW_H
