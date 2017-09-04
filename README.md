# Hawkeye
Yup, this gives you the full spherical camera rig, but not the math to put it together


OK, grab libArducam,libMMMUtil,libRESTServer and Hawkeye - dump them all in a common directory

and here's how to force build everything - optimizing it is up to you

cd ~/libMMMUtil
./clean.sh
make
cd ~/libArducam
./clean.sh
make
cd ~/libRESTServer
./clean.sh
make
cd ~/Hawkeye
./clean.sh
make
