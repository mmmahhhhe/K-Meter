/* ----------------------------------------------------------------------------

   K-Meter
   =======
   Implementation of a K-System meter according to Bob Katz' specifications

   Copyright (c) 2010-2016 Martin Zuther (http://www.mzuther.de/)

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

#include "standalone_application.h"


StandaloneFilterWindow *KmeterStandalone::createWindow()
{
    // load settings of stand-alone; this includes the directory in
    // which the current state is to be stored
    PropertiesFile::Options settings;

#ifdef KMETER_SURROUND
    settings.applicationName     = "kmeter_surround";
#else
    settings.applicationName     = "kmeter_stereo";
#endif

    settings.filenameSuffix      = "ini";
    settings.folderName          = ".config";
    settings.osxLibrarySubFolder = "Application Support";

    PropertiesFile *propertiesFile = new PropertiesFile(settings);

    // instantiate GUI
    StandaloneFilterWindow *filterWindow = new StandaloneFilterWindow(
        getApplicationName(),
        Colours::lightgrey,
        propertiesFile,
        true);

    // GUI cannot be resized
    filterWindow->setResizable(false, true);

    return filterWindow;
}


void KmeterStandalone::shutdown()
{
    // save plug-in settings
    mainWindow->pluginHolder->savePluginState();

    // kill GUI
    mainWindow = nullptr;
}


START_JUCE_APPLICATION(KmeterStandalone);


// Local Variables:
// ispell-local-dictionary: "british"
// End:
