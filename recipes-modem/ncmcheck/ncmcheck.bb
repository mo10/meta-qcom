SUMMARY = "Small utility to check if NCM flag is set in the misc partition"
LICENSE = "MIT"
MY_PN = "ncmcheck"
RPROVIDES_${PN} = "ncmcheck"
PR = "r7"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://src/ncmcheck.c"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${LDFLAGS} -O2 src/ncmcheck.c -o ncmcheck
}

do_install() {
    install -d ${D}${bindir}

    install -m 0755 ${S}/ncmcheck ${D}${bindir}
}
