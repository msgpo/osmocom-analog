#include <stdio.h>
#include <string.h>
#include "../libmobile/image.h"

const char *image[] = {
	"",
	"                                                      @R|   |",
	"           @Y|                                         @R/|===|\\",
	"         @Y-=@wO@Y=-                                      @R/ |\\ /| \\               @w~",
	"           @Y|                    @wAMPS               @R/  | X |  \\      @w~",
	"                                                  @R/   |===|   \\",
	"                                                 @R/    |   |\\    \\",
	"                                                @R/    /|   | \\    \\       @w~",
	"              @R|--|                            /    /  |===|  \\    \\",
	"             @R/|\\/|\\                          /    /   |   |    \\    \\",
	"            @R/ |--| \\                        /    /    |   |     \\    \\",
	"           @R/  |  |\\  \\                    /    /      |===|       \\    \\",
	"@g__        @R/  /|--|  \\  \\                /    /        |   |         \\    \\",
	"@g  \\_    @R/  /  |  |    \\  \\            /   /           |   |           \\    \\",
	"@g    \\ @R/  /    |  |       \\  \\______/   /              |   |             \\|   \\|",
	"@g     \\@R=========================================================================",
	"@g      \\_______@R|/\\|@g______        _@G***@g_@G*@g__@G**@g_@G***@g___@G**@g_@G*@g_@R| X |              @R|MMMM|",
	"@g         \\__  @R|\\/|@g      \\______/@G*  **   *  * * **  *  @R|/ \\|@g______@y________@R|MMMM|",
	"@y _____@g      \\_@R|/\\|@G *    *     *  *  ** * *  * *  *  * @R|\\ /|@G*@y____/  ~    ~ ~   ~",
	"@y/ ~  ~\\@b_______@RI@b__@RI@b_________@G*@b______@G*@b_@G*@b___@G*@b__@G*@b___@G*@b__@G*@b_@G*@b_@R| X |@y/   ~  ~   _________",
	"@y_______\\@b _      _  _    _       _ _   _ _    _    _   @R|/@y_@R\\@y/  ~ ______/",
	"@b _         _         _        _   _      _      _   @y_/~ ~ ____/  ~       ~   ~",
	"@b_  @y_@b    _   _          _        _         _        @y/_____/     ~   ____________",
	"@y__/~\\______________@b  _     _         _        _  @y_/ ~ ~ __________/  ~  ~   ~",
	"@y ~      ~   ~      \\____________________________/      /  ~   ~      ~    ~  ~",
	"@w",
	NULL
};

void print_image(void)
{
	int i, j;

	for (i = 0; image[i]; i++) {
		for (j = 0; j < (int)strlen(image[i]); j++) {
			if (image[i][j] == '@') {
				j++;
				switch(image[i][j]) {
				case 'R': /* bright red */
					printf("\033[1;31m");
					break;
				case 'g': /* gray */
					printf("\033[0;37m");
					break;
				case 'G': /* green */
					printf("\033[0;32m");
					break;
				case 'w': /* white */
					printf("\033[1;37m");
					break;
				case 'b': /* blue */
					printf("\033[0;34m");
					break;
				case 'y': /* yellow */
					printf("\033[0;33m");
					break;
				case 'Y': /* bright yellow */
					printf("\033[1;33m");
					break;
				}
			} else
				printf("%c", image[i][j]);
		}
		printf("\n");
	}
	printf("\033[0;39m");
}

