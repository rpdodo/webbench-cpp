OBJS=webtest.o socket.o
LIBS=

all:webtest

webtest:$(OBJS)
	g++ -g -o $@ $^ $(LIBS)

.cc.o:
	g++ -g -o $@ -c $<

clean:
	@rm -f $(OBJS) webtest
	@:>webtest.log
