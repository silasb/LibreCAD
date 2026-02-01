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
#include "qg_dlgttext.h"

#include "rs_ttext.h"
#include "lc_ttfutils.h"
#include "rs_math.h"

#include <QFileInfo>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QTimer>

QG_DlgTText::QG_DlgTText(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

    previewScene = new QGraphicsScene(this);
    gvPreview->setScene(previewScene);
    gvPreview->setRenderHint(QPainter::Antialiasing);

    // Populate font combo box with system TTF fonts
    QStringList fontPaths = LC_TTFUtils::getSystemFontPaths();
    for (const QString& path : fontPaths) {
        QFileInfo fi(path);
        cbFont->addItem(fi.baseName(), path);
    }

    // Connect signals for live preview
    connect(teText, &QPlainTextEdit::textChanged, this, &QG_DlgTText::updatePreview);
    connect(cbFont, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QG_DlgTText::updatePreview);
    connect(leSpacing, &QLineEdit::textChanged, this, &QG_DlgTText::updatePreview);
}

void QG_DlgTText::languageChange()
{
    retranslateUi(this);
}

void QG_DlgTText::setTText(RS_TText& t) {
    ttext = &t;

    teText->setPlainText(ttext->getText());

    // Select current font in combo
    QString fontPath = ttext->getFontPath();
    if (!fontPath.isEmpty()) {
        int idx = cbFont->findData(fontPath);
        if (idx >= 0) {
            cbFont->setCurrentIndex(idx);
        }
    }

    QString s;
    s.setNum(ttext->getHeight());
    leHeight->setText(s);
    s.setNum(ttext->getAngle());
    leAngle->setText(s);
    s.setNum(ttext->getLetterSpacing());
    leSpacing->setText(s);

    // Deferred so the view has its final size for fitInView
    QTimer::singleShot(0, this, &QG_DlgTText::updatePreview);
}

void QG_DlgTText::updateTText() {
    if (!ttext) return;

    RS_TTextData d(
        teText->toPlainText(),
        cbFont->currentData().toString(),
        ttext->getInsertionPoint(),
        RS_Math::eval(leHeight->text()),
        RS_Math::eval(leAngle->text()),
        RS_Math::eval(leSpacing->text())
    );

    ttext->setData(d);
    ttext->update();
}

void QG_DlgTText::updatePreview() {
    previewScene->clear();

    QString text = teText->toPlainText();
    QString fontPath = cbFont->currentData().toString();
    if (text.isEmpty() || fontPath.isEmpty()) return;

    bool ok = false;
    double spacing = RS_Math::eval(leSpacing->text(), &ok);
    if (!ok || spacing < 0.01) spacing = 1.0;

    double cursorX = 0.0;
    double cursorY = 0.0;

    for (int i = 0; i < text.length(); i++) {
        uint32_t ch = text.at(i).unicode();
        if (ch == '\n') {
            cursorX = 0.0;
            cursorY += 1.5; // Y-down in screen coords, so add
            continue;
        }
        if (ch == ' ') {
            cursorX += 0.5 * spacing;
            continue;
        }

        LC_GlyphOutline glyph = LC_TTFUtils::extractGlyphOutlines(fontPath, ch);
        if (glyph.contours.empty()) {
            cursorX += glyph.advanceX * spacing;
            continue;
        }

        // Build a QPainterPath for this glyph using even-odd fill
        QPainterPath path;
        path.setFillRule(Qt::OddEvenFill);

        for (const auto& contour : glyph.contours) {
            if (contour.points.size() < 3) continue;

            // Note: flip Y for screen coordinates (Qt Y-down)
            path.moveTo(contour.points[0].x + cursorX,
                        -contour.points[0].y + cursorY);
            for (size_t j = 1; j < contour.points.size(); j++) {
                path.lineTo(contour.points[j].x + cursorX,
                            -contour.points[j].y + cursorY);
            }
            path.closeSubpath();
        }

        QGraphicsPathItem* item = previewScene->addPath(
            path, QPen(Qt::black, 0), QBrush(Qt::darkGray));
        Q_UNUSED(item);

        cursorX += glyph.advanceX * spacing;
    }

    // Fit the preview to the view
    QRectF bounds = previewScene->itemsBoundingRect();
    if (!bounds.isNull()) {
        bounds.adjust(-0.1, -0.1, 0.1, 0.1);
        gvPreview->fitInView(bounds, Qt::KeepAspectRatio);
    }
}
