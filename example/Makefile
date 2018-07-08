
TARGET=debug
FILES:=example.html
LANGFILE:=en.lang 

-include $(FILES:.html=.d)


all : $(FILES)

ifeq ($(TARGET),debug)
%.html : %.page
		../webpacker -i "$<" -o "$@" -d "$(@:.html=.d)" -l $(LANGFILE)
else
%.html : %.page
		../webpacker -i "$<" -o "$@" -d "$(@:.html=.d)" -l $(LANGFILE) -c "$(@:.html=)"
%.css : %.html
	
%.js : %.html

endif		

clean:
	rm -f $(FILES)
	rm -f $(FILES:.html=.css)
	rm -f $(FILES:.html=.js)
	rm -f $(FILES:.html=.d)
	