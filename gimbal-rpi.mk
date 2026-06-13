################################################################################
#
# gimbal-rpi
#
################################################################################

GIMBAL_RPI_SITE = $(BR2_EXTERNAL_TIGERS_PATH)/package/gimbal-rpi
GIMBAL_RPI_SITE_METHOD = local
GIMBAL_RPI_DEPENDENCIES = systemd libgpiod2 eigen boost
GIMBAL_RPI_OVERRIDE_SRCDIR_RSYNC_EXCLUSIONS = --include .git
GIMBAL_RPI_SUPPORTS_IN_SOURCE_BUILD = NO

$(eval $(cmake-package))
