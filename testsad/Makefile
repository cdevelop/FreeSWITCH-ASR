KALDI=/data/kaldi

SOURCES = main.c

OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))

LIB = -L../libsad -lsad 
DEFINE = 




%.o: %.c
	gcc -I../libsad ${DEFINE} -fPIC -O2 -c  $< -o $@

%.o: %.cpp
	g++ -I../libsad ${DEFINE} -fPIC -O2 -std=c++11 -c  $< -o $@

	
testsad.so: $(OBJS)
	gcc  -o testsad $(OBJS)  ${LIB} -Wl,-rpath=../libsad





clean : 
	rm -rf $(OBJS) testsad
	
