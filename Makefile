CXXFLAGS =	-O3 -Wall -fmessage-length=0

OBJS =		develweb.o

LIBS =

TARGET =	wappbuild

DESTDIR = /usr/local/bin

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET) example_page

debug: 
	@$(MAKE) all "CXXFLAGS=-O0 -g3 -Wall" 

clean:
	rm -f $(OBJS) $(TARGET)
	@$(MAKE) -C example clean
	
example_page: 
	@$(MAKE) -C example all
	

$(DESTDIR):
	@mkdir -p $(DESTDIR)

install: | $(DESTDIR)
	@cp -vut $(DESTDIR) $(TARGET) 
	
dist-clean:
	@rm -vf $(DESTDIR)/`basename $(TARGET)`

