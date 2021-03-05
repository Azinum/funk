# config.mk

CC=gcc

PROG=funk

BUILD_DIR=build

OUT_DIR=output

INSTALL_DIR=/usr/local/bin

SRC=${wildcard src/*.c}

INC_DIR=include

INC=${wildcard ${INC_DIR}/*.h}

FLAGS=-o ${BUILD_DIR}/${PROG} -I${INC_DIR} -Wall -O2
