#!/bin/sh
echo - Retrieving code from source control
git pull
echo - Building project Makefiles
premake5 gmake --openmp
echo - Building release version
make config=release -j2
echo - Installing release version
./install.sh
echo - DONE.
