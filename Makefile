monitor:
	cc  -c -Wall monitor.c
	cc  -c -Wall util.c
	cc -o monitor monitor.o util.o -lcurl -lreadline

install: monitor
	strip monitor
	cp monitor /usr/local/bin
