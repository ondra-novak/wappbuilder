CXXFLAGS =	-O0 -g3 -Wall -fmessage-length=0

OBJS =		develweb.o

LIBS =

TARGET =	webpacker

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
