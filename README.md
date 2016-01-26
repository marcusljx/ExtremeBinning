# ExtremeBinning
Small scale C++ implementation of Extreme Binning. Based on [method published by Bhagwat, Deepavali, et al.](http://ieeexplore.ieee.org/xpl/login.jsp?tp=&arnumber=5366623&url=http%3A%2F%2Fieeexplore.ieee.org%2Fxpls%2Fabs_all.jsp%3Farnumber%3D5366623)
-----
# HOW TO USE
## Building
From the source directory, run:
```
cmake . && make
```
This will build two executables, `XB_Backup` and `XB_Restore`.

## Running
To back up a directory, run:
```
./XB_Backup <targetDirectory> <backupLocation>
```

To restore a directory, run:
```
./XB_Restore <backupLocation> <restoreLocation>
```
## IMPORTANT!
- Currently, all input parameters (directory paths) must end with `/` eg. `~/documents/` or `folder/sampleDirectory/`
- This program has been built and tested on Linux Ubuntu 14.04 only.
