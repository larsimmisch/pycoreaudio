PY_INCLUDE=$(shell python -c 'import sysconfig as sc; print sc.get_config_var("INCLUDEDIR")')

PY_LIBDIR=$(shell python -c 'import sysconfig as sc; print sc.get_config_var("LIBPL")')

PY_LIB=$(shell python -c 'import sysconfig as sc; print sc.get_config_var("LIBRARY")[3:-2]')

.PHONY: all build test clean

all: build

# all: coreaudio.so caplaymu

# coreaudio.o: coreaudio.c
# 	gcc -g -c -I$(PY_INCLUDE) -o $@ $<

# coreaudio.so: coreaudio.o
# 	gcc -g -dynamiclib -o $@ $< -L$(PY_LIBDIR) -l$(PY_LIB) -framework CoreServices -framework CoreAudio -framework AudioUnit 

caplaymu: caplaymu.c
	@gcc -g -o $@ $< -framework CoreServices -framework CoreAudio -framework AudioUnit 

clean:
	@rm -f *.o *.so caplaymu 
	@rm -rf build caplaymu.dSYM

test:
	@python3 play.py bimbam.wav

build:
	@python3 setup.py build