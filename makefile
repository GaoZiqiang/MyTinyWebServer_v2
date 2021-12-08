CC = gcc
CCC = g++
AR = ar
CFLAGS = -g -Wall
SO_CFLAGS = -fPIC
SO_LDFLAGS = -shared
AR_FLAGS = -cr
LIB_DIR = -L ../lib
LIBS = -lmysqlclient -lhiredis -lssl -lcrypto -lpthread
INC_DIR = -I tinyxml
LOG_DIR = log

APP_DIR = .
APP_NAME = main
APP_OBJ_DIR = obj

all: ${APP_DIR} ${APP_OBJ_DIR} ${APP_DIR}/${APP_NAME}

#for sbin
APP_OBJ = ${APP_OBJ_DIR}/main.o \
    ${APP_OBJ_DIR}/webserver.o \
	${APP_OBJ_DIR}/config.o \
	${APP_OBJ_DIR}/sql_connection_pool.o \
 	${APP_OBJ_DIR}/http_conn.o \
 	${APP_OBJ_DIR}/lst_timer.o \
 	${APP_OBJ_DIR}/log.o \
 	${APP_OBJ_DIR}/thread_pool.o \



${APP_DIR}:
	if [ ! -d ${APP_DIR} ]; then mkdir ${APP_DIR};  fi

${APP_OBJ_DIR}:
	if [ ! -d ${APP_OBJ_DIR} ]; then mkdir ${APP_OBJ_DIR};  fi


${LOG_DIR}:
	if [ ! -d ${LOG_DIR} ]; then mkdir ${LOG_DIR};  fi

${APP_DIR}/${APP_NAME}:${APP_OBJ}
	${CCC} -g -o $@  $(filter %.o, $^) ${LIB_DIR} ${LIBS}

${APP_OBJ_DIR}/%.o:%.cpp
	${CCC} -c ${CFLAGS}  ${INC_DIR} -o $@ $<

${APP_OBJ_DIR}/%.o:mysql/%.cpp
	${CCC} -c ${CFLAGS}  ${INC_DIR} -o $@ $<

${APP_OBJ_DIR}/%.o:http/%.cpp
	${CCC} -c ${CFLAGS}  ${INC_DIR} -o $@ $<

${APP_OBJ_DIR}/%.o:utils/%.cpp
	${CCC} -c ${CFLAGS}  ${INC_DIR} -o $@ $<

${APP_OBJ_DIR}/%.o:timer/%.cpp
	${CCC} -c ${CFLAGS}  ${INC_DIR} -o $@ $<

${APP_OBJ_DIR}/%.o:thread_pool/%.cpp
	${CCC} -c ${CFLAGS}  ${INC_DIR} -o $@ $<

clean:
	rm -rf ${APP_OBJ_DIR}
	rm -rf ${APP_DIR}/${APP_NAME}
