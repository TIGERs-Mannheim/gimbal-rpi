################################################################################
#
# ssl-tracking-cam
#
################################################################################

SSL_TRACKING_CAM_SITE = $(BR2_EXTERNAL_TIGERS_PATH)/package/ssl-tracking-cam
SSL_TRACKING_CAM_SITE_METHOD = local
SSL_TRACKING_CAM_DEPENDENCIES = systemd libgpiod2 protobuf eigen boost
SSL_TRACKING_CAM_OVERRIDE_SRCDIR_RSYNC_EXCLUSIONS = --include .git
SSL_TRACKING_CAM_SUPPORTS_IN_SOURCE_BUILD = NO

$(eval $(cmake-package))
