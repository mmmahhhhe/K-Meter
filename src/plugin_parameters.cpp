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

#include "plugin_parameters.h"


// The methods of this class may be called on the audio thread, so
// they are absolutely time-critical!

KmeterPluginParameters::KmeterPluginParameters()
{
    nParam = new int[nNumParameters];

    nParam[selCrestFactor] = 20;
    nParam[selExpanded] = 0;
    nParam[selPeak] = 0;
    nParam[selInfiniteHold] = 0;
    nParam[selMono] = 0;

    nParam[selValidationSelectedChannel] = -1;
    nParam[selValidationPeakMeterLevel] = 1;
    nParam[selValidationAverageMeterLevel] = 1;
    nParam[selValidationStereoMeterValue] = 1;
    nParam[selValidationPhaseCorrelation] = 1;

    strValidationFile = String::empty;

    bParamChanged = new bool[nNumParameters];

    for (int nIndex = 0; nIndex < nNumParameters; nIndex++)
    {
        UnmarkParameter(nIndex);
    }
}


KmeterPluginParameters::~KmeterPluginParameters()
{
    removeAllChangeListeners();

    delete [] nParam;
    nParam = NULL;

    delete [] bParamChanged;
    bParamChanged = NULL;
}


int KmeterPluginParameters::getNumParameters(bool bIncludeHiddenParameters)
{
    if (bIncludeHiddenParameters)
    {
        return nNumParameters;
    }
    else
    {
        return nNumParametersRevealed;
    }
}


bool KmeterPluginParameters::getParameterAsBool(int nIndex)
{
    return (getParameterAsInt(nIndex) != 0) ? true : false;
}


float KmeterPluginParameters::getParameterAsFloat(int nIndex)
{
    int nValue = getParameterAsInt(nIndex);
    return translateParameterToFloat(nIndex, nValue);
}


int KmeterPluginParameters::getParameterAsInt(int nIndex)
{
    jassert((nIndex >= 0) && (nIndex < nNumParameters));

    return nParam[nIndex];
}


void KmeterPluginParameters::setParameterFromBool(int nIndex, bool bValue)
{
    setParameterFromInt(nIndex, bValue ? 1 : 0);
}


void KmeterPluginParameters::setParameterFromFloat(int nIndex, float fValue)
{
    int nValue = translateParameterToInt(nIndex, fValue);
    setParameterFromInt(nIndex, nValue);
}


void KmeterPluginParameters::setParameterFromInt(int nIndex, int nValue)
{
    jassert((nIndex >= 0) && (nIndex < nNumParameters));

    if (nParam[nIndex] != nValue)
    {
        if (nIndex == selCrestFactor)
        {
            if ((nValue == 0) || (nValue == 12) || (nValue == 14))
            {
                nParam[nIndex] = nValue;
            }
            else
            {
                nParam[nIndex] = 20;
            }
        }
        else if (nIndex == selValidationSelectedChannel)
        {
            nParam[nIndex] = nValue;
        }
        else
        {
            nParam[nIndex] = (nValue != 0) ? 1 : 0;
        }

        MarkParameter(nIndex);
        sendChangeMessage(this);
    }
}


File KmeterPluginParameters::getValidationFile()
{
    File fileValidation = File(strValidationFile);

    if (fileValidation.existsAsFile())
    {
        return fileValidation;
    }
    else
    {
        return File::nonexistent;
    }
}


void KmeterPluginParameters::setValidationFile(File& fileValidation)
{
    if (fileValidation.existsAsFile())
    {
        strValidationFile = fileValidation.getFullPathName();
    }
}


void KmeterPluginParameters::MarkParameter(int nIndex)
{
    jassert((nIndex >= 0) && (nIndex < nNumParameters));

    bParamChanged[nIndex] = true;
}


void KmeterPluginParameters::UnmarkParameter(int nIndex)
{
    jassert((nIndex >= 0) && (nIndex < nNumParameters));

    bParamChanged[nIndex] = false;
}


bool KmeterPluginParameters::isParameterMarked(int nIndex)
{
    jassert((nIndex >= 0) && (nIndex < nNumParameters));

    return bParamChanged[nIndex];
}


const String KmeterPluginParameters::getParameterName(int nIndex)
{
    switch (nIndex)
    {
    case selCrestFactor:
        return "Crest factor";
        break;

    case selExpanded:
        return "Expand";
        break;

    case selPeak:
        return "Peak";
        break;

    case selInfiniteHold:
        return "Hold";
        break;

    case selMono:
        return "Mono";
        break;

    case selValidationFileName:
        return "Validation: file name";
        break;

    case selValidationSelectedChannel:
        return "Validation: selected channel";
        break;

    case selValidationPeakMeterLevel:
        return "Validation: peak meter level";
        break;

    case selValidationAverageMeterLevel:
        return "Validation: average meter level";
        break;

    case selValidationStereoMeterValue:
        return "Validation: stereo meter value";
        break;

    case selValidationPhaseCorrelation:
        return "Validation: phase correlation";
        break;

    default:
        return "invalid";
        break;
    }
}


const String KmeterPluginParameters::getParameterText(int nIndex)
{
    jassert((nIndex >= 0) && (nIndex < nNumParameters));

    if (nIndex == selCrestFactor)
    {
        if (nParam[nIndex] == 0)
        {
            return "Normal";
        }
        else if (nParam[nIndex] == 12)
        {
            return "K-12";
        }
        else if (nParam[nIndex] == 14)
        {
            return "K-14";
        }
        else
        {
            return "K-20";
        }
    }
    else if (nIndex == selValidationFileName)
    {
        File fileValidation = File(strValidationFile);

        if (fileValidation.existsAsFile())
        {
            return strValidationFile;
        }
        else
        {
            return String::empty;
        }
    }
    else if (nIndex == selValidationSelectedChannel)
    {
        if (nParam[nIndex] < 0)
        {
            return "All";
        }
        else
        {
            return String(nParam[nIndex]);
        }
    }
    else
    {
        return getParameterAsBool(nIndex) ? "on" : "off";
    }
}


float KmeterPluginParameters::translateParameterToFloat(int nIndex, int nValue)
{
    jassert((nIndex >= 0) && (nIndex < nNumParameters));

    if (nIndex == selCrestFactor)
    {
        if (nValue == 0)
        {
            return (selNormal / float(nNumCrestFactors - 1));
        }
        else if (nValue == 12)
        {
            return (selK12 / float(nNumCrestFactors - 1));
        }
        else if (nValue == 14)
        {
            return (selK14 / float(nNumCrestFactors - 1));
        }
        else
        {
            return (selK20 / float(nNumCrestFactors - 1));
        }
    }
    else if (nIndex == selValidationSelectedChannel)
    {
        // 0.00f: dump all channels
        // 0.01f: dump channel #0
        // 0.02f: dump channel #1
        // ...
        // 1.00f: dump channel #99
        return (nValue + 1.0f) / 100.0f;
    }
    else
    {
        return (nValue != 0) ? 1.0f : 0.0f;
    }
}


int KmeterPluginParameters::translateParameterToInt(int nIndex, float fValue)
{
    jassert((nIndex >= 0) && (nIndex < nNumParameters));

    if (nIndex == selCrestFactor)
    {
        if (fValue < (selK12 / float(nNumCrestFactors)))
        {
            return 0;
        }
        else if (fValue < (selK14 / float(nNumCrestFactors)))
        {
            return 12;
        }
        else if (fValue < (selK20 / float(nNumCrestFactors)))
        {
            return 14;
        }
        else
        {
            return 20;
        }
    }
    else if (nIndex == selValidationSelectedChannel)
    {
        // 0.00f: dump all channels
        // 0.01f: dump channel #0
        // 0.02f: dump channel #1
        // ...
        // 1.00f: dump channel #99
        int nRoundedValue = int(fValue * 100.0f + 0.5f);
        return nRoundedValue - 1;
    }
    else
    {
        return (fValue > 0.5f) ? true : false;
    }
}


XmlElement KmeterPluginParameters::storeAsXml()
{
    XmlElement xml("KMETER_SETTINGS");

    int nCrestFactor = getParameterAsInt(selCrestFactor);

    if (nCrestFactor == 0)
    {
        xml.setAttribute("CrestFactor", 20);
    }
    else
    {
        xml.setAttribute("CrestFactor", nCrestFactor);
    }

    xml.setAttribute("Expanded", getParameterAsInt(selExpanded));
    xml.setAttribute("Peak", getParameterAsInt(selPeak));
    xml.setAttribute("Hold", getParameterAsInt(selInfiniteHold));
    xml.setAttribute("Mono", getParameterAsInt(selMono));

    xml.setAttribute("ValidationFile", strValidationFile);
    xml.setAttribute("ValidationSelectedChannel", getParameterAsInt(selValidationSelectedChannel));
    xml.setAttribute("ValidationPeakMeterLevel", getParameterAsInt(selValidationPeakMeterLevel));
    xml.setAttribute("ValidationAverageMeterLevel", getParameterAsInt(selValidationAverageMeterLevel));
    xml.setAttribute("ValidationStereoMeterValue", getParameterAsInt(selValidationStereoMeterValue));
    xml.setAttribute("ValidationPhaseCorrelation", getParameterAsInt(selValidationPhaseCorrelation));

    return xml;
}


void KmeterPluginParameters::loadFromXml(XmlElement* xml)
{
    if (xml && xml->hasTagName("KMETER_SETTINGS"))
    {
        int nCrestFactor = 0;

        // make sure settings for K-Meter v1.21 and below load fine
        if (xml->hasAttribute("Headroom"))
        {
            nCrestFactor = xml->getIntAttribute("Headroom", getParameterAsInt(selCrestFactor));
        }
        else
        {
            nCrestFactor = xml->getIntAttribute("CrestFactor", getParameterAsInt(selCrestFactor));
        }

        if (nCrestFactor == 0)
        {
            setParameterFromInt(selCrestFactor, 20);
        }
        else
        {
            setParameterFromInt(selCrestFactor, nCrestFactor);
        }

        setParameterFromInt(selExpanded, xml->getIntAttribute("Expanded", getParameterAsInt(selExpanded)));
        setParameterFromInt(selPeak, xml->getIntAttribute("Peak", getParameterAsInt(selPeak)));
        setParameterFromInt(selInfiniteHold, xml->getIntAttribute("Hold", getParameterAsInt(selInfiniteHold)));
        setParameterFromInt(selMono, xml->getIntAttribute("Mono", getParameterAsInt(selMono)));

        File fileValidation = File(xml->getStringAttribute("ValidationFile", strValidationFile));
        setValidationFile(fileValidation);

        setParameterFromInt(selValidationSelectedChannel, xml->getIntAttribute("ValidationSelectedChannel", getParameterAsInt(selValidationSelectedChannel)));
        setParameterFromInt(selValidationPeakMeterLevel, xml->getIntAttribute("ValidationPeakMeterLevel", getParameterAsInt(selValidationPeakMeterLevel)));
        setParameterFromInt(selValidationAverageMeterLevel, xml->getIntAttribute("ValidationAverageMeterLevel", getParameterAsInt(selValidationAverageMeterLevel)));
        setParameterFromInt(selValidationStereoMeterValue, xml->getIntAttribute("ValidationStereoMeterValue", getParameterAsInt(selValidationStereoMeterValue)));
        setParameterFromInt(selValidationPhaseCorrelation, xml->getIntAttribute("ValidationPhaseCorrelation", getParameterAsInt(selValidationPhaseCorrelation)));
    }
}


// Local Variables:
// ispell-local-dictionary: "british"
// End:
