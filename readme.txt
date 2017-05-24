This is command line tool to generate X-Plane 10 sceneries with high-quality satellite photos from GoogleMaps as ground textures.
Project written in C++, using Qt library.

Use 'build.bat' file to build project under Windows.
Required tools:
- MS Visual Studio, 2013 Community Edition was used.
- Qt-5, http://qt-project.org.
- DSFTool.exe from X-Plane command line tools package (http://developer.x-plane.com/tools/xptools/) to generate .dsf scenery files.

Usage:
1. Install MSVC and Qt.
2. Starting geographic coordinates currently are in 'main.cpp' (will be moved to config).
   There are one more parameter - steps count. Program will try to download square of '2*steps+1' tiles.
   Remember, that GoogleMaps have requests limits, near 1000 requests per day without access key, so for first run 16 will be enough.
3. Build project, debug configuration used by default.
4. Run 'gmaps2xplane.exe' in 'debug/' directory, and wait until job is done. Coordinates of every processed tile will be printed.
   As result there are many files with .pol and .png extensions will be created in directory "debug/scenario_test/objects".
5. Go to directory "debug\scenario_test\Earth nav data\+50+030\" and extract DSFTool.exe there.
6. Run 'gen.bat', it will generate binary scenery file ("+50+030.dsf").
7. Copy or move all contents of 'scenario_test/' directory to 'Custom scenery/' of X-Plane 10 installation folder.
8. Run X-Plane, select UKKK airport (Kyiv, Zhulyany), take off from runway 08, turn left to course 15 degrees, fly towards city center, then look down.

This is initial version. In future all scenery generation steps will be simplified.
