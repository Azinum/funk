# config.mk

CC=gcc

PROG=funk

BUILD_DIR=build

OUT_DIR=output

INSTALL_DIR=/usr/local/bin

SRC=${wildcard src/*.c}

INC_DIR=include

INC=${wildcard ${INC_DIR}/*.h}

LIBS=-lreadline

FLAGS=-o ${BUILD_DIR}/${PROG} ${LIBS} -I${INC_DIR} -O2 -Wall
