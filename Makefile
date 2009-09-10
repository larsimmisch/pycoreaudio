PY_INCLUDE=$(shell python -c 'import distutils.sysconfig as sc; print sc.get_python_inc()')

PY_LIBDIR=$(shell python -c 'import distutils.sysconfig as sc; print sc.get_config_var("LIBPL")')

PY_LIB=$(shell python -c 'import distutils.sysconfig as sc; print sc.get_config_var("LIBRARY")[3:-2]')

all: coreaudio.so caplaymu

coreaudio.o: coreaudio.c
	gcc -g -c -I$(PY_INCLUDE) -o $@ $<

coreaudio.so: coreaudio.o
	gcc -g -dynamiclib -o $@ $< -L$(PY_LIBDIR) -l$(PY_LIB) -framework CoreServices -framework CoreAudio -framework AudioUnit 

caplaymu: caplaymu.c
	gcc -g -o $@ $< -framework CoreServices -framework CoreAudio -framework AudioUnit 

clean:
	rm -f *.o *.so caplaymu