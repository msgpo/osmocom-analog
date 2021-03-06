#include <stdio.h>
#include <string.h>
#include "../libmobile/image.h"

const char *image[] = {
	"@g",
	"",
	"",
	"           \\",
	"            \\",
	"             \\",
	"              \\_                 @wB-NETZ@g",
	"              \\ \\",
	"               \\_\\___",
	"               /  __ )",
	"              (__\\ _\\________",
	"                 / @G _______@g  )",
	"                / @G/        \\@g/",
	"               / @G/    @g______@G\\@g___",
	"              / @G/   @g/           )",
	"             (__@G\\  @g/           /    @w~@g",
	"                 @G\\@g/      ___  /",
	"                 /     /    \\/          @w~@g",
	"                (______\\     \\",
	"                        \\     \\",
	"                         \\     \\",
	"                          \\     \\",
	"              @w~@g            \\     \\",
	"                            \\     \\",
	"                             \\     \\@G (###)@g",
	"                              \\ @G(##))########)",
	"                         (#)))################(#))",
	"                    (#)#(#######)))#################)",
	"                   ((#########)#######################)",
	"@w=========================================================",
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
				case 'g':
					printf("\033[0;37m");
					break;
				case 'w':
					printf("\033[1;37m");
					break;
				case 'G':
					printf("\033[0;32m");
					break;
				}
			} else
				printf("%c", image[i][j]);
		}
		printf("\n");
	}
	printf("\033[0;39m");
}

