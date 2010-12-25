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

#include "stereo_kmeter.h"

StereoKmeter::StereoKmeter(const String &componentName, int posX, int posY, int nHeadroom, bool bExpanded, bool bDisplayPeakMeter, int nSegmentHeight)
{
	setName(componentName);
	isExpanded = bExpanded;
	displayPeakMeter = bDisplayPeakMeter;

	nPosX = posX;
	nPosY = posY;
	nMainSegmentHeight = nSegmentHeight;

	if (nHeadroom == 0)
		nMeterHeadroom = 0;
	else if (nHeadroom == 12)
		nMeterHeadroom = 12;
	else if (nHeadroom == 14)
		nMeterHeadroom = 14;
	else
		nMeterHeadroom = 20;

	if (displayPeakMeter)
	{
	  PeakMeterLeft = new MeterBar(T("Peak Meter Left"), 3, 48, 9, nMeterHeadroom, bExpanded, nMainSegmentHeight, T("left"));
	  PeakMeterRight = new MeterBar(T("Peak Meter Right"), 94, 48, 9, nMeterHeadroom, bExpanded, nMainSegmentHeight, T("right"));

	  addAndMakeVisible(PeakMeterLeft);
	  addAndMakeVisible(PeakMeterRight);
	
	  AverageMeterLeft = new MeterBar(T("Average Meter Left"), 17, 48, 18, nMeterHeadroom, bExpanded, nMainSegmentHeight, T("center"));
	  AverageMeterRight = new MeterBar(T("Average Meter Right"), 71, 48, 18, nMeterHeadroom, bExpanded, nMainSegmentHeight, T("center"));

	  addAndMakeVisible(AverageMeterLeft);
	  addAndMakeVisible(AverageMeterRight);
  	}
	else
	{
	  AverageMeterLeft = new MeterBar(T("Average Meter Left"), 7, 48, 20, nMeterHeadroom, bExpanded, nMainSegmentHeight, T("center"));
	  AverageMeterRight = new MeterBar(T("Average Meter Right"), 79, 48, 20, nMeterHeadroom, bExpanded, nMainSegmentHeight, T("center"));

	  addAndMakeVisible(AverageMeterLeft);
	  addAndMakeVisible(AverageMeterRight);
  	}

	OverflowMeterLeft = new OverflowMeter(T("Overflows Left"));
	OverflowMeterLeft->setBounds(3, 3, 32, 16);
	addAndMakeVisible(OverflowMeterLeft);

	OverflowMeterRight = new OverflowMeter(T("Overflows Right"));
	OverflowMeterRight->setBounds(71, 3, 32, 16);
	addAndMakeVisible(OverflowMeterRight);

	MaximumPeakLeft = new PeakLabel(T("Maximum Peak Left"), nHeadroom);
	MaximumPeakLeft->setBounds(3, 23, 32, 16);
	addAndMakeVisible(MaximumPeakLeft);

	MaximumPeakRight = new PeakLabel(T("Maximum Peak Right"), nHeadroom);
	MaximumPeakRight->setBounds(71, 23, 32, 16);
	addAndMakeVisible(MaximumPeakRight);
}

StereoKmeter::~StereoKmeter()
{
	deleteAllChildren();
}
	
void StereoKmeter::visibilityChanged()
{
	int height = 134 * nMainSegmentHeight + 52;
	setBounds(nPosX, nPosY, 106, height);
}

void StereoKmeter::paint(Graphics& g)
{
	g.fillAll(Colours::grey.withAlpha(0.1f));

	g.setColour(Colours::darkgrey);
	g.drawRect(0, 0, getWidth() - 1, getHeight() - 1);

	g.setColour(Colours::darkgrey.darker(0.8f));
	g.drawRect(1, 1, getWidth() - 1, getHeight() - 1);

	g.setColour(Colours::darkgrey.darker(0.4f));
	g.drawRect(1, 1, getWidth() - 2, getHeight() - 2);

	int x = 3;
	int y = 43;
	int width = 24;
	int height = 11;
	String strMarker;

	g.setColour(Colours::white);
	g.setFont(12.0f);

	g.drawFittedText("Over", 35, 3, 36, 16, Justification::centred, 1, 1.0f);
	g.drawFittedText("Peak", 35, 23, 36, 16, Justification::centred, 1, 1.0f);

	g.setFont(11.0f);

	if (isExpanded)
	{
		y -= 10 * nMainSegmentHeight;
		int nStart = 0;

		if (nMeterHeadroom < 8)
			nStart = 0;
		else
			nStart = 8; // zoom into important region

		for (int n=0; n >= -13; n -= 1)
		{
			if ((nStart + n) > 0)
				strMarker = T("+") + String(nStart + n);
			else
				strMarker = String(nStart + n);

			y += 10 * nMainSegmentHeight;
			drawMarkers(g, strMarker, x, y, width, height);
		}
	}
	else if (nMeterHeadroom == 0)
	{
		y -= 8 * nMainSegmentHeight;

		for (int n=0; n >= -40; n -= 4)
		{
			if (n > 0)
				strMarker = T("+") + String(n);
			else
				strMarker = String(n);

			y += 8 * nMainSegmentHeight;
			drawMarkers(g, strMarker, x, y, width, height);
		}

		for (int n=-50; n >= -80; n -= 10)
		{
			strMarker = String(n);

			y += 10 * nMainSegmentHeight;
			drawMarkers(g, strMarker, x, y, width, height);
		}
	}
	else if (nMeterHeadroom == 12)
	{
		y -= 8 * nMainSegmentHeight;

		for (int n=12; n >= -28; n -= 4)
		{
			if (n > 0)
				strMarker = T("+") + String(n);
			else
				strMarker = String(n);

			y += 8 * nMainSegmentHeight;
			drawMarkers(g, strMarker, x, y, width, height);
		}

		y -= 6 * nMainSegmentHeight;

		for (int n=-30; n >= -60; n -= 10)
		{
			strMarker = String(n);

			y += 10 * nMainSegmentHeight;
			drawMarkers(g, strMarker, x, y, width, height);
		}
	}
	else if (nMeterHeadroom == 14)
	{
		strMarker = String(T("+14"));
		drawMarkers(g, strMarker, x, y, width, height);
		y -= 4 * nMainSegmentHeight;

		for (int n=12; n >= -28; n -= 4)
		{
			if (n > 0)
				strMarker = T("+") + String(n);
			else
				strMarker = String(n);

			y += 8 * nMainSegmentHeight;
			drawMarkers(g, strMarker, x, y, width, height);
		}

		y -= 6 * nMainSegmentHeight;

		for (int n=-30; n >= -60; n -= 10)
		{
			strMarker = String(n);

			y += 10 * nMainSegmentHeight;
			drawMarkers(g, strMarker, x, y, width, height);
		}
	}
	else
	{
		y -= 8 * nMainSegmentHeight;

		for (int n=20; n >= -24; n -= 4)
		{
			if (n > 0)
				strMarker = T("+") + String(n);
			else
				strMarker = String(n);

			y += 8 * nMainSegmentHeight;
			drawMarkers(g, strMarker, x, y, width, height);
		}

		y -= 4 * nMainSegmentHeight;

		for (int n=-30; n >= -60; n -= 10)
		{
			strMarker = String(n);

			y += 10 * nMainSegmentHeight;
			drawMarkers(g, strMarker, x, y, width, height);
		}
	}
}

void StereoKmeter::resized()
{
}

void StereoKmeter::setLevels(MeterBallistics* pMeterBallistics)
{
	if (displayPeakMeter && pMeterBallistics)
	{
	  PeakMeterLeft->setLevels(pMeterBallistics->getPeakMeter(0), pMeterBallistics->getPeakMeterPeak(0));
	  PeakMeterRight->setLevels(pMeterBallistics->getPeakMeter(1), pMeterBallistics->getPeakMeterPeak(1));
	}

	MaximumPeakLeft->updateLevel(pMeterBallistics->getPeakMeterMaximumPeak(0));
	MaximumPeakRight->updateLevel(pMeterBallistics->getPeakMeterMaximumPeak(1));

	AverageMeterLeft->setLevels(pMeterBallistics->getAverageMeter(0), pMeterBallistics->getAverageMeterPeak(0));
	AverageMeterRight->setLevels(pMeterBallistics->getAverageMeter(1), pMeterBallistics->getAverageMeterPeak(1));

	OverflowMeterLeft->setOverflows(pMeterBallistics->getOverflows(0));
	OverflowMeterRight->setOverflows(pMeterBallistics->getOverflows(1));
}

void StereoKmeter::drawMarkers(Graphics& g, String& strMarker, int x, int y, int width, int height)
{
	g.setColour(Colours::white);
	g.drawFittedText(strMarker, x + 38, y, width, height, Justification::centred, 1, 1.0f);

	g.setColour(Colours::grey);

	int nMarkerY = y + 5;
	int nStart = 0;
	int nEnd = 0;
	int nWidth = 0;

	if (displayPeakMeter)
	{
	  nWidth = 3;
	  nStart = x + 10;
	  nEnd = nStart + nWidth;
	}
	else
	{
	  nWidth = 9;
	  nStart = x + 25;
	  nEnd = nStart + nWidth;
	}

	for (int nMarkerX=nStart; nMarkerX < nEnd; nMarkerX++)
	  g.setPixel(nMarkerX, nMarkerY);

	if (displayPeakMeter)
	{
	  nStart = x + 89;
	  nEnd = nStart - nWidth;
	}
	else
	{
	  nStart = x + 74;
	  nEnd = nStart - nWidth;
	}

	for (int nMarkerX=nStart; nMarkerX > nEnd; nMarkerX--)
	  g.setPixel(nMarkerX, nMarkerY);
}


// Local Variables:
// ispell-local-dictionary: "british"
// End:
