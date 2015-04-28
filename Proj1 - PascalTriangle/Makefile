CC      = gcc -Wall
ALL     = pascal w
OBJECTS = $(addsuffix .o, $(ALL)) err.o

all: $(ALL)

$(ALL): %: %.o err.o
	$(CC) $^ -o $@

$(OBJECTS): %.o: %.c err.h
	$(CC) -c $<

clean:
	rm -f $(ALL) $(OBJECTS)