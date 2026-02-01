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
#ifndef RS_TTEXT_H
#define RS_TTEXT_H

#include "rs_entitycontainer.h"

struct RS_TTextData {
    RS_TTextData() = default;
    RS_TTextData(const QString& text,
                 const QString& fontPath,
                 const RS_Vector& insertionPoint,
                 double height,
                 double angle,
                 double letterSpacing = 1.0);

    QString text;
    QString fontPath;
    RS_Vector insertionPoint;
    double height = 10.0;
    double angle = 0.0;
    double letterSpacing = 1.0;
};

std::ostream& operator << (std::ostream& os, const RS_TTextData& d);

/**
 * TrueType Text entity - renders TTF/OTF fonts as spline outlines
 * with solid hatch fill.
 */
class RS_TText : public RS_EntityContainer {
public:
    RS_TText(RS_EntityContainer* parent, const RS_TTextData& d);

    RS_Entity* clone() const override;

    RS2::EntityType rtti() const override { return RS2::EntityTText; }

    RS_TTextData getData() const { return data; }
    void setData(const RS_TTextData& d) { data = d; }

    QString getText() const { return data.text; }
    QString getFontPath() const { return data.fontPath; }
    RS_Vector getInsertionPoint() const { return data.insertionPoint; }
    double getHeight() const { return data.height; }
    double getAngle() const { return data.angle; }
    double getLetterSpacing() const { return data.letterSpacing; }

    void update() override;

    RS_VectorSolutions getRefPoints() const override;
    RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                 double* dist = nullptr) const override;

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, const double& angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;

    void draw(RS_Painter* painter, RS_GraphicView* view,
              double& patternOffset) override;

    friend std::ostream& operator << (std::ostream& os, const RS_TText& t);

protected:
    RS_TTextData data;
};

#endif // RS_TTEXT_H
