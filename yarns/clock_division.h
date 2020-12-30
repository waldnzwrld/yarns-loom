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

const uint8_t count = 29;
const uint8_t unity = 17;
const ClockDivision list[count] = {
  { "18 1/8", 192 },
  { "29 2/9", 108 },
  { "14 1/4", 96 },
  { "27 2/7", 84 },
  { "13 1/3", 72 },
  { "38 3/8", 64 },
  { "25 2/5", 60 },
  { "37 3/7", 56 },
  { "49 4/9", 54 },
  { "12 1/2", 48 },
  { "47 4/7", 42 },
  { "35 3/5", 40 },
  { "23 2/3", 36 },
  { "34 3/4", 32 },
  { "45 4/5", 30 },
  { "67 6/7", 28 },
  { "89 8/9", 27 },
  { "11 1/1", 24 },
  { "87 8/7", 21 },
  { "65 6/5", 20 },
  { "43 4/3", 18 },
  { "32 3/2", 16 },
  { "85 8/5", 15 },
  { "21 2/1", 12 },
  { "83 8/3", 9 },
  { "31 3/1", 8 },
  { "41 4/1", 6 },
  { "61 6/1", 4 },
  { "81 8/1", 3 },
};

}  // namespace clock_division
}  // namespace yarns

#endif // YARNS_CLOCK_DIVISIONS_H_
