/* ----------------------------------------------------------------------------

   K-Meter
   =======
   Implementation of a K-System meter according to Bob Katz' specifications

   Copyright (c) 2010-2011 Martin Zuther (http://www.mzuther.de/)

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

#include "peak_label.h"

PeakLabel::PeakLabel(const String& componentName, int nCrestFactor) : Label(componentName, T(""))
{
    nMeterCrestFactor = nCrestFactor;

    resetLevel();

    setFont(12.0f);
    setJustificationType(Justification::centredRight);
    setColour(Label::backgroundColourId, Colours::grey.darker(0.7f));
    setColour(Label::textColourId, Colours::white);
    setColour(Label::outlineColourId, Colours::grey.darker(0.2f));
}

PeakLabel::~PeakLabel()
{
}

void PeakLabel::resetLevel()
{
    float fMaximumCrestFactor = 20.0f; // i.e. K-20

    // the RMS of a sine wave is its amplitude divided by the square
    // root of 2, thus the difference between peak value and RMS is the
    // square root of 2 -- so let's convert this difference to dB
    float fPeakToAverageCorrection = 20.0f * log10(sqrt(2.0f));

    float fMeterMinimumDecibel = -(fMaximumCrestFactor + fPeakToAverageCorrection + 70.0f);

    // ensure peak label update on next call of updateLevel()
    fMaximumLevel = fMeterMinimumDecibel - 0.1f;
}

void PeakLabel::updateLevel(float newLevel)
{
    if (newLevel == fMaximumLevel)
    {
        return;
    }

    fMaximumLevel = newLevel;
    float fCorrectedLevel = fMaximumLevel + nMeterCrestFactor;

    if (fCorrectedLevel < 0.0f)
    {
        setText(String(fCorrectedLevel, 1), false);
    }
    else
    {
        setText(String("+") + String(fCorrectedLevel, 1), false);
    }

    if (fMaximumLevel < 0.0f)
    {
        setColour(Label::backgroundColourId, Colours::grey.darker(0.7f));
    }
    else
    {
        setColour(Label::backgroundColourId, Colours::red.darker(0.2f));
    }
}


// Local Variables:
// ispell-local-dictionary: "british"
// End:
