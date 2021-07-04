
TARGET = timeSync
SOURCE_FILES = ${wildcard ./*.cpp}
CC = g++
RM = rm -rf
LDFLAG = -lpthread -lmylog -lmydatastruct
LDFLAG += -L./lib/lib
LDFLAG += -L./extern/lib
INCLUDE = -I./lib/include
INCLUDE += -I./extern/lib/include
MACRO = 

OBJECTS = $(patsubst %.cpp,%.o, $(SOURCE_FILES))
#OBJECTS := $(foreach item,$(SOURCE_FILES),$(CC) -c $(item) -o $(patsubst %.c,%.o, $(item)) $(INCLUDE) $(MACRO))
#gcc -E  hello.c  -o hello.i
#PRECOMPILE_FILES := $(foreach item,$(SOURCE_FILES),$(CC) -E $(item) -o $(patsubst %.c,%.i, $(item)) $(INCLUDE) $(MACRO))
#COMPILE_FILES := $(foreach item,$(SOURCE_FILES),$(CC) -S $(item) -o $(patsubst %.c,%.s, $(item)) $(INCLUDE) $(MACRO))
#COMPILE_FILES = $(patsubst %.c,%.s, $(SOURCE_FILES))

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAG)

#gcc  -c  hello.s  -o  hello.o
$(OBJECTS):$(SOURCE_FILES)
	$(CC) -c $^ $(INCLUDE)

#gcc  -S  hello.i   -o  hello.s
#$(COMPILE_FILES):$(PRECOMPILE_FILES)
	#$(CC) -S $^

clean:
	$(RM) $(TARGET) $(OBJECTS)
