SRC_DIR=./../../../Source
COMMON_SRC_DIR=${SRC_DIR}/Common
TEST_SRC_DIR=test

BUILD_DIR=./build
BIN_DIR=${BUILD_DIR}/bin
OBJ_DIR=${BUILD_DIR}/obj
LIB_DIR=${BUILD_DIR}/lib
DIR_LIST=${BUILD_DIR} ${OBJ_DIR} ${LIB_DIR} ${BIN_DIR}

INCLUDE= -I${SRC_DIR} -I${COMMON_SRC_DIR}
COMPILE_FLAGS=${CFLAGS} -std=c++14 -fPIC -Os -w -pedantic
LINK_FLAGS=${LFLAGS} -lpthread -ldl

LIB_COMMON=${LIB_DIR}/libcommon.a #${LIB_DIR}/libcommon.so
COMMON_OBJ=${OBJ_DIR}/CriticalSection.o ${OBJ_DIR}/DateTimeClass.o ${OBJ_DIR}/FileClass.o ${OBJ_DIR}/IniFileClass.o ${OBJ_DIR}/LogClass.o ${OBJ_DIR}/md5.o ${OBJ_DIR}/MemoryManagement.o ${OBJ_DIR}/MemTest.o ${OBJ_DIR}/path.o ${OBJ_DIR}/Platform.o ${OBJ_DIR}/StdString.o ${OBJ_DIR}/SyncEvent.o ${OBJ_DIR}/Thread.o ${OBJ_DIR}/Trace.o ${OBJ_DIR}/Util.o
COMMON_OBJ_C=${COMMON_OBJ}

TEST_EXE=${BIN_DIR}/test
TEST_OBJ=${OBJ_DIR}/test.o
TEST_OBJ_C=${TEST_OBJ}

.PHONY: test

all: ${LIB_COMMON}
create_dirs: ${DIR_LIST}

${DIR_LIST}:
	@test -d $@ || mkdir -p $@

${COMMON_OBJ}: ${COMMON_SRC_DIR}/CriticalSection.h ${COMMON_SRC_DIR}/DateTimeClass.h ${COMMON_SRC_DIR}/FileClass.h ${COMMON_SRC_DIR}/IniFileClass.h ${COMMON_SRC_DIR}/LogClass.h ${COMMON_SRC_DIR}/md5.h ${COMMON_SRC_DIR}/MemoryManagement.h ${COMMON_SRC_DIR}/MemTest.h ${COMMON_SRC_DIR}/path.h ${COMMON_SRC_DIR}/Platform.h ${COMMON_SRC_DIR}/SmartPointer.h ${COMMON_SRC_DIR}/StdString.h ${COMMON_SRC_DIR}/stdtypes.h ${COMMON_SRC_DIR}/SyncEvent.h ${COMMON_SRC_DIR}/Thread.h ${COMMON_SRC_DIR}/Trace.h ${COMMON_SRC_DIR}/TraceModulesCommon.h ${COMMON_SRC_DIR}/Util.h

${COMMON_OBJ_C}: ${OBJ_DIR}/%.o: ${COMMON_SRC_DIR}/%.cpp | create_dirs
	${CXX} ${INCLUDE} ${COMPILE_FLAGS} -c -o $@ $<

${LIB_DIR}/libcommon.a: ${COMMON_OBJ_C}
	@rm -f $@ && ${AR} ruv $@ ${COMMON_OBJ_C}

${LIB_DIR}/libcommon.so: ${COMMON_OBJ_C}
	${CXX} -shared -o $@ ${COMMON_OBJ_C} ${LINK_FLAGS}

${TEST_OBJ_C}: ${OBJ_DIR}/%.o: ${TEST_SRC_DIR}/%.cpp
	echo ${INCLUDE}
	${CXX} ${INCLUDE} ${COMPILE_FLAGS} -c -o $@ $<

${TEST_EXE}: ${LIB_COMMON} | ${TEST_OBJ_C}
	${CXX} -o $@ ${TEST_OBJ_C} -L${LIB_DIR} -lcommon ${LINK_FLAGS}

test: ${TEST_EXE}
	@cd ${BUILD_DIR}/bin && echo "\n\nStarting test..." && ./test
clean:
	@rm -rf ${BUILD_DIR} > /dev/null && echo "all clean"



