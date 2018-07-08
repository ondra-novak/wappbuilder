CXXFLAGS =	-O0 -g3 -Wall -fmessage-length=0

OBJS =		develweb.o

LIBS =

TARGET =	webpacker 

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET) example_page

clean:
	rm -f $(OBJS) $(TARGET)
	$(MAKE) -C example clean
	
example_page: 
	$(MAKE) -C example all
	
