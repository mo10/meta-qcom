inherit bin_package

DESCRIPTION = "Shared scripts for rootfs and recoveryfs"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/\
${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"
PROVIDES = "shared-scripts"
MY_PN = "shared-scripts"
PR = "r0"
INSANE_SKIP_${PN} = "ldflags"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
SRC_URI="file://adbd \
         file://recoverymount \
         file://log_rename.sh \
         file://logcat \
         file://rootfsmount \
         file://ncm_udhcpc "

S = "${WORKDIR}"


do_install() {
      install -d ${D}/etc/default
      install -d ${D}/etc/init.d
      install -d ${D}/etc/rcS.d
      install -d ${D}/bin

      install -m 0755 ${S}/adbd ${D}/etc/init.d/
      install -m 0755  ${S}/recoverymount ${D}/bin
      install -m 0755  ${S}/logcat ${D}/bin
      install -m 0755  ${S}/rootfsmount ${D}/bin
      install -m 0755  ${S}/log_rename.sh ${D}/etc/init.d/
      install -m 0755  ${S}/ncm_udhcpc ${D}/etc/init.d/
      ln -sf -r ${D}/etc/init.d/log_rename.sh ${D}/etc/rcS.d/S11log_rename.sh

      touch ${D}/etc/default/adbd

}
