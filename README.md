# viscloud/labeler

### 1. Overview

This repository contains two artifacts:
1. `labeler.cpp`: A script that assists with manually labeling large amounts
of video data with the temporal boundaries of interesting events.
2. `labels` directory: The label data files generated by the above script
for the various datasets that the `viscloud` organization uses.

### 2. Compilation

`labeler.cpp` requires OpenCV with Qt support. Most likely, you will need to
install Qt and OpenCV from source. If your installation of OpenCV is already
built with Qt, then skip to section 2.3.

#### 2.1 Clone and compile Qt.

```sh
git clone git@github.com:qt/qt5
cd qt5
git checkout 5.11
git submodule update --init
./configure -prefix $PWD/qtbase -opensource -nomake tests
make
```
**Note:** This takes a long time.

#### 2.2. Clone and compile OpenCV, making sure to enable Qt support.

```sh
git clone git@github.com:opencv/opencv
cd opencv
git checkout 3.3.0
mkdir build
cd build
# TODO: cmake arguments
cmake ..
make
sudo make install
```

#### 2.3. Clone and compile the labeler.

```sh
git clone git@github.com:viscloud/labeler
make
```
