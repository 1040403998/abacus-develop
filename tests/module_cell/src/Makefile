# This is the Makefile of ABACUS-cell API

#==========================
# Compiler information 
#==========================
CPLUSPLUS     = icpc
CPLUSPLUS_MPI = mpiicpc
FFTW_DIR = /home/liwenfei/codes/FFTW3
OBJ_DIR = cell_obj
NP      = 8

#==========================
# LIBS and INCLUDES
#==========================
LIBS = -lifcore -lm -lpthread ${FFTW_LIB}

#==========================
# FFTW package needed 
#==========================
HONG_FFTW        = -D__FFTW3
FFTW_INCLUDE_DIR = ${FFTW_DIR}/include
FFTW_LIB_DIR     = ${FFTW_DIR}/lib
FFTW_LIB         = -L${FFTW_LIB_DIR} -lfftw3 -Wl,-rpath=${FFTW_LIB_DIR}

#==========================
# OPTIMIZE OPTIONS
#==========================
INCLUDES = -I. -Icommands -I${FFTW_INCLUDE_DIR}

# -pedantic turns off more extensions and generates more warnings
# -xHost generates instructions for the highest instruction set available on the compilation host processor
OPTS = ${INCLUDES} -Ofast -std=c++11 -simd -march=native -xHost -m64 -qopenmp -Werror -Wall -pedantic -g

include Makefile.Objects

VPATH=../../../source/module_base\
:../../../source/src_io\
:../../../source/src_pw\
:../../../source/src_parallel\
:../../../source/module_orbital\
:../../../source/module_neighbor\
:../../../source/module_cell\
:../../../source\
:../../module_orb/src/
:./\

#==========================
# Define HONG
#==========================
#HONG= -DMETIS -DMKL_ILP64 -D__CELL -D__NORMAL ${HONG_FFTW} -D__ORBITAL
HONG= -DMETIS -DMKL_ILP64 -D__CELL -D__NORMAL -D__LCAO ${HONG_FFTW} 
HONG_MPI= -DMETIS -DMKL_ILP64 -D__MPI -D__CELL -D__NORMAL -D__LCAO

FP_OBJS_0=main.o\
$(OBJS_MAIN)\
$(OBJS_IO)\
$(OBJS_PW)\
$(OBJS_BASE)\
$(OBJS_CELL)\
$(OBJS_ORB)\
$(OBJS_NEIGHBOR)\
#$(OBJS_PARALELL)\

FP_OBJS=$(patsubst %.o, ${OBJ_DIR}/%.o, ${FP_OBJS_0})

#==========================
# MAKING OPTIONS
#==========================
cell : 
	@ make init
	@ make -j $(NP) serial

init :
	@ if [ ! -d $(OBJ_DIR) ]; then mkdir $(OBJ_DIR); fi
	@ if [ ! -d $(OBJ_DIR)/README ]; then echo "This directory contains all of the .o files" > $(OBJ_DIR)/README; fi

parallel : ${FP_OBJS} 
	${CPLUSPLUS_MPI} ${OPTS} $(FP_OBJS) ${LIBS} -o ${VERSION}.x 

serial : ${FP_OBJS} 
	${CPLUSPLUS} ${OPTS} $(FP_OBJS) ${LIBS} -o ${VERSION}.x 

#==========================
# rules
#==========================
${OBJ_DIR}/%.o:%.cpp
	${CPLUSPLUS} ${OPTS} -c ${HONG} $< -o $@

.PHONY:clean
clean:
	@ if [ -d $(OBJ_DIR) ]; then rm -rf $(OBJ_DIR); fi
	@ if [ -e ${VERSION}.x ]; then rm -f ${VERSION}.x; fi
