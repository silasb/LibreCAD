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

#include <cmath>
#include <iostream>
#include <QPainterPath>
#include "rs_ttext.h"
#include "lc_ttfutils.h"
#include "rs_line.h"
#include "rs_hatch.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_painter.h"

RS_TTextData::RS_TTextData(const QString& text,
                           const QString& fontPath,
                           const RS_Vector& insertionPoint,
                           double height,
                           double angle,
                           double letterSpacing)
    : text(text)
    , fontPath(fontPath)
    , insertionPoint(insertionPoint)
    , height(height)
    , angle(angle)
    , letterSpacing(letterSpacing)
{
}

std::ostream& operator << (std::ostream& os, const RS_TTextData& d) {
    os << "(\"" << d.text.toStdString() << "\","
       << " font=" << d.fontPath.toStdString()
       << " height=" << d.height
       << " angle=" << d.angle
       << " spacing=" << d.letterSpacing << ")";
    return os;
}

RS_TText::RS_TText(RS_EntityContainer* parent, const RS_TTextData& d)
    : RS_EntityContainer(parent)
    , data(d)
{
    update();
}

RS_Entity* RS_TText::clone() const {
    RS_TText* t = new RS_TText(*this);
    t->setOwner(isOwner());
    t->initId();
    t->detach();
    return t;
}

void RS_TText::update() {
    clear();

    if (data.text.isEmpty() || data.fontPath.isEmpty() || data.height < 1e-6) {
        calculateBorders();
        return;
    }

    double cursorX = 0.0;
    double cursorY = 0.0;
    double angleRad = data.angle * M_PI / 180.0;
    double cosA = cos(angleRad);
    double sinA = sin(angleRad);

    for (int i = 0; i < data.text.length(); i++) {
        uint32_t ch = data.text.at(i).unicode();
        if (ch == '\n') {
            cursorX = 0.0;
            cursorY -= 1.5; // line height in normalized units (cap height = 1.0)
            continue;
        }
        if (ch == ' ') {
            // Approximate space width
            cursorX += 0.5 * data.letterSpacing;
            continue;
        }

        LC_GlyphOutline glyph = LC_TTFUtils::extractGlyphOutlines(data.fontPath, ch);

        if (glyph.contours.empty()) {
            cursorX += glyph.advanceX * data.letterSpacing;
            continue;
        }

        // Build a hatch entity from the glyph contours.
        // Each contour becomes a loop (RS_EntityContainer) of RS_Line segments.
        RS_HatchData hatchData(true, 1.0, 0.0, "SOLID");
        RS_Hatch* hatch = new RS_Hatch(this, hatchData);
        hatch->setPen(RS_Pen(RS2::FlagInvalid));

        for (const auto& contour : glyph.contours) {
            if (contour.points.size() < 3) continue;

            RS_EntityContainer* loop = new RS_EntityContainer(hatch);

            for (size_t j = 0; j < contour.points.size(); j++) {
                size_t k = (j + 1) % contour.points.size();

                // Transform point: scale by height, offset by cursor, rotate, translate
                auto transformPt = [&](const RS_Vector& p) -> RS_Vector {
                    double px = (p.x + cursorX) * data.height;
                    double py = (p.y + cursorY) * data.height;
                    double rx = px * cosA - py * sinA + data.insertionPoint.x;
                    double ry = px * sinA + py * cosA + data.insertionPoint.y;
                    return RS_Vector(rx, ry);
                };

                RS_Vector p1 = transformPt(contour.points[j]);
                RS_Vector p2 = transformPt(contour.points[k]);

                RS_Line* line = new RS_Line(loop, RS_LineData(p1, p2));
                loop->addEntity(line);
            }

            hatch->addEntity(loop);
        }

        hatch->update();
        addEntity(hatch);

        cursorX += glyph.advanceX * data.letterSpacing;
    }

    calculateBorders();
}

RS_VectorSolutions RS_TText::getRefPoints() const {
    return RS_VectorSolutions({data.insertionPoint});
}

RS_Vector RS_TText::getNearestEndpoint(const RS_Vector& coord,
                                        double* dist) const {
    double d = data.insertionPoint.distanceTo(coord);
    if (dist) *dist = d;
    return data.insertionPoint;
}

void RS_TText::move(const RS_Vector& offset) {
    data.insertionPoint.move(offset);
    RS_EntityContainer::move(offset);
}

void RS_TText::rotate(const RS_Vector& center, const double& angle) {
    data.insertionPoint.rotate(center, angle);
    data.angle += angle * 180.0 / M_PI;
    update();
}

void RS_TText::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    data.insertionPoint.rotate(center, angleVector);
    data.angle += angleVector.angle() * 180.0 / M_PI;
    update();
}

void RS_TText::scale(const RS_Vector& center, const RS_Vector& factor) {
    data.insertionPoint.scale(center, factor);
    data.height *= factor.x;
    update();
}

void RS_TText::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    data.insertionPoint.mirror(axisPoint1, axisPoint2);
    RS_Vector dir = axisPoint2 - axisPoint1;
    double mirrorAngle = dir.angle();
    data.angle = 2.0 * mirrorAngle * 180.0 / M_PI - data.angle;
    update();
}

void RS_TText::draw(RS_Painter* painter, RS_GraphicView* view,
                    double& /*patternOffset*/) {
    if (!painter || !view) return;

    // Custom draw: iterate child hatches and build QPainterPath per glyph
    // using floating-point coordinates and explicit OddEvenFill.
    //
    // RS_Hatch::draw() uses integer QPolygon which causes rounding errors:
    // the first/last points of a contour can round to different pixels,
    // preventing the polygon from being detected as closed. This merges
    // separate contours into one polygon, destroying the even-odd fill
    // needed for letter holes (g, B, P, O, etc.).

    const QBrush oldBrush = painter->brush();
    const RS_Pen pen = painter->getPen();

    for (RS_Entity* child = firstEntity(RS2::ResolveNone);
         child; child = nextEntity(RS2::ResolveNone)) {

        if (child->rtti() != RS2::EntityHatch) continue;
        RS_Hatch* hatch = static_cast<RS_Hatch*>(child);

        QPainterPath path;
        path.setFillRule(Qt::OddEvenFill);

        // Each direct child of the hatch is a loop (RS_EntityContainer)
        for (RS_Entity* loopEntity = hatch->firstEntity(RS2::ResolveNone);
             loopEntity; loopEntity = hatch->nextEntity(RS2::ResolveNone)) {

            if (loopEntity->rtti() != RS2::EntityContainer) continue;
            RS_EntityContainer* loop = static_cast<RS_EntityContainer*>(loopEntity);

            bool first = true;
            for (RS_Entity* edge = loop->firstEntity(RS2::ResolveNone);
                 edge; edge = loop->nextEntity(RS2::ResolveNone)) {

                if (edge->rtti() != RS2::EntityLine) continue;

                double gx1 = view->toGuiX(edge->getStartpoint().x);
                double gy1 = view->toGuiY(edge->getStartpoint().y);

                if (first) {
                    path.moveTo(gx1, gy1);
                    first = false;
                }

                double gx2 = view->toGuiX(edge->getEndpoint().x);
                double gy2 = view->toGuiY(edge->getEndpoint().y);
                path.lineTo(gx2, gy2);
            }

            path.closeSubpath();
        }

        painter->setBrush(pen.getColor());
        painter->disablePen();
        painter->drawPath(path);
    }

    painter->setBrush(oldBrush);
    painter->setPen(pen);
}

std::ostream& operator << (std::ostream& os, const RS_TText& t) {
    os << " TText: " << t.data << std::endl;
    return os;
}
