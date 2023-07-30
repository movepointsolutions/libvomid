/* (C)opyright 2009 Anton Novikov
 * See LICENSE file for license details.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <vomid.h>

static void
die(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);

	exit(1);
}

static void
print_enum_clb(const char *id, const char *name, void *arg)
{
	printf("%s\t%s\n", id, name);
}

int device_set = 0;

static void
set_enum_clb(const char *id, const char *name, void *arg)
{
	if (!device_set && !strcmp(id, arg)) {
		printf("Trying to set device %s\n", id);
		if (vmd_set_device(VMD_OUTPUT_DEVICE, id) == VMD_OK)
			device_set = 1;
	}
}

static void
beat()
{
	static const int b = VMD_VOICE_NOTEON + VMD_DRUMCHANNEL;
	static int x = 0;
	x = (x + 1) % 2;
	unsigned char o[3] = {b, 35 + x, 100};
	vmd_output(o, sizeof(o));
	vmd_flush_output();
}

int
main(int argc, char **argv)
{
	if (argc >= 2 && !strcmp(argv[1], "-l")) {
		vmd_enum_devices(VMD_OUTPUT_DEVICE, print_enum_clb, NULL);
		exit(0);
	} else if (argc < 3) {
		die("Usage: %s output_port beats.txt\n"
		    "               play file\n"
		    "       %s -l\n"
			"               list all output ports\n",
		argv[0], argv[0]);
	}

	vmd_enum_devices(VMD_OUTPUT_DEVICE, set_enum_clb, argv[1]);
	if (!device_set)
		die("Could not open output port\n");

	vmd_systime_t t = 0, nt;
	FILE *f = fopen(argv[2], "r");
	while (fscanf(f, "%lf", &nt) == 1) {
		vmd_sleep(nt - t);
		beat();
		t = nt;
	}
	beat();
}
