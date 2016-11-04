#/bin/sh

ZEPHYR_BASE=../../..

# add btstack folder to subsys/Makefile
MAKEFILE_ADD_ON='obj-$(CONFIG_BTSTACK) += btstack/'
NET_MAKEFILE=${ZEPHYR_BASE}/subsys/Makefile
grep -q -F btstack ${NET_MAKEFILE} || echo ${MAKEFILE_ADD_ON} >> ${NET_MAKEFILE}

# add BTstack KConfig to net/Kconfig
SUBSYS_KCONFIG=${ZEPHYR_BASE}/subsys/Kconfig
grep -q -F btstack ${SUBSYS_KCONFIG} || echo 'source "subsys/btstack/Kconfig"' >> ${SUBSYS_KCONFIG}

# create net/btstack
mkdir -p ${ZEPHYR_BASE}/subsys/btstack

# copy sources
rsync -a ../../src/ ${ZEPHYR_BASE}/subsys/btstack

# copy btstack_config.h
rsync btstack_config.h ${ZEPHYR_BASE}/subsys/btstack

# copy Kconfig
rsync Kconfig ${ZEPHYR_BASE}/subsys/btstack

# copy Makefiles
rsync Makefile.src 	        ${ZEPHYR_BASE}/subsys/btstack/Makefile
rsync Makefile.ble 	        ${ZEPHYR_BASE}/subsys/btstack/ble/Makefile
rsync Makefile.gatt-service ${ZEPHYR_BASE}/subsys/btstack/ble/gatt-service/Makefile

