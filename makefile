CC=g++
GCC_OPTIONS=-c -Wall -pedantic -Wno-deprecated
FN=Map

ARCH=$(shell uname)
ifeq ($(ARCH), Linux)
GL_OPTIONS=-lGLEW -lglut -lGL  -lXmu -lX11  -lm
else
GL_OPTIONS=-framework OpenGL -framework GLUT 
endif

OPTIONS=$(GCC_OPTIONS) $(GL_OPTIONS)
SOURCES=$(FN).cpp ../include/initShader.cpp 
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=$(FN)

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(GL_OPTIONS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(GCC_OPTIONS) $< -o $@

clean:
	rm *.o $(FN) ../include/initShader.o
