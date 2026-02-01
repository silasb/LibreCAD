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
#ifndef LC_TTFUTILS_H
#define LC_TTFUTILS_H

#include <QString>
#include <QStringList>
#include <vector>
#include "rs_vector.h"

/**
 * A single contour from a glyph outline.
 * Points form a closed polyline (line segments from FreeType decomposition).
 * clockwise indicates winding direction (true = outer boundary, false = hole).
 */
struct LC_GlyphContour {
    std::vector<RS_Vector> points;
    bool clockwise = true;
};

/**
 * Result of extracting a single glyph's outlines.
 */
struct LC_GlyphOutline {
    std::vector<LC_GlyphContour> contours;
    double advanceX = 0.0;  // horizontal advance width (normalized to height=1)
};

/**
 * Utility class wrapping FreeType for extracting glyph outlines
 * from TrueType/OpenType font files.
 */
class LC_TTFUtils {
public:
    static bool initFreeType();
    static void cleanupFreeType();

    /**
     * Extract glyph outlines for a character from a font file.
     * All coordinates are normalized so that the capital letter height = 1.0.
     * @param fontPath  Path to the TTF/OTF file
     * @param charCode  Unicode code point
     * @return Glyph outline data with contours and advance width
     */
    static LC_GlyphOutline extractGlyphOutlines(const QString& fontPath,
                                                  uint32_t charCode);

    /**
     * Get a list of system TTF/OTF font file paths.
     */
    static QStringList getSystemFontPaths();

private:
    static bool initialized;
};

#endif // LC_TTFUTILS_H
