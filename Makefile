CC:= gcc 
CFLAGS := -Wall -g 
TARGET += sh550

SRCS := $(wildcard *.c)

OBJS := $(patsubst %.c,%.o,$(SRCS))

all: $(TARGET)
	./$(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf $(TARGET) *.o

.PHONY: all clean
