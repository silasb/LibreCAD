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

#include "lc_ttfutils.h"
#include "rs_debug.h"
#include "rs_settings.h"

#include <QDir>
#include <QDirIterator>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_GLYPH_H

static FT_Library ftLibrary = nullptr;
bool LC_TTFUtils::initialized = false;

// Outline decomposition context
struct DecomposeContext {
    std::vector<LC_GlyphContour>* contours;
    LC_GlyphContour* current;
    double factor;  // normalization factor
};

static int ftMoveTo(const FT_Vector* to, void* user) {
    auto* ctx = static_cast<DecomposeContext*>(user);
    // Start a new contour
    ctx->contours->emplace_back();
    ctx->current = &ctx->contours->back();
    ctx->current->points.push_back(
        RS_Vector(to->x * ctx->factor, to->y * ctx->factor));
    return 0;
}

static int ftLineTo(const FT_Vector* to, void* user) {
    auto* ctx = static_cast<DecomposeContext*>(user);
    if (!ctx->current) return 0;
    ctx->current->points.push_back(
        RS_Vector(to->x * ctx->factor, to->y * ctx->factor));
    return 0;
}

static int ftConicTo(const FT_Vector* control, const FT_Vector* to, void* user) {
    auto* ctx = static_cast<DecomposeContext*>(user);
    if (!ctx->current) return 0;

    // Flatten quadratic Bézier into line segments
    RS_Vector p0 = ctx->current->points.back();
    RS_Vector p1(control->x * ctx->factor, control->y * ctx->factor);
    RS_Vector p2(to->x * ctx->factor, to->y * ctx->factor);

    const int steps = 8;
    for (int i = 1; i <= steps; i++) {
        double t = (double)i / steps;
        double u = 1.0 - t;
        RS_Vector pt = p0 * (u * u) + p1 * (2.0 * u * t) + p2 * (t * t);
        ctx->current->points.push_back(pt);
    }
    return 0;
}

static int ftCubicTo(const FT_Vector* control1, const FT_Vector* control2,
                     const FT_Vector* to, void* user) {
    auto* ctx = static_cast<DecomposeContext*>(user);
    if (!ctx->current) return 0;

    // Flatten cubic Bézier into line segments
    RS_Vector p0 = ctx->current->points.back();
    RS_Vector p1(control1->x * ctx->factor, control1->y * ctx->factor);
    RS_Vector p2(control2->x * ctx->factor, control2->y * ctx->factor);
    RS_Vector p3(to->x * ctx->factor, to->y * ctx->factor);

    const int steps = 8;
    for (int i = 1; i <= steps; i++) {
        double t = (double)i / steps;
        double u = 1.0 - t;
        RS_Vector pt = p0 * (u * u * u)
                     + p1 * (3.0 * u * u * t)
                     + p2 * (3.0 * u * t * t)
                     + p3 * (t * t * t);
        ctx->current->points.push_back(pt);
    }
    return 0;
}

static const FT_Outline_Funcs ftOutlineFuncs = {
    (FT_Outline_MoveTo_Func)ftMoveTo,
    (FT_Outline_LineTo_Func)ftLineTo,
    (FT_Outline_ConicTo_Func)ftConicTo,
    (FT_Outline_CubicTo_Func)ftCubicTo,
    0, 0
};

// Compute signed area to determine winding direction
static double signedArea(const std::vector<RS_Vector>& pts) {
    double area = 0.0;
    size_t n = pts.size();
    for (size_t i = 0; i < n; i++) {
        size_t j = (i + 1) % n;
        area += pts[i].x * pts[j].y;
        area -= pts[j].x * pts[i].y;
    }
    return area * 0.5;
}

bool LC_TTFUtils::initFreeType() {
    if (initialized) return true;
    FT_Error error = FT_Init_FreeType(&ftLibrary);
    if (error) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "LC_TTFUtils: FT_Init_FreeType failed");
        return false;
    }
    initialized = true;
    return true;
}

void LC_TTFUtils::cleanupFreeType() {
    if (ftLibrary) {
        FT_Done_FreeType(ftLibrary);
        ftLibrary = nullptr;
    }
    initialized = false;
}

LC_GlyphOutline LC_TTFUtils::extractGlyphOutlines(const QString& fontPath,
                                                    uint32_t charCode) {
    LC_GlyphOutline result;

    if (!initFreeType()) return result;

    FT_Face face = nullptr;
    FT_Error error = FT_New_Face(ftLibrary, fontPath.toUtf8().constData(), 0, &face);
    if (error) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "LC_TTFUtils: cannot open font %s", fontPath.toUtf8().constData());
        return result;
    }

    // First, measure capital 'H' to get normalization factor
    FT_UInt hIndex = FT_Get_Char_Index(face, 'H');
    double capHeight = face->ascender;  // fallback
    if (hIndex != 0) {
        error = FT_Load_Glyph(face, hIndex, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE);
        if (!error) {
            FT_Glyph hGlyph;
            FT_Get_Glyph(face->glyph, &hGlyph);
            FT_BBox bbox;
            FT_Glyph_Get_CBox(hGlyph, FT_GLYPH_BBOX_UNSCALED, &bbox);
            if (bbox.yMax > 0) {
                capHeight = bbox.yMax;
            }
            FT_Done_Glyph(hGlyph);
        }
    }

    double factor = (capHeight > 0) ? 1.0 / capHeight : 1.0 / face->units_per_EM;

    // Load requested glyph
    FT_UInt glyphIndex = FT_Get_Char_Index(face, charCode);
    if (glyphIndex == 0) {
        FT_Done_Face(face);
        return result;
    }

    error = FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE);
    if (error) {
        FT_Done_Face(face);
        return result;
    }

    result.advanceX = face->glyph->advance.x * factor;

    if (face->glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
        FT_Done_Face(face);
        return result;
    }

    FT_Glyph glyph;
    FT_Get_Glyph(face->glyph, &glyph);
    FT_OutlineGlyph og = (FT_OutlineGlyph)glyph;

    DecomposeContext ctx;
    ctx.contours = &result.contours;
    ctx.current = nullptr;
    ctx.factor = factor;

    FT_Outline_Decompose(&og->outline, &ftOutlineFuncs, &ctx);

    // Determine winding direction for each contour
    for (auto& contour : result.contours) {
        if (contour.points.size() < 3) continue;
        double area = signedArea(contour.points);
        // In FreeType's coordinate system (y-up), positive area = CCW = outer contour
        contour.clockwise = (area < 0);
    }

    FT_Done_Glyph(glyph);
    FT_Done_Face(face);
    return result;
}

QStringList LC_TTFUtils::getSystemFontPaths() {
    QStringList paths;
    QStringList searchDirs;

#ifdef Q_OS_LINUX
    searchDirs << "/usr/share/fonts"
               << "/usr/local/share/fonts"
               << QDir::homePath() + "/.fonts"
               << QDir::homePath() + "/.local/share/fonts";
#elif defined(Q_OS_WIN)
    searchDirs << "C:/Windows/Fonts";
#elif defined(Q_OS_MAC)
    searchDirs << "/Library/Fonts"
               << "/System/Library/Fonts"
               << QDir::homePath() + "/Library/Fonts";
#endif

    // Add user-configured font directories from preferences (/Paths/Fonts)
    RS_SETTINGS->beginGroup("/Paths");
    QString userFonts = RS_SETTINGS->readEntry("/Fonts", "");
    RS_SETTINGS->endGroup();
    if (!userFonts.isEmpty()) {
        searchDirs << userFonts.split(";", Qt::SkipEmptyParts);
    }

    QStringList filters;
    filters << "*.ttf" << "*.otf" << "*.TTF" << "*.OTF";

    for (const QString& dir : searchDirs) {
        QDirIterator it(dir, filters, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            paths << it.next();
        }
    }

    paths.sort();
    return paths;
}
