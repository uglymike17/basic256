/** OpenSimplex2 noise for BASIC-256.
 **
 ** Ported from the reference Java implementation OpenSimplex2.java by Kurt
 ** Spencer (KdotJPG), https://github.com/KdotJPG/OpenSimplex2, which is
 ** released under CC0 1.0 Universal.  CC0 is a public domain dedication, so
 ** the code may be carried into this GPL program unchanged in substance.
 **
 ** Only the two dimensional part is carried over - that is all the BASIC-256
 ** NOISE function offers.  The constants, the gradient table, the lattice
 ** traversal and the hash are the reference's; what changed is the language
 ** (Java to C++), the seed type (a long long rather than Java's long, which is
 ** the same 64 bits), and the multiplications, which are done through unsigned
 ** arithmetic here because signed overflow is undefined behaviour in C++ while
 ** Java defines it to wrap.
 **/

#include "opensimplex.h"

namespace OpenSimplex2 {

	static const int64_t PRIME_X = (int64_t)0x5205402B9270C86FLL;
	static const int64_t PRIME_Y = (int64_t)0x598CD327003817B5LL;
	static const int64_t HASH_MULTIPLIER = (int64_t)0x53A3F72DEEC546F5LL;

	static const double ROOT2OVER2 = 0.7071067811865476;
	static const double SKEW_2D = 0.366025403784439;
	static const double UNSKEW_2D = -0.21132486540518713;

	static const float RSQUARED_2D = 0.5f;
	static const double NORMALIZER_2D = 0.01001634121365712;

	static const int N_GRADS_2D_EXPONENT = 7;
	static const int N_GRADS_2D = 1 << N_GRADS_2D_EXPONENT;		// 128

	// The 24 gradient directions of the reference table, 15 degrees apart.  The
	// reference divides each by NORMALIZER_2D and then cycles them into an
	// array of N_GRADS_2D pairs; 24 does not divide 128, and the wrap is on the
	// source index, so the table is not simply repeated a whole number of times.
	static const float GRAD2[] = {
		 0.38268343236509f,   0.923879532511287f,
		 0.923879532511287f,  0.38268343236509f,
		 0.923879532511287f, -0.38268343236509f,
		 0.38268343236509f,  -0.923879532511287f,
		-0.38268343236509f,  -0.923879532511287f,
		-0.923879532511287f, -0.38268343236509f,
		-0.923879532511287f,  0.38268343236509f,
		-0.38268343236509f,   0.923879532511287f,
		 0.130526192220052f,  0.99144486137381f,
		 0.608761429008721f,  0.793353340291235f,
		 0.793353340291235f,  0.608761429008721f,
		 0.99144486137381f,   0.130526192220051f,
		 0.99144486137381f,  -0.130526192220051f,
		 0.793353340291235f, -0.60876142900872f,
		 0.608761429008721f, -0.793353340291235f,
		 0.130526192220052f, -0.99144486137381f,
		-0.130526192220052f, -0.99144486137381f,
		-0.608761429008721f, -0.793353340291235f,
		-0.793353340291235f, -0.608761429008721f,
		-0.99144486137381f,  -0.130526192220052f,
		-0.99144486137381f,   0.130526192220051f,
		-0.793353340291235f,  0.608761429008721f,
		-0.608761429008721f,  0.793353340291235f,
		-0.130526192220052f,  0.99144486137381f,
	};

	static const int GRAD2_LEN = (int)(sizeof(GRAD2) / sizeof(GRAD2[0]));

	// Built once, on first use.  A function local static is initialized exactly
	// once and the C++11 rules make that thread safe, which matters only
	// because the interpreter runs on its own thread.
	struct GradientTable {
		float g[N_GRADS_2D * 2];
		GradientTable() {
			for (int i = 0, j = 0; i < N_GRADS_2D * 2; i++, j++) {
				if (j == GRAD2_LEN) j = 0;
				g[i] = (float)(GRAD2[j] / NORMALIZER_2D);
			}
		}
	};

	static const GradientTable& gradients() {
		static const GradientTable table;
		return table;
	}

	// Java's long arithmetic wraps; C++ signed overflow does not, so go through
	// unsigned for anything that can overflow.
	static inline int64_t mul(int64_t a, int64_t b) {
		return (int64_t)((uint64_t)a * (uint64_t)b);
	}

	static inline int fastFloor(double x) {
		int xi = (int)x;
		return x < xi ? xi - 1 : xi;
	}

	static float grad(int64_t seed, int64_t xsvp, int64_t ysvp, float dx, float dy) {
		int64_t hash = seed ^ xsvp ^ ysvp;
		hash = mul(hash, HASH_MULTIPLIER);
		hash ^= hash >> (64 - N_GRADS_2D_EXPONENT + 1);
		int gi = (int)hash & ((N_GRADS_2D - 1) << 1);
		const float *g = gradients().g;
		return g[gi] * dx + g[gi | 1] * dy;
	}

	// The lattice traversal, on coordinates that have already been skewed.
	static float noise2_UnskewedBase(int64_t seed, double xs, double ys) {
		int xsb = fastFloor(xs), ysb = fastFloor(ys);
		float xi = (float)(xs - xsb), yi = (float)(ys - ysb);

		int64_t xsbp = mul((int64_t)xsb, PRIME_X), ysbp = mul((int64_t)ysb, PRIME_Y);

		float t = (xi + yi) * (float)UNSKEW_2D;
		float dx0 = xi + t, dy0 = yi + t;

		float value = 0;
		float a0 = RSQUARED_2D - dx0 * dx0 - dy0 * dy0;
		if (a0 > 0) {
			value = (a0 * a0) * (a0 * a0) * grad(seed, xsbp, ysbp, dx0, dy0);
		}

		float a1 = (float)(2 * (1 + 2 * UNSKEW_2D) * (1 / UNSKEW_2D + 2)) * t +
			((float)(-2 * (1 + 2 * UNSKEW_2D) * (1 + 2 * UNSKEW_2D)) + a0);
		if (a1 > 0) {
			float dx1 = dx0 - (float)(1 + 2 * UNSKEW_2D);
			float dy1 = dy0 - (float)(1 + 2 * UNSKEW_2D);
			value += (a1 * a1) * (a1 * a1) *
				grad(seed, xsbp + PRIME_X, ysbp + PRIME_Y, dx1, dy1);
		}

		if (dy0 > dx0) {
			float dx2 = dx0 - (float)UNSKEW_2D;
			float dy2 = dy0 - (float)(UNSKEW_2D + 1);
			float a2 = RSQUARED_2D - dx2 * dx2 - dy2 * dy2;
			if (a2 > 0) {
				value += (a2 * a2) * (a2 * a2) *
					grad(seed, xsbp, ysbp + PRIME_Y, dx2, dy2);
			}
		} else {
			float dx2 = dx0 - (float)(UNSKEW_2D + 1);
			float dy2 = dy0 - (float)UNSKEW_2D;
			float a2 = RSQUARED_2D - dx2 * dx2 - dy2 * dy2;
			if (a2 > 0) {
				value += (a2 * a2) * (a2 * a2) *
					grad(seed, xsbp + PRIME_X, ysbp, dx2, dy2);
			}
		}

		return value;
	}

	double noise2(int64_t seed, double x, double y) {
		double s = SKEW_2D * (x + y);
		return (double)noise2_UnskewedBase(seed, x + s, y + s);
	}

	double noise1(int64_t seed, double x) {
		// The reference's noise2_ImproveX domain rotation with the second
		// coordinate held at zero, so the line walked through the field is not
		// parallel to any row of the lattice.
		double xx = x * ROOT2OVER2;
		double yy = 0.0;
		return (double)noise2_UnskewedBase(seed, yy + xx, yy - xx);
	}

}
