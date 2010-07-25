/* ----------------------------------------------------------------------------

   K-Meter
   =======
   Implementation of a K-System meter according to Bob Katz' specifications

   Copyright (c) 2010 Martin Zuther (http://www.mzuther.de/)

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Thank you for using free software!

---------------------------------------------------------------------------- */

#include "average_level_rms.h"

AverageLevelRms::AverageLevelRms(AudioSampleBuffer* buffer, int buffer_size, int sample_rate)
{
  pSampleBuffer = buffer;
  nSampleRate = sample_rate;
  nBufferSize = buffer_size;
}

AverageLevelRms::~AverageLevelRms()
{
}

float AverageLevelRms::getLevel(int channel, int sample_rate)
{
  if (nSampleRate != sample_rate)
	 nSampleRate = sample_rate;

  return pSampleBuffer->getRMSLevel(channel, 0, nBufferSize);
}
