# Wayland and Weston

Wayland is a replacement for the X11 window system protocol and architecture with the aim to be easier to develop, extend, and maintain. More information is available on the Wayland site https://wayland.freedesktop.org .

Weston is the reference implementation of a Wayland compositor, as well as a useful enviroinment in and of itself. More information is available on the Weston site https://wayland.freedesktop.org/weston .

Wayland and Weston are used to dual sensor display in V4L2 mode.

---

# Environment Requirement

The recommended environment list are as follows:
* **ubuntu**: Ubuntu 16.04.5 LTS
* **python**: 3.5.2
* **ninja**: 1.7.1
* **meson**: 0.54.0
* **wayland**: 1.20.0
* **wayland-protocol**: 1.22
* **libinput**: 1.8.0
* **weston**: 7.0.0

**Note**: Whether the version of related environment is supported depends on your system(such as ubuntu version), it may not support the latest version of related environment, you can choose the appropriate veresion according to your system.

**e.g.**: We choose weston 7.0.0 for Ubuntu 16.04.5 LTS, weston's `meson.build` file given the requirements as follows: `meson_version >= 0.47`, `wayland-server and wayland-client >= 1.12.0`, `libinput >= 0.8.0` and so on. In the same way, libinput, wayland's `meson.build` file also given the requirements, all that together determines the final version.

The code website of the related environment is as follows:
* **wayland,wayland-protocol,weston**: https://gitlab.freedesktop.org/wayland/
* **libinput**: https://gitlab.freedesktop.org/libinput/

---

# Installation Steps
For installation steps, please refer to the official website https://wayland.freedesktop.org/building.html or the steps described below.

## Ninja

* **install ninja**: we recommend that use the apt to install specified version, or select source code installation method if apt install failed because system not support corresponding version.

```bash
# check if installed
ninja --version

# install by apt
sudo apt install ninja-build

# install the specified version by apt
sudo apt-cache show ninja-build
sudo apt install ninja-build=version

# install by source code
git clone https://github.com/ninja-build/ninja.git
cd ninja
# specfy tag v1.7.1 if need， if source code is .zip format, don't decompress in windows system, all operation in linux system
# git checkout -b v1.7.1 v1.7.1
./configure.py --bootstrap
cp ./ninja /usr/bin
```
## Pip3
* **install pip3**：

```bash
# check if installed
pip3 --help

# install by apt
sudo apt install python3-pip

# cannot import name "sysconfig" from "distutils"
apt-get remove python3-pip
vim /etc/apt/sources.list
apt-get update
apt-get install python3-pip
```

## Meson
* **install meson**：

```bash
# check if installed
meson -v

# install by pip3
sudo pip3 install meson

# install specified version meson.tar file by pip3
# download website: https://pypi.org/
sudo pip3 install meson.xxx.tar

#  ERROR: Could not find a version that satisfies the requirement setuptools (from versions: none)
pip3 install -U setuptools

# Temporary failure in name resolution')': /simple/setuptools/
vi /etc/hosts
```

## Wayland
* **environment variable**

```bash
export WLD=/usr
export LD_LIBRARY_PATH=$WLD/lib
export PKG_CONFIG_PATH=$WLD/lib/pkgconfig/:$WLD/share/pkgconfig/
export PATH=$WLD/bin:$PATH
```

* **install Wayland**: we recommend source code method to install specified version.

```bash
git clone https://gitlab.freedesktop.org/wayland/wayland.git
cd wayland
# specfy tag 1.20.0 if need
# git checkout -b 1.20.0 1.20.0
meson build/ --prefix=/usr
sudo ninja -C build/ install
```

* **Note**: it may occurs ERROR for xx environment is not exist or version not meets requires when meson build, you can refer to the methods or steps as follows:

```bash
# search related repo that include specified name
sudo apt-cache search <name>

# view the support version list on this system
sudo apt-cache show <name>
sudo apt install <name>=version

# install x.deb by apt, download .deb file on https://packages.ubuntu.com
sudo apt install <pathname>

# meson ERROR: install libxml-2.0
sudo apt install libxml2-dev

# meson ERROR: xsltproc
sudo apt install xsltproc

# Dependency "libffi" not found
apt install libffi-dev

# meson ERROR: xmlto
sudo apt install xmlto

# meson ERROR: graphviz or dot no found
sudo apt-get install graphviz

# error not found doxygen
apt install doxygen

# doxygen: error while loading shared libraries: libclang-9.so.1: cannot open shared object file: No such file or directory
meson build/ --prefix=/usr -Ddocumentation=false

## Wayland protocols
**install Wayland protocols**: we recommend source code method to install specified version.

git clone https://gitlab.freedesktop.org/wayland/wayland-protocols.git
cd wayland-protocols
# specfy tag 1.22 if need
# git checkout -b 1.22 1.22
meson build/ --prefix=/usr
sudo ninja -C build/ install
```

* **check whether install success**: `wayland-scanner -v`

## Libinput
* **dependency**: 

```bash
# dependency
UBUNTU_PACKAGES:  'git gcc g++ pkg-config meson check libudev-dev libevdev-dev doxygen graphviz python3-sphinx python3-recommonmark python3-sphinx-rtd-theme python3-pytest-xdist libwacom-dev libcairo2-dev libgtk-3-dev libglib2.0-dev libmtdev-dev'
```

* **install libinput**: we recommend source code method to install specified version.

```bash
# install dependency, maybe enough
sudo apt-get build-dep libinput

# building libinput
git clone https://gitlab.freedesktop.org/libinput/libinput
cd libinput
# specfy tag 1.8.0 if need  我安装的1.8.0版本
# git checkout -b 1.8.0 1.8.0
#sudo apt-get build-dep libinput
meson --prefix=/usr builddir/ -Ddocumentation=false -Ddebug-gui=false -Dtests=false
# valgrind not found, use command: meson --prefix=/usr builddir/ -Ddocumentation=false -Ddebug-gui=false -Dtests=false
sudo ninja -C builddir/ install
```

* **build options**: all the build options can be found in the file `meson_options.txt`. You can set unneeded option false to avoid ERROR when meson build.

* **note**: it may occurs ERROR for xx environment is not exist or version not meets requires when meson build, you can refer to the methods or steps as follows:

```bash
# ERROR: Dependency "libudev" not found
apt-get install libudev-dev

#  ERROR: Dependency "mtdev" not found
apt-get install libmtdev-dev

# ERROR: Dependency "libevdev" not found
apt-get install libevdev-dev

#  ERROR: Dependency "libwacom" not found
apt-get install libwacom-dev

# meson ERROR: gtk-3
sudo apt-get install libgtk-3-dev

# meson ERROR: check
sudo apt-get install check

# meson ERROR: sphinx-build
sudo apt-get install python3-sphinx
...
```

## Weston
* **install weston**: we recommend source code method to install specified version.

```bash
# install dependency, maybe enough
sudo apt-get build-dep weston

# building weston
git clone https://gitlab.freedesktop.org/wayland/weston.git
cd weston
# specfy tag 7.0.0 if need
# git checkout -b 7.0.0 7.0.0
meson build/ --prefix=/usr -Dimage-webp=false -Dbackend-rdp=false -Dbackend-vnc=false -Dpipewire=false -Dsimple-clients=im -Dcolor-management-colord=false -Dbackend-drm-screencast-vaapi=false -Dsimple-dmabuf-drm=intel -Ddemo-clients=false

meson build/ --prefix=/usr -Dimage-webp=false -Dbackend-rdp=false -Dpipewire=false -Dsimple-dmabuf-drm=intel

sudo ninja -C build/ install
```

* **build options**: all the build options can be found in the file `meson_options.txt`. You can set unneeded option false to avoid ERROR when meson build.

* **note**: it may occurs ERROR for xx environment is not exist or version not meets requires when meson build, you can refer to the methods or steps as follows:

```bash
# ERROR：xkbcommon not found
apt install libxkbcommon-dev

# ERROR: Dependency "pixman-1" not found
apt install libpixman-1-dev

# ERROR: Dependency "libdrm" not found
apt-get install libdrm-dev

# ERROR: Dependency "cairo" not found
apt install libcairo-dev

# ERROR: Problem encountered: JPEG image loading requires libjpeg or jpeg, neither was found. Or, you can use '-Dimage-jpeg=false'
apt-get install libjpeg-dev

# ERROR: Problem encountered: gl-renderer requires glesv2 which was not found. #这个必须安装，显示组件
# ERROR: Problem encountered: libweston + gl-renderer requires egl which was not found.
apt install libgles2-mesa-dev

# ERROR: Problem encountered: logind support requires libsystemd or libsystemd-login but neither was found. 
apt install libsystemd-dev

# ERROR: Problem encountered: logind support requires dbus-1 >= 1.6 which was not found.
apt install libdbus-1-dev

# ERROR: C library 'pam' not found
apt install libpam-dev

# ERROR: Problem encountered: drm-backend requires gbm which was not found #这个必须安装，不能选为false
apt install libgbm-dev

# ERROR: Problem encountered: xwayland requires xcb-composite which was not found. Or, you can use '-Dxwayland=false'
apt install libxcb-composite0-dev

# ERROR: Problem encountered: LCMS must be enabled to support colord
# ERROR: Problem encountered: cms-static requires lcms2 which was not found.
apt install liblcms2-dev

# ERROR: Problem encountered: cms-colord requires colord >= 0.1.27 which was not found
apt install libcolord-dev

# ERROR: Problem encountered: VA-API recorder requires libva >= 0.34.0 which was not found.
apt install libva-dev

# ERROR: Problem encountered: x11-backend requires x11-xcb which was not found. Or, you can use '-Dbackend-x11=false'.
apt install libx11-xcb-dev

# ERROR: Problem encountered: xwayland requires xcursor which was not found. Or, you can use '-Dxwayland=false'.
apt search xcursor
apt install libxcursor-dev

# meson ERROR: pciaccess 
sudo apt-get install libpciaccess-dev

# meson ERROR: gstreamer-1.0
sudo apt install gstreamer-1.0

# ERROR: Problem encountered: Remoting plugin requires gstreamer-1.0 which was not found. If you rather not build this, set '-Dremoting=false'.
apt install libgstreamer1.0-dev

# ERROR: Problem encountered: Remoting plugin requires gstreamer-allocators-1.0 which was not found. If you rather not build this, set '-Dremoting=false'
apt install libgstreamer-plugins-base1.0-dev

# ERROR: Problem encountered: weston-editor requires 'pangocairo' which was not found. "fb3"
 -Ddemo-clients=false

# ERROR: Problem encountered: weston-simple-dmabuf-egl requires glesv2 which was not found
 -Dsimple-clients=im
 
# ERROR: Problem encountered: simple-dmabuf-drm is configured to use libdrm_etnaviv but it was not found.
-Dsimple-dmabuf-drm=intel

**Run Weston**: `weston`