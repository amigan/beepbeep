/*****************************************************************************\
**                                                                           **
** BEEP-BEEP                                                                 **
**                                                                           **
**---------------------------------------------------------------------------**
** Copyright: Andreas Eversberg                                              **
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
#include <linux/soundcard.h>

#define SAMPLE_RATE	8000
#define DSP_DEVICE	"/dev/dsp"
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

/*
 * stream bell sound
 */
void dial_dsp(int dsp, char *dial)
{
	int i, j, len;
	double k1, k2;
	unsigned char buffer[SAMPLE_RATE]; /* one second */

	if (dsp < 0)
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
			write(dsp, buffer, len);
			/* send pause */
			len = ccitt5[i].delay * SAMPLE_RATE / 1000;
			memset(buffer, 128, len);
			write(dsp, buffer, len);
		}
		dial++;
	}

	ioctl(dsp, SOUND_PCM_SYNC, 0);
}


/*
 * open dsp
 */
int open_dsp(char *device, int rate)
{
	int dsp;
	unsigned long arg;
//	audio_buf_info info;

	if ((dsp=open(device, O_WRONLY)) < 0) {
		fprintf(stderr, "cannot open '%s'\n", device);
		return -errno;
	}
#if 0
	if (ioctl(dsp, SNDCTL_DSP_RESET, NULL) < 0) {
		fprintf(stderr, "ioctl failed with SOUND_DSP_RESET\n");
		close(dsp);
		return -errno;
	}
#endif
	arg = 8;
	if (ioctl(dsp, SOUND_PCM_WRITE_BITS, &arg) < 0) {
		fprintf(stderr, "ioctl failed with SOUND_PCM_WRITE_BITS = %lu\n", arg);
		close(dsp);
		return -errno;
	}
	if ((int)arg != 8) {
		fprintf(stderr, "ioctl cannot set correct sample size\n");
		close(dsp);
		return -errno;
	}

        arg = 1;
        if (ioctl(dsp, SOUND_PCM_WRITE_CHANNELS, &arg) < 0) {
                fprintf(stderr, "ioctl failed with SOUND_PCM_WRITE_CHANNELS = %lu\n", arg);
		close(dsp);
		return -errno;
        }
        if ((int)arg != 1) {
                fprintf(stderr, "ioctl cannot set correct channel number\n");
		close(dsp);
		return -errno;
        }

	arg = rate;
     	if (ioctl(dsp, SOUND_PCM_WRITE_RATE,&arg) < 0) {
                fprintf(stderr, "ioctl failed with SOUND_PCM_WRITE_RATE = %lu\n", arg);
		close(dsp);
		return -errno;
        }
        if ((int)arg != rate) {
                fprintf(stderr, "ioctl cannot set correct sample rate (got %lu)\n",arg);
		close(dsp);
		return -errno;
        }
	
	return dsp;
}

void close_dsp(int dsp)
{
	if (dsp >= 0) {
#if 0
		if (ioctl(dsp, SNDCTL_DSP_RESET, NULL) < 0)
			fprintf(stderr, "ioctl failed with SOUND_DSP_RESET\n");
#endif
		close(dsp);
	}
}

int main(int argc, char *argv[])
{
	int dsp;
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

	dsp = open_dsp(DSP_DEVICE, SAMPLE_RATE);
	if (dsp < 0)
		return dsp;

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
	printf("Escape to exit.\n");

	tcgetattr(0, &orig);
     	now = orig;
	now.c_lflag &= ~(ISIG|ICANON|ECHO);
	now.c_cc[VMIN]=1;
	now.c_cc[VTIME]=2;

	tcsetattr(0, TCSANOW, &now);
	while ((ch = getc(stdin)) != 27) {
		switch(ch) {
		case 'y': case 'z':
			dial_dsp(dsp, "C");
			break;
		case 'x':
			dial_dsp(dsp, "A");
			break;
		case 'f':
			dial_dsp(dsp, "B");
			break;
		case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9': case '0':
		case 'a': case 'b': case 'c': case 'd': case 'e':
			digit[0] = ch;
			dial_dsp(dsp, digit);
			break;
		case '.':
			if (dial[0])
				dial_dsp(dsp, dial);
			break;
		}
		if (ch == 3)
			break;
	}
	tcsetattr(0, TCSANOW, &orig);

	close_dsp(dsp);

	return 0;
}


