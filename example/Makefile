
PAKFLAGS:=
FILES:=example.html

-include $(FILES:.html=.html.d)


all : $(FILES)



%.html : %.page
		../webpacker -i "$<" -o "$@" -d "$@.d"
		

		
