# makefile

include config.mk

all: prepare compile run

prepare:
	@mkdir -p ${BUILD_DIR}

compile: ${SRC}
	${CC} ${SRC} ${FLAGS}

run:
	./${BUILD_DIR}/${PROG}

install:
	${CC} ${SRC} ${FLAGS}
	chmod o+x ${BUILD_DIR}/${PROG}
	cp ${BUILD_DIR}/${PROG} ${INSTALL_DIR}/${PROG}

uninstall:
	rm ${INSTALL_DIR}/${PROG}

clean:
	rm -dr ${BUILD_DIR}
