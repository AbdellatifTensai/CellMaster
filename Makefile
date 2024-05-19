CFLAGS= -Wall -Wextra -pedantic
INCLUDE= `pkg-config raylib --cflags`
LINK= `pkg-config raylib --libs` -lm

main:
	gcc $(CFLAGS) -O3 $(INCLUDE) src/minicel.c -o main $(LINK)

debug:
	gcc $(CFLAGS) -DDEBUG -g $(INCLUDE) src/minicel.c -o main $(LINK)
