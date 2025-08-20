CFLAGS = -Wall -O2 -std=c++11

CC = c++

#CC = clang++ -g

OBJSDIR = objs
#$(shell mkdir -p $(OBJSDIR))

DEPS = main.h string__.h classes.h bytes_array.h

OBJS = $(OBJSDIR)/http2_server.o \
	$(OBJSDIR)/huffman_code.o \
	$(OBJSDIR)/fcgi.o \
	$(OBJSDIR)/cgi.o \
	$(OBJSDIR)/scgi.o \
	$(OBJSDIR)/http2.o \
	$(OBJSDIR)/event_handler.o \
	$(OBJSDIR)/config.o \
	$(OBJSDIR)/functions.o \
	$(OBJSDIR)/classes.o \
	$(OBJSDIR)/ssl.o \
	$(OBJSDIR)/accept_connect.o \
	$(OBJSDIR)/socket.o \
	$(OBJSDIR)/percent_coding.o \
	$(OBJSDIR)/index.o \
	$(OBJSDIR)/log.o \

http2_server: $(OBJS)
	$(CC) $(CFLAGS) -o $@  $(OBJS) -lpthread -L/usr/local/lib/ -L/usr/local/lib64/ -lssl -lcrypto

$(OBJSDIR)/http2_server.o: http2_server.cpp $(DEPS)
	$(CC) $(CFLAGS) -c http2_server.cpp -o $@

$(OBJSDIR)/huffman_code.o: huffman_code.cpp $(DEPS)
	$(CC) $(CFLAGS) -c huffman_code.cpp -o $@

$(OBJSDIR)/fcgi.o: fcgi.cpp $(DEPS)
	$(CC) $(CFLAGS) -c fcgi.cpp -o $@

$(OBJSDIR)/ssl.o: ssl.cpp $(DEPS)
	$(CC) $(CFLAGS) -c ssl.cpp -o $@

$(OBJSDIR)/scgi.o: scgi.cpp $(DEPS)
	$(CC) $(CFLAGS) -c scgi.cpp -o $@

$(OBJSDIR)/cgi.o: cgi.cpp $(DEPS)
	$(CC) $(CFLAGS) -c cgi.cpp -o $@

$(OBJSDIR)/config.o: config.cpp $(DEPS)
	$(CC) $(CFLAGS) -c config.cpp -o $@

$(OBJSDIR)/http2.o: http2.cpp $(DEPS)
	$(CC) $(CFLAGS) -c http2.cpp -o $@

$(OBJSDIR)/classes.o: classes.cpp $(DEPS)
	$(CC) $(CFLAGS) -c classes.cpp -o $@

$(OBJSDIR)/accept_connect.o: accept_connect.cpp $(DEPS) 
	$(CC) $(CFLAGS) -c accept_connect.cpp -o $@

$(OBJSDIR)/event_handler.o: event_handler.cpp $(DEPS)
	$(CC) $(CFLAGS) -c event_handler.cpp -o $@

$(OBJSDIR)/socket.o: socket.cpp $(DEPS)
	$(CC) $(CFLAGS) -c socket.cpp -o $@

$(OBJSDIR)/percent_coding.o: percent_coding.cpp $(DEPS)
	$(CC) $(CFLAGS) -c percent_coding.cpp -o $@

$(OBJSDIR)/functions.o: functions.cpp $(DEPS)
	$(CC) $(CFLAGS) -c functions.cpp -o $@

$(OBJSDIR)/index.o: index.cpp $(DEPS)
	$(CC) $(CFLAGS) -c index.cpp -o $@

$(OBJSDIR)/log.o: log.cpp $(DEPS)
	$(CC) $(CFLAGS) -c log.cpp -o $@

clean:
	rm -f http2_server
	rm -f $(OBJSDIR)/*.o
