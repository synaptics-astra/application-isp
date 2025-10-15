#!/bin/bash

source build/header.rc
source build/chipset.rc
source build/utils.rc

oe_dir=${CONFIG_SYNA_SDK_PATH}/toolchain/oe/linux-x64/gcc-13.3.0-poky
cmake_args=""

BUILD_DIR=${CONFIG_SYNA_SDK_BUILD_PATH}/V4L2ISP
if [[ -d ${BUILD_DIR} ]];then
    # keep release build clean anytime
    rm -rf ${BUILD_DIR}
fi
mkdir -p ${BUILD_DIR}
v4l2isp_driver_builddir=${CONFIG_SYNA_SDK_PATH}/application/v4l2isp/driver/build

function build_module()
{
    cmake_args+=" -DCMAKE_BUILD_TYPE=RELEASE
                 -DDEBUG_OPTMIZE_LEVEL=2
		 -DCMAKE_INSTALL_PREFIX=${v4l2isp_driver_builddir}"

    v4l2isp_module=$1
    if [ "${v4l2isp_module}" = "driver" ]; then
	mkdir -p ${v4l2isp_driver_builddir}
	pushd ${v4l2isp_driver_builddir}
    else
        mkdir -p ${BUILD_DIR}/${v4l2isp_module}
        pushd ${BUILD_DIR}/${v4l2isp_module}
    fi

    cmake -G Ninja ${CONFIG_SYNA_SDK_PATH}/application/v4l2isp/$1 ${cmake_args}
    ninja -j16
    cmake --install ${v4l2isp_driver_builddir}

    popd
}

#Build for release
for syna_chip_name in ${syna_chip_name_set[*]}
do
    if [ ${syna_chip_name} != "dolphin" ];then
	continue
    fi

    source build/chipparam.rc $syna_chip_name
    toolchain_array=()
    count=$(get_target_count)
    for ((index = 0; index != count; index++));
    do
	target_list=$(get_target_list_by_index $index)
	IFS=',' read cur_runtime cur_sysroot cur_toolchain cur_toolchain_path <<< "$target_list"
	# not build for the same toolchain
	if [[ " ${toolchain_array[@]} " =~ " ${cur_toolchain} " ]]; then
            continue
	fi

	# only build for poky
	if [ ${cur_toolchain} != "aarch64-poky-linux-" ]; then
	    echo "Skip v4l2isp for ${cur_toolchain}"
	    continue
	fi

	echo "Build v4l2isp for ${cur_toolchain}"
	source build/module/toolchain/${cur_toolchain}.rc
	source ${oe_dir}/environment-setup-cortexa73-poky-linux
	build_module driver
	export TUNE_EDITION_OPTION_DRIVER_UNITS=1
        export TUNE_EDITION_OPTION_FILESYSTEM=1
        export TUNE_EDITION_OPTION_OS=1
        export TUNE_EDITION_OPTION_TRANSMIT_HTTP=1
        export TUNE_EDITION_OPTION_TRANSMIT_RTMP=0
        export TUNE_EDITION_OPTION_TRANSMIT_TCP=1
        export TUNE_EDITION_OPTION_TRANSMIT_UART=1
        export TUNE_EDITION_OPTION_XML=1
	build_module t_frameworks
	build_module mediacontrol
    done
done

#Copy result for release
v4l2isp_rel_dir=${CONFIG_SYNA_SDK_REL_PATH}/application/v4l2isp
if [[ -d ${v4l2isp_rel_dir} ]];then
    rm -rf ${v4l2isp_rel_dir}
fi
mkdir -p ${v4l2isp_rel_dir}
#Release source code
rsync -az ${CONFIG_SYNA_SDK_PATH}/application/v4l2isp/driver/isi/sensor_drv ${v4l2isp_rel_dir}
mkdir ${v4l2isp_rel_dir}/sensor_drv/include
cp -r ${CONFIG_SYNA_SDK_PATH}/application/v4l2isp/driver/isi/include_priv/*.h ${v4l2isp_rel_dir}/sensor_drv/include
mv ${v4l2isp_rel_dir}/sensor_drv/CMakeLists_release.txt  ${v4l2isp_rel_dir}/sensor_drv/CMakeLists.txt
rsync -az ${CONFIG_SYNA_SDK_PATH}/application/v4l2isp/qt_camera_app ${v4l2isp_rel_dir}
rsync -az ${CONFIG_SYNA_SDK_PATH}/application/v4l2isp/mediacontrol/daemon ${v4l2isp_rel_dir}
cp -r ${CONFIG_SYNA_SDK_PATH}/application/v4l2isp/mediacontrol/tools/isp_video_test ${v4l2isp_rel_dir}/daemon
mv ${v4l2isp_rel_dir}/daemon/CMakeLists_release.txt ${v4l2isp_rel_dir}/daemon/CMakeLists.txt

#Release head files
mkdir ${v4l2isp_rel_dir}/sensor_drv/include/common
cp ${v4l2isp_driver_builddir}/generated/RELEASE/include/common/buf_defs.h ${v4l2isp_rel_dir}/sensor_drv/include/common/
cp ${v4l2isp_driver_builddir}/generated/RELEASE/include/common/mipi.h ${v4l2isp_rel_dir}/sensor_drv/include/common/
cp ${v4l2isp_driver_builddir}/generated/RELEASE/include/common/misc.h ${v4l2isp_rel_dir}/sensor_drv/include/common/
cp ${v4l2isp_driver_builddir}/generated/RELEASE/include/common/return_codes.h ${v4l2isp_rel_dir}/sensor_drv/include/common/
mkdir ${v4l2isp_rel_dir}/sensor_drv/include/ebase
cp ${v4l2isp_driver_builddir}/generated/RELEASE/include/ebase/builtins.h ${v4l2isp_rel_dir}/sensor_drv/include/ebase/
cp ${v4l2isp_driver_builddir}/generated/RELEASE/include/ebase/dct_assert.h ${v4l2isp_rel_dir}/sensor_drv/include/ebase/
cp ${v4l2isp_driver_builddir}/generated/RELEASE/include/ebase/linux_compat.h ${v4l2isp_rel_dir}/sensor_drv/include/ebase/
cp ${v4l2isp_driver_builddir}/generated/RELEASE/include/ebase/trace.h ${v4l2isp_rel_dir}/sensor_drv/include/ebase/
cp ${v4l2isp_driver_builddir}/generated/RELEASE/include/ebase/types.h ${v4l2isp_rel_dir}/sensor_drv/include/ebase/
mkdir ${v4l2isp_rel_dir}/sensor_drv/include/hal
cp ${v4l2isp_driver_builddir}/generated/RELEASE/include/hal/hal_api.h ${v4l2isp_rel_dir}/sensor_drv/include/hal/
cp ${v4l2isp_driver_builddir}/generated/RELEASE/include/hal/hal_common.h ${v4l2isp_rel_dir}/sensor_drv/include/hal/
mkdir ${v4l2isp_rel_dir}/sensor_drv/include/oslayer
cp ${v4l2isp_driver_builddir}/generated/RELEASE/include/oslayer/oslayer.h ${v4l2isp_rel_dir}/sensor_drv/include/oslayer/
cp ${v4l2isp_driver_builddir}/generated/RELEASE/include/oslayer/oslayer_linux.h ${v4l2isp_rel_dir}/sensor_drv/include/oslayer/
rsync -az ${v4l2isp_driver_builddir}/generated/RELEASE/include/isi ${v4l2isp_rel_dir}/sensor_drv/include

#Release executable files and libraries
mkdir -p ${v4l2isp_rel_dir}/bin
mkdir -p ${v4l2isp_rel_dir}/lib
cp -r ${BUILD_DIR}/t_frameworks/t_server_c/t_server_c* ${v4l2isp_rel_dir}/bin
cp ${v4l2isp_driver_builddir}/generated/RELEASE/lib/libisp_driver.so ${v4l2isp_rel_dir}/lib
cp ${v4l2isp_driver_builddir}/generated/RELEASE/lib/libcam_common.so ${v4l2isp_rel_dir}/lib
cp -r ${BUILD_DIR}/t_frameworks/t_common_c/libt_common_c.so* ${v4l2isp_rel_dir}/lib
cp -r ${BUILD_DIR}/t_frameworks/t_json_c/libt_json_c.so* ${v4l2isp_rel_dir}/lib
cp -r ${BUILD_DIR}/t_frameworks/t_database_c/libt_database_c.so* ${v4l2isp_rel_dir}/lib
cp -r ${BUILD_DIR}/t_frameworks/t_driver_c/libt_driver_c.so* ${v4l2isp_rel_dir}/lib
cp -r ${BUILD_DIR}/t_frameworks/t_shell_c/libt_shell_c.so* ${v4l2isp_rel_dir}/lib
cp -r ${BUILD_DIR}/t_frameworks/t_transmit_c/libt_transmit_c.so* ${v4l2isp_rel_dir}/lib
cp ${BUILD_DIR}/mediacontrol/media_pipeline/media_device/libmc_media_device.so ${v4l2isp_rel_dir}/lib
