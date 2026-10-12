/** Copyright (C) 2006, Ian Paul Larsen.
 **
 **  This program is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 **/

// Whole-array opcodes that take the array's variable number (OPTYPE_VARIABLE).
//
// Interpreter::execByteCode() in Interpreter.cpp decodes each opcode and
// hands these ones to execArrayOp() with the variable number.  A break ends
// the opcode, and the checks execByteCode() makes after every opcode still
// run when this returns.

#include <algorithm>
#include <numeric>

#include "InterpreterPrivate.h"

namespace {

	// SORT option bits, pushed by sortstmt in basicParse.y (SORTFLAG_*)
	const int SORT_DESCENDING = 1;
	const int SORT_IGNORECASE = 2;
	const int SORT_COLUMN = 4;

	// Where a value falls in the sort order: all numbers first, then all
	// strings, then anything else, and unassigned elements always last.
	// Values of different kinds are never converted to be compared.  The
	// comparison the rest of the language uses turns a string into a number
	// when the other side is one, so "abc" = 0 and "b" = 0 but "abc" < "b" -
	// an order no sort can follow, and one std::sort may crash on.
	enum SortRank { RANK_NUMBER, RANK_NAN, RANK_STRING, RANK_OTHER, RANK_UNASSIGNED };

	int sortRank(DataElement *e) {
		switch (DataElement::getType(e)) {
			case T_INT: return RANK_NUMBER;
			case T_FLOAT: return std::isnan(e->floatval) ? RANK_NAN : RANK_NUMBER;
			case T_STRING: return RANK_STRING;
			case T_UNASSIGNED: return RANK_UNASSIGNED;
			default: return RANK_OTHER;
		}
	}

	// -1, 0 or 1 for two values of the same rank.  Numbers compare exactly,
	// not with the epsilon of Convert::compareFloats(): "equal to within
	// epsilon" is not transitive, and a sort needs it to be.
	int sortCompareSameRank(DataElement *a, DataElement *b, int rank, Qt::CaseSensitivity cs) {
		if (rank == RANK_NUMBER) {
			if (a->type == T_INT && b->type == T_INT) {
				return (a->intval > b->intval) - (a->intval < b->intval);
			}
			const double x = (a->type == T_INT) ? (double) a->intval : a->floatval;
			const double y = (b->type == T_INT) ? (double) b->intval : b->floatval;
			return (x > y) - (x < y);
		}
		if (rank == RANK_STRING) {
			const int c = a->stringval.compare(b->stringval, cs);
			return (c > 0) - (c < 0);
		}
		return 0;
	}

}

void Interpreter::execArrayOp(int opcode, int i) {
	switch(opcode) {

		case OP_SORT: {
			// SORT array [, column] [, ascending | descending] [, ignorecase]
			// A one row array (every 1D array) has its elements sorted; any
			// other array has its rows sorted on one column, the first unless
			// a column is given, and each row moves as a whole.  The sort is
			// stable, so rows with equal keys keep their order and a sort on
			// one column followed by a sort on another gives a two-key order.
			const int flags = stack->popInt();
			DataElement *columnE = stack->popDE();			// RELEASE
			DataElement *a = variables->getData(i);			// DONT RELEASE

			// A map or a single value: trappable, unlike ERROR_NOTARRAY
			if (DataElement::getType(a) != T_ARRAY) {
				error->q(ERROR_EXPECTEDARRAY);
				delete columnE;
				break;
			}

			const int rows = a->arrayRows();
			const int cols = a->arrayCols();
			int key = 0;
			if (flags & SORT_COLUMN) {
				key = convert->getInt(columnE) - arraybase;
				if (key < 0 || key >= cols) {
					error->q(ERROR_ARRAYINDEX, i);
					delete columnE;
					break;
				}
			}
			delete columnE;

			// The things being reordered: each element of a one row array
			// with no column given, otherwise each row
			const bool elements = (rows == 1 && !(flags & SORT_COLUMN));
			const int count = elements ? cols : rows;
			const int width = elements ? 1 : cols;
			const int keyOffset = elements ? 0 : key;
			std::vector<DataElement> &data = a->arr->data;

			const bool descending = flags & SORT_DESCENDING;
			const Qt::CaseSensitivity cs = (flags & SORT_IGNORECASE) ? Qt::CaseInsensitive : Qt::CaseSensitive;

			// Sort the positions, then move the elements once.  Descending
			// reverses the comparison rather than the result, which would
			// also reverse rows with equal keys and lose the stability.
			// Unassigned elements go last either way.
			std::vector<int> order(count);
			std::iota(order.begin(), order.end(), 0);
			std::stable_sort(order.begin(), order.end(), [&](int x, int y) {
				DataElement *p = &data[x * width + keyOffset];
				DataElement *q = &data[y * width + keyOffset];
				const int rp = sortRank(p);
				const int rq = sortRank(q);
				if (rp != rq) {
					if (rp == RANK_UNASSIGNED || rq == RANK_UNASSIGNED) return rp < rq;
					return descending ? rp > rq : rp < rq;
				}
				const int c = sortCompareSameRank(p, q, rp, cs);
				return descending ? c > 0 : c < 0;
			});

			std::vector<DataElement> sorted(data.size());
			for (int k = 0; k < count; k++) {
				for (int c = 0; c < width; c++) {
					sorted[k * width + c].stealFrom(&data[order[k] * width + c]);
				}
			}
			data.swap(sorted);
			watchvariable(debugMode, i);
		}
		break;

		case OP_ARRAYADD: {
			// ARRAYADD array, value [, value ...]
			// The values are added to the end of the array as one new row.
			// A single value added to a one row array (every 1D array)
			// lengthens that row instead, so a list grows one element at a
			// time.  One array given on its own stands for its elements: a
			// 1D array is a row of values, and each row of a 2D array is
			// added in turn.  An unassigned variable becomes an array of the
			// rows added, and every other array keeps its columns, which the
			// new rows have to match.
			const int n = stack->popInt();
			std::vector<DataElement> values(n);
			for (int k = n - 1; k >= 0; k--) values[k].stealFrom(stack->popDEborrow());
			DataElement *a = variables->getData(i);			// DONT RELEASE

			DataElement *source = (n == 1 && values[0].type == T_ARRAY) ? &values[0] : NULL;
			const int addRows = source ? source->arrayRows() : 1;
			const int addCols = source ? source->arrayCols() : n;

			int rows = 0;
			int cols = addCols;
			if (DataElement::getType(a) == T_ARRAY) {
				rows = a->arrayRows();
				cols = a->arrayCols();
			} else if (DataElement::getType(a) != T_UNASSIGNED) {
				// A map or a single value: trappable, unlike ERROR_NOTARRAY
				error->q(ERROR_EXPECTEDARRAY);
				break;
			}

			// where the new values go: after the last element of a one row
			// array being given one value, otherwise in new rows at the end
			const bool lengthen = (rows == 1 && addRows == 1 && addCols == 1);
			if (lengthen) {
				a->arrayDim(1, cols + 1, true);
			} else {
				if (addCols != cols) {
					error->q(ERROR_ARRAYNITEMS, i);
					break;
				}
				a->arrayDim(rows + addRows, cols, true);
			}
			if (DataElement::getError()) {
				error->q(DataElement::getError(true), i);
				break;
			}

			std::vector<DataElement> &data = a->arr->data;
			const int start = lengthen ? cols : rows * cols;
			for (int k = 0; k < addRows * addCols; k++) {
				data[start + k].stealFrom(source ? &source->arr->data[k] : &values[k]);
			}
			watchvariable(debugMode, i);
		}
		break;

	}
}
