NAME = jpanel
CFLAGS = -std=c++14 `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0 x11`

all:
	c++ $(CFLAGS) main.cpp -o $(NAME)

clean:
	rm *.o $(NAME)