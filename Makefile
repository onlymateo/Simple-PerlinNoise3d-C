##
## EPITECH PROJECT, 2023
## my_paint
## File description:
## make
##

SRC	=	$(shell ls ./*.c)

NAMEH	=	my.h

WINDOW	=	-lcsfml-window

GRAPH	=	-lcsfml-graphics

SYS	=	-lcsfml-system

AUD	=	-lcsfml-audio

NET	=	-lcsfml-network

all:
	gcc $(SRC) $(NET) $(AUD) $(SYS) $(GRAPH) $(WINDOW) -lm -lGL -lGLU -lglut -ffast-math


clean:

fclean: clean
	rm -f *.a

re:	fclean all

