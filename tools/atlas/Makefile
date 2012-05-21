CXX=g++ #temp hack

SRC = texture-atlas.cpp \

OBJS+=$(filter %.o,$(SRC:.cpp=.o))
TARGET=texture-atlas

all: texture-atlas

%.o: %.cpp
	@rm -f $@ 
	$(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS) -lIL -lILU -ltinyxml

clean:
	@-rm $(TARGET) $(OBJS)
