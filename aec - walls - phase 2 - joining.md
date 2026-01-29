# Phase 2: Automatic Wall L-Join Cleanup

## Overview

When two walls share a common endpoint (within tolerance) on the same layer, their offset lines should automatically extend/trim to form a clean L-join corner instead of overlapping with separate end caps.

## Current Implementation Status

The L-join feature is partially implemented. The core infrastructure is in place and works for certain drawing directions, but the side-matching (pairing) logic has an unresolved bug that causes it to fail depending on the drawing direction.

### What Works
- Drawing walls heading north then east produces a clean L-join
- Neighbor detection (`findNeighborAt`) correctly finds adjacent walls
- `update()` correctly generates joined geometry when corner points are computed correctly
- `updateNeighbors()` triggers re-computation on transform operations (move, rotate, scale, mirror, stretch, moveRef)
- End caps are correctly suppressed at joined endpoints
- The wall added to the container before `update()` is called so neighbor lookup works
- Neighbor walls are updated in `trigger()` after a new wall is added

### What Does NOT Work (Known Bug)
- **Drawing south then east (or west) produces incorrect joins.** The side-matching logic in `computeJoinPoints()` fails for certain winding/drawing directions.
- The root cause is the **tiebreaker in the pairing selection**. The algorithm must choose between two possible pairings of intersection points: `{intLL, intRR}` ("same-side") or `{intLR, intRL}` ("cross-side"). For non-90-degree walls, the correct pairing is the one with shorter total distance to the shared point. For exactly 90-degree walls (equal thickness), both pairings are equidistant, requiring a tiebreaker.
- Multiple tiebreaker approaches were tried and all failed to be fully direction-invariant:
  1. **Forward/backward along wall direction** — worked for one wall but gave the opposite pairing for the neighbor wall, causing the two walls to disagree on corner points.
  2. **Entity ID ordering** — worked for one drawing direction but not the reverse.
  3. **Angle-sorted cross product** — sorting the two away-from-corner vectors by angle and computing the cross product. Still direction-dependent.
- **The fundamental challenge**: both walls must independently compute the SAME two corner points. Any criterion that depends on "this wall" vs "neighbor wall" perspective will give flipped results unless it's truly symmetric. Distance-based selection IS symmetric and works for non-90-degree walls. The remaining problem is a purely symmetric tiebreaker for 90-degree equal-thickness walls.

## Algorithm

### Detection
During `RS_Wall::update()`, after generating the basic offset geometry, scan sibling walls (same parent container, same layer, EntityWall type) for shared endpoints:
- For each of this wall's endpoints (start, end), check if any other wall has a start or end point within `RS_TOLERANCE` distance.
- If found, compute the L-join geometry for that corner.

### L-Join Geometry
When two walls share a common endpoint:

1. Compute the 4 offset lines (2 per wall) as infinite lines
2. Compute all 4 intersection points: intLL (left-left), intLR (left-right), intRL (right-left), intRR (right-right)
3. Select the correct pairing of 2 corner points from the 4 candidates
4. Assign the selected points to leftPt and rightPt for this wall
5. Suppress the end cap at the joined endpoint

### Pairing Selection (the hard part)
The 4 intersections form two possible pairings:
- **"Same-side"**: `{intLL, intRR}` — this wall's left meets neighbor's left, right meets right
- **"Cross-side"**: `{intLR, intRL}` — this wall's left meets neighbor's right, right meets left

**For non-90-degree walls**: Pick the pairing whose total distance to the shared point is smaller. This is symmetric — both walls compute the same geometric distances and select the same pair.

**For 90-degree equal-thickness walls**: Both pairings are equidistant. A tiebreaker is needed. **This is the unsolved problem.** The tiebreaker must be:
1. Deterministic
2. Give the same result regardless of which wall calls `computeJoinPoints`
3. Give the same result regardless of CW vs CCW drawing order

### Implementation in update()
1. Compute the raw offset points (start_left, start_right, end_left, end_right)
2. For each endpoint, check for a neighbor wall via `findNeighborAt()`
3. If neighbor found at start: replace start_left/start_right with intersection points, suppress cap1
4. If neighbor found at end: replace end_left/end_right with intersection points, suppress cap2
5. Generate child lines from the (possibly adjusted) corner points

### Neighbor lookup
```
RS_Wall* findNeighborAt(const RS_Vector& point) const
```
- Iterate parent container's entities
- Filter: same layer, EntityWall type, not self, not undone
- Check if any wall's start or endpoint is within `RS_TOLERANCE` of the given point
- Return the first match (or nullptr)

O(n) per wall per update, scoped to same-layer walls only.

### Neighbor updates on transforms
`updateNeighbors()` is called **before** and **after** each transform method (move, rotate, scale, mirror, stretch, moveRef):
- Before: so former neighbors restore their end caps when this wall moves away
- After: so new neighbors pick up the join at the new position

### Preventing infinite recursion
- `update()` does NOT trigger neighbor updates — it only adjusts its own geometry
- `updateNeighbors()` calls `update()` on neighbors, but those updates don't cascade further
- Neighbor updates from `trigger()` in the draw action are explicit and one-level-deep

## Files Modified

### 1. `librecad/src/lib/engine/rs_wall.h`
- Added public method: `RS_Wall* findNeighborAt(const RS_Vector& point) const`
- Added public method: `void updateNeighbors()`
- Added private method: `void computeJoinPoints(const RS_Wall* neighbor, const RS_Vector& sharedPoint, RS_Vector& leftPt, RS_Vector& rightPt) const`

### 2. `librecad/src/lib/engine/rs_wall.cpp`
- Rewrote `update()` to compute joined geometry
- Implemented `findNeighborAt()` — iterates parent container, filters by layer+type
- Implemented `computeJoinPoints()` — computes all 4 line-line intersections, selects correct pairing
- Implemented `updateNeighbors()` — scans for all same-layer walls sharing an endpoint and calls their `update()`
- All transform methods (move, rotate, scale, mirror, stretch, moveRef) call `updateNeighbors()` before and after

### 3. `librecad/src/actions/rs_actiondrawwall.cpp`
- In `trigger()`: wall is added to the container BEFORE `update()` is called (so neighbor lookup works)
- After adding and updating the new wall, neighbor walls at start and end points are found and their `update()` is called

## Key Details

### Line-line intersection
Computed directly as infinite line intersection:
```
Given lines (p1,p2) and (p3,p4):
  d = (p1.x-p2.x)*(p3.y-p4.y) - (p1.y-p2.y)*(p3.x-p4.x)
  t = ((p1.x-p3.x)*(p3.y-p4.y) - (p1.y-p3.y)*(p3.x-p4.x)) / d
  intersection = p1 + t*(p2-p1)
```

### Perpendicular / offset line computation
- `perpA = RS_Vector::polar(halfThick, angle + M_PI_2)` — +90 degrees from wall direction
- Left offset line: `startpoint + perp` to `endpoint + perp`
- Right offset line: `startpoint - perp` to `endpoint - perp`

## Remaining Work

### P0: Fix the pairing tiebreaker for 90-degree walls
The auto-join only works for certain drawing directions (e.g., north then east). Drawing south then east/west fails. The tiebreaker in `computeJoinPoints()` needs to be truly direction-invariant. Possible approaches to investigate:
- Use the **angle bisector** of the two away-from-corner directions to determine interior vs exterior
- Use a **winding number** or **signed area** check on the combined outline to detect the crossed/bowtie case and reject it
- Compute **both** pairings, generate the outline for each, and pick the one with positive signed area (non-self-intersecting)
- Consider whether the problem is actually in the assignment of leftPt/rightPt AFTER selecting the correct two geometric points (both walls agree on the points but assign them to left/right differently)

### P1: Verify transform behavior
- Verify move/rotate/scale/mirror/stretch with joined walls
- Verify undo/redo restores joins correctly
- Verify delete of one wall restores neighbor's end cap

### P2: Three-wall chain
- Test drawing three walls in an L-shape chain (two L-joins)
- Verify both corners clean up independently

### P3: T-intersection support (future)
- Current implementation only handles L-joins (two walls sharing an endpoint)
- T-intersections (wall endpoint meeting another wall's midpoint) are not supported
