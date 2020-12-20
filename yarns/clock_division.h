// Copyright 2013 Emilie Gillet.
// Copyright 2020 Chris Rogers.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Clock division definitions.

#ifndef YARNS_CLOCK_DIVISION_H_
#define YARNS_CLOCK_DIVISION_H_

namespace yarns {
namespace clock_division {

struct ClockDivision {
  const char* const display; // Ratio out:in
  const uint16_t num_ticks; // PPQN
};

const uint8_t count = 26;
const uint8_t unity = 14;
const ClockDivision list[count] = {
  { "18", 192 },
  { "14", 96 },
  { "27", 84 },
  { "13", 72 },
  { "38", 64 },
  { "25", 60 },
  { "37", 56 },
  { "12", 48 },
  { "47", 42 },
  { "35", 40 },
  { "23", 36 },
  { "34", 32 },
  { "45", 30 },
  { "67", 28 },
  { "11", 24 },
  { "87", 21 },
  { "65", 20 },
  { "43", 18 },
  { "32", 16 },
  { "85", 15 },
  { "21", 12 },
  { "83", 9 },
  { "31", 8 },
  { "41", 6 },
  { "61", 4 },
  { "81", 3 },
};

}  // namespace clock_division
}  // namespace yarns

#endif // YARNS_CLOCK_DIVISIONS_H_
