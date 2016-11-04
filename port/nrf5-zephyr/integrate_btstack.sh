#/bin/sh

ZEPHYR_BASE=../../..

# add btstack folder to net/Makefile
MAKEFILE_ADD_ON='obj-$(CONFIG_BTSTACK) += btstack/'
NET_MAKEFILE=${ZEPHYR_BASE}/net/Makefile
grep -q -F btstack ${NET_MAKEFILE} || echo ${MAKEFILE_ADD_ON} >> ${NET_MAKEFILE}

# add BTstack KConfig to net/Kconfig
NET_KCONFIG=${ZEPHYR_BASE}/net/Kconfig
grep -q -F btstack ${NET_KCONFIG} || cat net-Kconfig.patch | patch -d ${ZEPHYR_BASE}/net Kconfig

# create net/btstack
mkdir -p ${ZEPHYR_BASE}/net/btstack

# copy sources
rsync -a ../../src/ ${ZEPHYR_BASE}/net/btstack

# copy btstack_config.h
rsync btstack_config.h ${ZEPHYR_BASE}/net/btstack

# copy Kconfig
rsync Kconfig ${ZEPHYR_BASE}/net/btstack

# copy Makefiles
rsync Makefile.src 	        ${ZEPHYR_BASE}/net/btstack/Makefile
rsync Makefile.ble 	        ${ZEPHYR_BASE}/net/btstack/ble/Makefile
rsync Makefile.gatt-service ${ZEPHYR_BASE}/net/btstack/ble/gatt-service/Makefile

