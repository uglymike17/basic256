/** OpenSimplex2 noise for BASIC-256.
 **
 ** The algorithm is OpenSimplex2 (the "fast" 2D variant) by Kurt Spencer
 ** (KdotJPG), https://github.com/KdotJPG/OpenSimplex2, released under CC0 1.0
 ** Universal - a public domain dedication, so it may be used here without
 ** conflicting with this program's licence.  See opensimplex.cpp for what was
 ** carried over and what was changed.
 **/

#ifndef OPENSIMPLEX_H
#define OPENSIMPLEX_H

#include <cstdint>

namespace OpenSimplex2 {

	// Two dimensional noise.  Continuous everywhere, zero at every lattice
	// point, and bounded by -1..1 (it approaches those ends without reaching
	// them, so do not expect a full sweep from a short sample).
	double noise2(int64_t seed, double x, double y);

	// One dimensional noise.  OpenSimplex has no native one dimensional form -
	// this walks a line through the two dimensional field.  The line is rotated
	// off the lattice axes (the reference implementation's "ImproveX" domain
	// rotation), because sampling straight along an axis crosses the simplex
	// grid at a repeating angle and the regularity shows.
	double noise1(int64_t seed, double x);

}

#endif
