/*****************************************************************************\
**                                                                           **
** BEEP-BEEP                                                                 **
**                                                                           **
**---------------------------------------------------------------------------**
** Copyright: Andreas Eversberg                                              **
** Converted to use PulseAudio by Daniel Ponte				     **
**                                                                           **
** minimal blueboxing software                                               **
**                                                                           **
\*****************************************************************************/ 


#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdlib.h>

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>

#define SAMPLE_RATE	44100
#define PI		3.14159265

struct ccitt5 {
	char digit;
	int f1, f2;
	int duration, delay;
} ccitt5[] = {
	{'A', 2400,    0, 150, 100},
	{'B', 2600,    0, 850, 100},
	{'C', 2400, 2600,  70, 100},
	{'1',  700,  900,  55,  50},
	{'2',  700, 1100,  55,  50},
	{'3',  900, 1100,  55,  50},
	{'4',  700, 1300,  55,  50},
	{'5',  900, 1300,  55,  50},
	{'6', 1100, 1300,  55,  50},
	{'7',  700, 1500,  55,  50},
	{'8',  900, 1500,  55,  50},
	{'9', 1100, 1500,  55,  50},
	{'0', 1300, 1500,  55,  50},
	{'d',  700, 1700,  55,  50},
	{'e',  900, 1700,  55,  50},
	{'a', 1100, 1700, 100,  50},
	{'b', 1300, 1700, 100,  50},
	{'c', 1500, 1700,  55,  50},
	{0, 0, 0, 0, 0}
};

void close_pa(pa_simple *s)
{
	if (s)
		pa_simple_free(s);
}


/*
 * stream bell sound
 */
void dial_pa(pa_simple *s, char *dial)
{
	int i, j, len;
	int error;
	double k1, k2;
	unsigned char buffer[SAMPLE_RATE]; /* one second */

	if(!s)
		return;

	while(*dial) {
		i = 0;
		while(ccitt5[i].digit) {
			if (ccitt5[i].digit == *dial)
				break;
			i++;
		}
		if (ccitt5[i].digit) {
			/* send tone */
			len = ccitt5[i].duration * SAMPLE_RATE / 1000;
			k1 = 2.0 * PI * ccitt5[i].f1 / SAMPLE_RATE;
			k2 = 2.0 * PI * ccitt5[i].f2 / SAMPLE_RATE;
			for (j = 0; j < len; j++)
				buffer[j] = 64.0 * (2.0 + sin(k1 * j) + sin(k2 * j));
			if (pa_simple_write(s, buffer, (size_t) len, &error) < 0) {
				fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
				close_pa(s);
				exit(-1);
			}
			/* send pause */
			len = ccitt5[i].delay * SAMPLE_RATE / 1000;
			memset(buffer, 128, len);
			if (pa_simple_write(s, buffer, (size_t) len, &error) < 0) {
				fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
				close_pa(s);
				exit(-1);
			}

		}
		dial++;
	}

	if (pa_simple_drain(s, &error) < 0) {
		fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
		close_pa(s);
		exit(-1);
	}
}


/*
 * open pulse
 */
pa_simple* open_pa(char *pname)
{
	int error;

	static const pa_sample_spec ss = {
		.format = PA_SAMPLE_U8,
		.rate = SAMPLE_RATE,
		.channels = 1
	};
	pa_simple *s = NULL;

	if (!(s = pa_simple_new(NULL, pname, PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) {
		fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
		return NULL;
	}

	return s;
}

int main(int argc, char *argv[])
{
	pa_simple *s;
	char *dial = "";
	int ch;
	char digit[2] = "x";
	struct termios orig, now;

	if (argc > 1 && !strcmp(argv[1], "--help")) {
		printf("Usage: %s [<dial string>]\n\n", argv[0]);
		printf("Runs with with the given dial string.\n");
		printf("For KP1, KP2, ST, use characters a, b, c instead.\n");
		return 0;
	}

	if (argc > 1)
		dial = argv[1];

	s = open_pa(argv[0]);
	if (s == NULL) {
		fprintf(stderr, "cannot open pulse\n");
		return 1;
	}

	printf("Welcome to Beep-Beep, your simple bluebox!\n");
	printf("------------------------------------------\n");
	printf("y or z = clear-forward\n");
	printf("x = seize\n");
	printf("0-9 = Digits\n");
	printf("a = KP1\n");
	printf("b = KP2\n");
	printf("c = ST\n");
	printf("d = Code 11\n");
	printf("e = Code 12\n");
	printf("f = forward transfer\n");
	if (dial[0])
		printf(". = dial %s\n", dial);
	printf("Escape or q to exit.\n");

	tcgetattr(0, &orig);
     	now = orig;
	now.c_lflag &= ~(ISIG|ICANON|ECHO);
	now.c_cc[VMIN]=1;
	now.c_cc[VTIME]=2;

	tcsetattr(0, TCSANOW, &now);
	while ((ch = getc(stdin)) != 27) {
		switch(ch) {
		case 'y': case 'z':
			dial_pa(s, "C");
			break;
		case 'x':
			dial_pa(s, "A");
			break;
		case 'f':
			dial_pa(s, "B");
			break;
		case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9': case '0':
		case 'a': case 'b': case 'c': case 'd': case 'e':
			digit[0] = ch;
			dial_pa(s, digit);
			break;
		case '.':
			if (dial[0])
				dial_pa(s, dial);
			break;
		}
		if (ch == 3 || ch == 'q')
			break;
	}
	tcsetattr(0, TCSANOW, &orig);

	close_pa(s);

	return 0;
}


