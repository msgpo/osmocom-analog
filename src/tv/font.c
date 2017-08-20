#include <stdint.h>
#include "font.h"

static const uint8_t font_c64_data[] = {
/* --- new character ' ' (32) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
/* --- new character '!' (33) */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x18,	/* ...##... */
	0x00,	/* ........ */
/* --- new character '"' (34) */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
/* --- new character '#' (35) */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0xff,	/* ######## */
	0x66,	/* .##..##. */
	0xff,	/* ######## */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x00,	/* ........ */
/* --- new character '$' (36) */
	0x18,	/* ...##... */
	0x3e,	/* ..#####. */
	0x60,	/* .##..... */
	0x3c,	/* ..####.. */
	0x06,	/* .....##. */
	0x7c,	/* .#####.. */
	0x18,	/* ...##... */
	0x00,	/* ........ */
/* --- new character '%' (37) */
	0x62,	/* .##...#. */
	0x66,	/* .##..##. */
	0x0c,	/* ....##.. */
	0x18,	/* ...##... */
	0x30,	/* ..##.... */
	0x66,	/* .##..##. */
	0x46,	/* .#...##. */
	0x00,	/* ........ */
/* --- new character '&' (38) */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x38,	/* ..###... */
	0x67,	/* .##..### */
	0x66,	/* .##..##. */
	0x3f,	/* ..###### */
	0x00,	/* ........ */
/* --- new character ''' (39) */
	0x06,	/* .....##. */
	0x0c,	/* ....##.. */
	0x18,	/* ...##... */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
/* --- new character '(' (40) */
	0x0c,	/* ....##.. */
	0x18,	/* ...##... */
	0x30,	/* ..##.... */
	0x30,	/* ..##.... */
	0x30,	/* ..##.... */
	0x18,	/* ...##... */
	0x0c,	/* ....##.. */
	0x00,	/* ........ */
/* --- new character ')' (41) */
	0x30,	/* ..##.... */
	0x18,	/* ...##... */
	0x0c,	/* ....##.. */
	0x0c,	/* ....##.. */
	0x0c,	/* ....##.. */
	0x18,	/* ...##... */
	0x30,	/* ..##.... */
	0x00,	/* ........ */
/* --- new character '*' (42) */
	0x00,	/* ........ */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0xff,	/* ######## */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x00,	/* ........ */
	0x00,	/* ........ */
/* --- new character '+' (43) */
	0x00,	/* ........ */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x7e,	/* .######. */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x00,	/* ........ */
	0x00,	/* ........ */
/* --- new character ',' (44) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x30,	/* ..##.... */
/* --- new character '-' (45) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x7e,	/* .######. */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
/* --- new character '.' (46) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x00,	/* ........ */
/* --- new character '/' (47) */
	0x00,	/* ........ */
	0x03,	/* ......## */
	0x06,	/* .....##. */
	0x0c,	/* ....##.. */
	0x18,	/* ...##... */
	0x30,	/* ..##.... */
	0x60,	/* .##..... */
	0x00,	/* ........ */
/* --- new character '0' (48) */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x6e,	/* .##.###. */
	0x76,	/* .###.##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character '1' (49) */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x38,	/* ..###... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x7e,	/* .######. */
	0x00,	/* ........ */
/* --- new character '2' (50) */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x06,	/* .....##. */
	0x0c,	/* ....##.. */
	0x30,	/* ..##.... */
	0x60,	/* .##..... */
	0x7e,	/* .######. */
	0x00,	/* ........ */
/* --- new character '3' (51) */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x06,	/* .....##. */
	0x1c,	/* ...###.. */
	0x06,	/* .....##. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character '4' (52) */
	0x06,	/* .....##. */
	0x0e,	/* ....###. */
	0x1e,	/* ...####. */
	0x66,	/* .##..##. */
	0x7f,	/* .####### */
	0x06,	/* .....##. */
	0x06,	/* .....##. */
	0x00,	/* ........ */
/* --- new character '5' (53) */
	0x7e,	/* .######. */
	0x60,	/* .##..... */
	0x7c,	/* .#####.. */
	0x06,	/* .....##. */
	0x06,	/* .....##. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character '6' (54) */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x60,	/* .##..... */
	0x7c,	/* .#####.. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character '7' (55) */
	0x7e,	/* .######. */
	0x66,	/* .##..##. */
	0x0c,	/* ....##.. */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x00,	/* ........ */
/* --- new character '8' (56) */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character '9' (57) */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3e,	/* ..#####. */
	0x06,	/* .....##. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character ':' (58) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x18,	/* ...##... */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x18,	/* ...##... */
	0x00,	/* ........ */
	0x00,	/* ........ */
/* --- new character ';' (59) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x18,	/* ...##... */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x30,	/* ..##.... */
/* --- new character '<' (60) */
	0x0e,	/* ....###. */
	0x18,	/* ...##... */
	0x30,	/* ..##.... */
	0x60,	/* .##..... */
	0x30,	/* ..##.... */
	0x18,	/* ...##... */
	0x0e,	/* ....###. */
	0x00,	/* ........ */
/* --- new character '=' (61) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x7e,	/* .######. */
	0x00,	/* ........ */
	0x7e,	/* .######. */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
/* --- new character '>' (62) */
	0x70,	/* .###.... */
	0x18,	/* ...##... */
	0x0c,	/* ....##.. */
	0x06,	/* .....##. */
	0x0c,	/* ....##.. */
	0x18,	/* ...##... */
	0x70,	/* .###.... */
	0x00,	/* ........ */
/* --- new character '?' (63) */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x06,	/* .....##. */
	0x0c,	/* ....##.. */
	0x18,	/* ...##... */
	0x00,	/* ........ */
	0x18,	/* ...##... */
	0x00,	/* ........ */
/* --- new character '@' (64) */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x6e,	/* .##.###. */
	0x6e,	/* .##.###. */
	0x60,	/* .##..... */
	0x62,	/* .##...#. */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character 'A' (65) */
	0x18,	/* ...##... */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x7e,	/* .######. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x00,	/* ........ */
/* --- new character 'B' (66) */
	0x7c,	/* .#####.. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x7c,	/* .#####.. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x7c,	/* .#####.. */
	0x00,	/* ........ */
/* --- new character 'C' (67) */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character 'D' (68) */
	0x78,	/* .####... */
	0x6c,	/* .##.##.. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x6c,	/* .##.##.. */
	0x78,	/* .####... */
	0x00,	/* ........ */
/* --- new character 'E' (69) */
	0x7e,	/* .######. */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x78,	/* .####... */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x7e,	/* .######. */
	0x00,	/* ........ */
/* --- new character 'F' (70) */
	0x7e,	/* .######. */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x78,	/* .####... */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x00,	/* ........ */
/* --- new character 'G' (71) */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x60,	/* .##..... */
	0x6e,	/* .##.###. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character 'H' (72) */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x7e,	/* .######. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x00,	/* ........ */
/* --- new character 'I' (73) */
	0x3c,	/* ..####.. */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character 'J' (74) */
	0x1e,	/* ...####. */
	0x0c,	/* ....##.. */
	0x0c,	/* ....##.. */
	0x0c,	/* ....##.. */
	0x0c,	/* ....##.. */
	0x6c,	/* .##.##.. */
	0x38,	/* ..###... */
	0x00,	/* ........ */
/* --- new character 'K' (75) */
	0x66,	/* .##..##. */
	0x6c,	/* .##.##.. */
	0x78,	/* .####... */
	0x70,	/* .###.... */
	0x78,	/* .####... */
	0x6c,	/* .##.##.. */
	0x66,	/* .##..##. */
	0x00,	/* ........ */
/* --- new character 'L' (76) */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x7e,	/* .######. */
	0x00,	/* ........ */
/* --- new character 'M' (77) */
	0x63,	/* .##...## */
	0x77,	/* .###.### */
	0x7f,	/* .####### */
	0x6b,	/* .##.#.## */
	0x63,	/* .##...## */
	0x63,	/* .##...## */
	0x63,	/* .##...## */
	0x00,	/* ........ */
/* --- new character 'N' (78) */
	0x66,	/* .##..##. */
	0x76,	/* .###.##. */
	0x7e,	/* .######. */
	0x7e,	/* .######. */
	0x6e,	/* .##.###. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x00,	/* ........ */
/* --- new character 'O' (79) */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character 'P' (80) */
	0x7c,	/* .#####.. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x7c,	/* .#####.. */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x00,	/* ........ */
/* --- new character 'Q' (81) */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x0e,	/* ....###. */
	0x00,	/* ........ */
/* --- new character 'R' (82) */
	0x7c,	/* .#####.. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x7c,	/* .#####.. */
	0x78,	/* .####... */
	0x6c,	/* .##.##.. */
	0x66,	/* .##..##. */
	0x00,	/* ........ */
/* --- new character 'S' (83) */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x60,	/* .##..... */
	0x3c,	/* ..####.. */
	0x06,	/* .....##. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character 'T' (84) */
	0x7e,	/* .######. */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x00,	/* ........ */
/* --- new character 'U' (85) */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character 'V' (86) */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x18,	/* ...##... */
	0x00,	/* ........ */
/* --- new character 'W' (87) */
	0x63,	/* .##...## */
	0x63,	/* .##...## */
	0x63,	/* .##...## */
	0x6b,	/* .##.#.## */
	0x7f,	/* .####### */
	0x77,	/* .###.### */
	0x63,	/* .##...## */
	0x00,	/* ........ */
/* --- new character 'X' (88) */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x18,	/* ...##... */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x00,	/* ........ */
/* --- new character 'Y' (89) */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x00,	/* ........ */
/* --- new character 'Z' (90) */
	0x7e,	/* .######. */
	0x06,	/* .....##. */
	0x0c,	/* ....##.. */
	0x18,	/* ...##... */
	0x30,	/* ..##.... */
	0x60,	/* .##..... */
	0x7e,	/* .######. */
	0x00,	/* ........ */
/* --- new character '[' (91) */
	0x3c,	/* ..####.. */
	0x30,	/* ..##.... */
	0x30,	/* ..##.... */
	0x30,	/* ..##.... */
	0x30,	/* ..##.... */
	0x30,	/* ..##.... */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character '\' (92) */
	0x00,	/* ........ */
	0xc0,	/* ##...... */
	0x60,	/* .##..... */
	0x30,	/* ..##.... */
	0x18,	/* ...##... */
	0x0c,	/* ....##.. */
	0x06,	/* .....##. */
	0x00,	/* ........ */
/* --- new character ']' (93) */
	0x3c,	/* ..####.. */
	0x0c,	/* ....##.. */
	0x0c,	/* ....##.. */
	0x0c,	/* ....##.. */
	0x0c,	/* ....##.. */
	0x0c,	/* ....##.. */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character '^' (94) */
	0x18,	/* ...##... */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
/* --- new character '_' (95) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0xff,	/* ######## */
/* --- new character '`' (96) */
	0x60,	/* .##..... */
	0x30,	/* ..##.... */
	0x18,	/* ...##... */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
/* --- new character 'a' (97) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x3c,	/* ..####.. */
	0x06,	/* .....##. */
	0x3e,	/* ..#####. */
	0x66,	/* .##..##. */
	0x3e,	/* ..#####. */
	0x00,	/* ........ */
/* --- new character 'b' (98) */
	0x00,	/* ........ */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x7c,	/* .#####.. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x7c,	/* .#####.. */
	0x00,	/* ........ */
/* --- new character 'c' (99) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x3c,	/* ..####.. */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character 'd' (100) */
	0x00,	/* ........ */
	0x06,	/* .....##. */
	0x06,	/* .....##. */
	0x3e,	/* ..#####. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3e,	/* ..#####. */
	0x00,	/* ........ */
/* --- new character 'e' (101) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x7e,	/* .######. */
	0x60,	/* .##..... */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character 'f' (102) */
	0x00,	/* ........ */
	0x0e,	/* ....###. */
	0x18,	/* ...##... */
	0x3e,	/* ..#####. */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x00,	/* ........ */
/* --- new character 'g' (103) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x3e,	/* ..#####. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3e,	/* ..#####. */
	0x06,	/* .....##. */
	0x7c,	/* .#####.. */
/* --- new character 'h' (104) */
	0x00,	/* ........ */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x7c,	/* .#####.. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x00,	/* ........ */
/* --- new character 'i' (105) */
	0x00,	/* ........ */
	0x18,	/* ...##... */
	0x00,	/* ........ */
	0x38,	/* ..###... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character 'j' (106) */
	0x00,	/* ........ */
	0x06,	/* .....##. */
	0x00,	/* ........ */
	0x06,	/* .....##. */
	0x06,	/* .....##. */
	0x06,	/* .....##. */
	0x06,	/* .....##. */
	0x3c,	/* ..####.. */
/* --- new character 'k' (107) */
	0x00,	/* ........ */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x6c,	/* .##.##.. */
	0x78,	/* .####... */
	0x6c,	/* .##.##.. */
	0x66,	/* .##..##. */
	0x00,	/* ........ */
/* --- new character 'l' (108) */
	0x00,	/* ........ */
	0x38,	/* ..###... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character 'm' (109) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x66,	/* .##..##. */
	0x7f,	/* .####### */
	0x7f,	/* .####### */
	0x6b,	/* .##.#.## */
	0x63,	/* .##...## */
	0x00,	/* ........ */
/* --- new character 'n' (110) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x7c,	/* .#####.. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x00,	/* ........ */
/* --- new character 'o' (111) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x00,	/* ........ */
/* --- new character 'p' (112) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x7c,	/* .#####.. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x7c,	/* .#####.. */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
/* --- new character 'q' (113) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x3e,	/* ..#####. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3e,	/* ..#####. */
	0x06,	/* .....##. */
	0x06,	/* .....##. */
/* --- new character 'r' (114) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x7c,	/* .#####.. */
	0x66,	/* .##..##. */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x60,	/* .##..... */
	0x00,	/* ........ */
/* --- new character 's' (115) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x3e,	/* ..#####. */
	0x60,	/* .##..... */
	0x3c,	/* ..####.. */
	0x06,	/* .....##. */
	0x7c,	/* .#####.. */
	0x00,	/* ........ */
/* --- new character 't' (116) */
	0x00,	/* ........ */
	0x18,	/* ...##... */
	0x7e,	/* .######. */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x0e,	/* ....###. */
	0x00,	/* ........ */
/* --- new character 'u' (117) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3e,	/* ..#####. */
	0x00,	/* ........ */
/* --- new character 'v' (118) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x18,	/* ...##... */
	0x00,	/* ........ */
/* --- new character 'w' (119) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x63,	/* .##...## */
	0x6b,	/* .##.#.## */
	0x7f,	/* .####### */
	0x3e,	/* ..#####. */
	0x36,	/* ..##.##. */
	0x00,	/* ........ */
/* --- new character 'x' (120) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x66,	/* .##..##. */
	0x3c,	/* ..####.. */
	0x18,	/* ...##... */
	0x3c,	/* ..####.. */
	0x66,	/* .##..##. */
	0x00,	/* ........ */
/* --- new character 'y' (121) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x66,	/* .##..##. */
	0x3e,	/* ..#####. */
	0x0c,	/* ....##.. */
	0x78,	/* .####... */
/* --- new character 'z' (122) */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x7e,	/* .######. */
	0x0c,	/* ....##.. */
	0x18,	/* ...##... */
	0x30,	/* ..##.... */
	0x7e,	/* .######. */
	0x00,	/* ........ */
/* --- new character '{' (123) */
	0x1c,	/* ...###.. */
	0x30,	/* ..##.... */
	0x30,	/* ..##.... */
	0x60,	/* .##..... */
	0x30,	/* ..##.... */
	0x30,	/* ..##.... */
	0x1c,	/* ...###.. */
	0x00,	/* ........ */
/* --- new character '|' (124) */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x18,	/* ...##... */
	0x00,	/* ........ */
/* --- new character '}' (125) */
	0x38,	/* ..###... */
	0x0c,	/* ....##.. */
	0x0c,	/* ....##.. */
	0x06,	/* .....##. */
	0x0c,	/* ....##.. */
	0x0c,	/* ....##.. */
	0x38,	/* ..###... */
	0x00,	/* ........ */
/* --- new character '~' (126) */
	0x33,	/* ..##..## */
	0x7e,	/* .######. */
	0xcc,	/* ##..##.. */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
	0x00,	/* ........ */
/* --- new character '' (127) */
	0x00,	/* ........ */
	0x08,	/* ....#... */
	0x0c,	/* ....##.. */
	0xfe,	/* #######. */
	0xfe,	/* #######. */
	0x0c,	/* ....##.. */
	0x08,	/* ....#... */
	0x00,	/* ........ */
};

const uint8_t *get_font(char c)
{
	if (c < 32) /* implies c > 127 */
		c = 32;
	return font_c64_data + 8 * (int)(c - 32);
}

