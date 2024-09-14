CC=gcc
CFLAGS=-fno-stack-protector -fPIC -DPIC -D_FORTIFY_SOURCE=0 -g -O0

all: ckpt restart readckpt sample libckpt.so

sample: sample.c
	 
	${CC} ${CFLAGS} -o $@ $<

ckpt: ckpt.c
	${CC} ${CFLAGS} -o $@ $<

libckpt.so: libckpt.c
	${CC} ${CFLAGS} -shared -o $@ $<

restart:
	gcc ${CFLAGS} -static -o restart restart.c \
       -Wl,-Ttext-segment=5000000 -Wl,-Tdata=5100000 -Wl,-Tbss=5200000

readckpt: readckpt.c
	${CC} ${CFLAGS} -o $@ $<

check: clean all
	rm -f myckpt
	./ckpt ./sample & sleep 2
	cat /proc/`pgrep -n sample`/maps
	#echo "\n"
	#cat /proc/`pgrep -n restart`/maps
	pkill -12 sample & sleep 1
	pkill -9 sample
	./restart

clean: 
	rm -f sample ckpt restart readckpt libckpt.so myckpt
