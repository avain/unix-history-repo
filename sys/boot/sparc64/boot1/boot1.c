/*
 * Copyright (c) 1998 Robert Nordier
 * All rights reserved.
 * Copyright (c) 2001 Robert Drehmel
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 *
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/diskslice.h>
#include <sys/disklabel.h>
#include <sys/dirent.h>
#include <machine/elf.h>
#include <machine/stdarg.h>

#include <ufs/ffs/fs.h>
#include <ufs/ufs/dinode.h>

#define _PATH_LOADER	"/boot/loader"
#define _PATH_KERNEL	"/boot/kernel/kernel"

#define BSIZEMAX	8192

/*
 * This structure will be refined along with the addition of a bootpath
 * parsing routine when it is necessary to cope with bootpaths that are
 * not in the exact <devpath>@<controller>,<disk>:<partition> format and
 * for which we need to evaluate the disklabel ourselves.
 */ 
struct disk {
	int meta;
};
struct disk dsk;

static const char digits[] = "0123456789abcdef";

static char bootpath[128];
static char bootargs[128];

static int kflag;

static uint32_t fs_off;

typedef int putc_func_t(int c, void *arg);

int main(int ac, char **av);

static void exit(int);
static void load(const char *);
static ino_t lookup(const char *);
static ssize_t fsread(ino_t, void *, size_t);
static int dskread(void *, u_int64_t, int);

static void bcopy(const void *src, void *dst, size_t len);
static void bzero(void *b, size_t len);

static int printf(const char *fmt, ...);
static int vprintf(const char *fmt, va_list ap);
static int putchar(int c, void *arg);

static int __printf(const char *fmt, putc_func_t *putc, void *arg, va_list ap);
static int __putc(int c, void *arg);
static int __puts(const char *s, putc_func_t *putc, void *arg);
static char *__uitoa(char *buf, u_int val, int base);
static char *__ultoa(char *buf, u_long val, int base);

/*
 * Open Firmware interface functions
 */
typedef u_int64_t	ofwcell_t;
typedef int32_t		ofwh_t;
typedef u_int32_t	u_ofwh_t;
typedef int (*ofwfp_t)(ofwcell_t []);
ofwfp_t ofw;			/* the prom Open Firmware entry */

void ofw_init(int, int, int, int, ofwfp_t);
ofwh_t ofw_finddevice(const char *);
ofwh_t ofw_open(const char *);
int ofw_getprop(ofwh_t, const char *, void *, size_t);
int ofw_read(ofwh_t, void *, size_t);
int ofw_write(ofwh_t, const void *, size_t);
int ofw_seek(ofwh_t, u_int64_t);

ofwh_t bootdevh;
ofwh_t stdinh, stdouth;

/*
 * This has to stay here, as the PROM seems to ignore the
 * entry point specified in the a.out header.  (or elftoaout is broken)
 */

void
ofw_init(int d, int d1, int d2, int d3, ofwfp_t ofwaddr)
{
	ofwh_t chosenh;
	char *av[16];
	char *p;
	int ac;

	ofw = ofwaddr;

	chosenh = ofw_finddevice("/chosen");
	ofw_getprop(chosenh, "stdin", &stdinh, sizeof(stdinh));
	ofw_getprop(chosenh, "stdout", &stdouth, sizeof(stdouth));
	ofw_getprop(chosenh, "bootargs", bootargs, sizeof(bootargs));
	ofw_getprop(chosenh, "bootpath", bootpath, sizeof(bootpath));

	bootargs[sizeof(bootargs) - 1] = '\0';
	bootpath[sizeof(bootpath) - 1] = '\0';

	if ((bootdevh = ofw_open(bootpath)) == -1) {
		printf("Could not open boot device.\n");
	}

	ac = 0;
	p = bootargs;
	for (;;) {
		while (*p == ' ' && *p != '\0')
			p++;
		if (*p == '\0' || ac >= 16)
			break;
		av[ac++] = p;
		while (*p != ' ' && *p != '\0')
			p++;
		if (*p != '\0')
			*p++ = '\0';
	}

	exit(main(ac, av));
}

ofwh_t
ofw_finddevice(const char *name)
{
	ofwcell_t args[] = {
		(ofwcell_t)"finddevice",
		1,
		1,
		(ofwcell_t)name,
		0
	};

	if ((*ofw)(args)) {
		printf("ofw_finddevice: name=\"%s\"\n", name);
		return (1);
	}
	return (args[4]);
}

int
ofw_getprop(ofwh_t ofwh, const char *name, void *buf, size_t len)
{
	ofwcell_t args[] = {
		(ofwcell_t)"getprop",
		4,
		1,
		(u_ofwh_t)ofwh,
		(ofwcell_t)name,
		(ofwcell_t)buf,
		len,
	0
	};

	if ((*ofw)(args)) {
		printf("ofw_getprop: ofwh=0x%x buf=%p len=%u\n",
			ofwh, buf, len);
		return (1);
	}
	return (0);
}

ofwh_t
ofw_open(const char *path)
{
	ofwcell_t args[] = {
		(ofwcell_t)"open",
		1,
		1,
		(ofwcell_t)path,
		0
	};

	if ((*ofw)(args)) {
		printf("ofw_open: path=\"%s\"\n", path);
		return (-1);
	}
	return (args[4]);
}

int
ofw_close(ofwh_t devh)
{
	ofwcell_t args[] = {
		(ofwcell_t)"close",
		1,
		0,
		(u_ofwh_t)devh
	};

	if ((*ofw)(args)) {
		printf("ofw_close: devh=0x%x\n", devh);
		return (1);
	}
	return (0);
}

int
ofw_read(ofwh_t devh, void *buf, size_t len)
{
	ofwcell_t args[] = {
		(ofwcell_t)"read",
		4,
		1,
		(u_ofwh_t)devh,
		(ofwcell_t)buf,
		len,
		0
	};

	if ((*ofw)(args)) {
		printf("ofw_read: devh=0x%x buf=%p len=%u\n", devh, buf, len);
		return (1);
	}
	return (0);
}

int
ofw_write(ofwh_t devh, const void *buf, size_t len)
{
	ofwcell_t args[] = {
		(ofwcell_t)"write",
		3,
		1,
		(u_ofwh_t)devh,
		(ofwcell_t)buf,
		len,
		0
	};

	if ((*ofw)(args)) {
		printf("ofw_write: devh=0x%x buf=%p len=%u\n", devh, buf, len);
		return (1);
	}
	return (0);
}

int
ofw_seek(ofwh_t devh, u_int64_t off)
{
	ofwcell_t args[] = {
		(ofwcell_t)"seek",
		4,
		1,
		(u_ofwh_t)devh,
		off >> 32,
		off,
		0
	};

	if ((*ofw)(args)) {
		printf("ofw_seek: devh=0x%x off=0x%lx\n", devh, off);
		return (1);
	}
	return (0);
}

void
ofw_exit(void)
{
	ofwcell_t args[3];

	args[0] = (ofwcell_t)"exit";
	args[1] = 0;
	args[2] = 0;

	(*ofw)(args);
}

static void
bcopy(const void *dst, void *src, size_t len)
{
	const char *d = dst;
	char *s = src;

	while (len-- != 0)
		*s++ = *d++;
}

static void
bzero(void *b, size_t len)
{
	char *p = b;

	while (len-- != 0)
		*p++ = 0;
}

static int
strcmp(const char *s1, const char *s2)
{
	for (; *s1 == *s2 && *s1; s1++, s2++)
		;
	return ((u_char)*s1 - (u_char)*s2);
}

static int
fsfind(const char *name, ino_t * ino)
{
	char buf[DEV_BSIZE];
	struct dirent *d;
	char *s;
	ssize_t n;

	fs_off = 0;
	while ((n = fsread(*ino, buf, DEV_BSIZE)) > 0) {
		for (s = buf; s < buf + DEV_BSIZE;) {
			d = (void *)s;
			if (!strcmp(name, d->d_name)) {
				*ino = d->d_fileno;
				return (d->d_type);
			}
			s += d->d_reclen;
		}
	}
	return (0);
}

int
main(int ac, char **av)
{
	const char *path;
	int i;

	path = _PATH_LOADER;
	for (i = 0; i < ac; i++) {
		switch (av[i][0]) {
		case '-':
			switch (av[i][1]) {
			case 'k':
				kflag = 1;
				break;
			default:
				break;
			}
			break;
		default:
			path = av[i];
			break;
		}
	}

	printf(" \n>> FreeBSD/sparc64 boot block\n"
	"   Boot path:   %s\n"
	"   Boot loader: %s\n", bootpath, path);
	load(path);
	return (1);
}

static void
exit(int code)
{

	ofw_exit();
}

static void
load(const char *fname)
{
	Elf64_Ehdr eh;
	Elf64_Phdr ph;
	caddr_t p;
	ino_t ino;
	int i;

	if ((ino = lookup(fname)) == 0) {
		printf("File %s not found\n", fname);
		return;
	}
	if (fsread(ino, &eh, sizeof(eh)) != sizeof(eh)) {
		printf("Can't read elf header\n");
		return;
	}
	if (!IS_ELF(eh)) {
		printf("Not an ELF file\n");
		return;
	}
	for (i = 0; i < eh.e_phnum; i++) {
		fs_off = eh.e_phoff + i * eh.e_phentsize;
		if (fsread(ino, &ph, sizeof(ph)) != sizeof(ph)) {
			printf("Can't read program header %d\n", i);
			return;
		}
		if (ph.p_type != PT_LOAD)
			continue;
		fs_off = ph.p_offset;
		p = (caddr_t)ph.p_vaddr;
		if (fsread(ino, p, ph.p_filesz) != ph.p_filesz) {
			printf("Can't read content of section %d\n", i);
			return;
		}
		if (ph.p_filesz != ph.p_memsz)
			bzero(p + ph.p_filesz, ph.p_memsz - ph.p_filesz);
	}
	ofw_close(bootdevh);
	(*(void (*)(int, int, int, int, ofwfp_t))eh.e_entry)(0, 0, 0, 0, ofw);
}

static ino_t
lookup(const char *path)
{
	char name[MAXNAMLEN + 1];
	const char *s;
	ino_t ino;
	ssize_t n;
	int dt;

	ino = ROOTINO;
	dt = DT_DIR;
	name[0] = '/';
	name[1] = '\0';
	for (;;) {
		if (*path == '/')
			path++;
		if (!*path)
			break;
		for (s = path; *s && *s != '/'; s++)
			;
		if ((n = s - path) > MAXNAMLEN)
			return (0);
		bcopy(path, name, n);
		name[n] = 0;
		if (dt != DT_DIR) {
			printf("%s: not a directory.\n", name);
			return (0);
		}
		if ((dt = fsfind(name, &ino)) <= 0)
			break;
		path = s;
	}
	return (dt == DT_REG ? ino : 0);
}

static ssize_t
fsread(ino_t inode, void *buf, size_t nbyte)
{
	static struct fs fs;
	static struct dinode din;
	static char blkbuf[BSIZEMAX];
	static ufs_daddr_t indbuf[BSIZEMAX / sizeof(ufs_daddr_t)];
	static ino_t inomap;
	static ufs_daddr_t blkmap, indmap;
	static unsigned int fsblks;
	char *s;
	ufs_daddr_t lbn, addr;
	size_t n, nb, off;

	if (!dsk.meta) {
		inomap = 0;
		if (dskread(blkbuf, SBOFF / DEV_BSIZE, SBSIZE / DEV_BSIZE))
			return (-1);
		bcopy(blkbuf, &fs, sizeof(fs));
		if (fs.fs_magic != FS_MAGIC) {
			printf("Not ufs\n");
			return (-1);
		}
		fsblks = fs.fs_bsize >> DEV_BSHIFT;
		dsk.meta++;
	}
	if (!inode)
		return (0);
	if (inomap != inode) {
		if (dskread(blkbuf, fsbtodb(&fs, ino_to_fsba(&fs, inode)),
		    fsblks))
			return (-1);
		bcopy(blkbuf + ((inode % INOPB(&fs)) * sizeof(din)), &din,
		    sizeof(din));
		inomap = inode;
		fs_off = 0;
		blkmap = indmap = 0;
	}
	s = buf;
	if (nbyte > (n = din.di_size - fs_off))
		nbyte = n;
	nb = nbyte;
	while (nb) {
		lbn = lblkno(&fs, fs_off);
		if (lbn < NDADDR)
			addr = din.di_db[lbn];
		else {
			if (indmap != din.di_ib[0]) {
				if (dskread(indbuf, fsbtodb(&fs, din.di_ib[0]),
				    fsblks))
					return (-1);
				indmap = din.di_ib[0];
			}
			addr = indbuf[(lbn - NDADDR) % NINDIR(&fs)];
		}
		n = dblksize(&fs, &din, lbn);
		if (blkmap != addr) {
			if (dskread(blkbuf, fsbtodb(&fs, addr),
			    n >> DEV_BSHIFT)) {
				return (-1);
			}
			blkmap = addr;
		}
		off = blkoff(&fs, fs_off);
		n -= off;
		if (n > nb)
			n = nb;
		bcopy(blkbuf + off, s, n);
		s += n;
		fs_off += n;
		nb -= n;
	}
	return (nbyte);
}

static int
dskread(void *buf, u_int64_t lba, int nblk)
{
	/*
	 * The OpenFirmware should open the correct partition for us.
	 * That means, if we read from offset zero on an open instance handle,
	 * we should read from offset zero of that partition.
	 */
	ofw_seek(bootdevh, lba * DEV_BSIZE);
	ofw_read(bootdevh, buf, nblk * DEV_BSIZE);
	return (0);
}

static int
printf(const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vprintf(fmt, ap);
	va_end(ap);
	return (ret);
}

static int
vprintf(const char *fmt, va_list ap)
{
	int ret;

	ret = __printf(fmt, putchar, 0, ap);
	return (ret);
}

static int
putchar(int c, void *arg)
{
	char buf;

	if (c == '\n') {
		buf = '\r';
		ofw_write(stdouth, &buf, 1);
	}
	buf = c;
	ofw_write(stdouth, &buf, 1);
	return (1);
}

static int
__printf(const char *fmt, putc_func_t *putc, void *arg, va_list ap)
{
	char buf[(sizeof(long) * 8) + 1];
	char *nbuf;
	u_long ul;
	u_int ui;
	int lflag;
	int sflag;
	char *s;
	int pad;
	int ret;
	int c;

	nbuf = &buf[sizeof buf - 1];
	ret = 0;
	while ((c = *fmt++) != 0) {
		if (c != '%') {
			ret += putc(c, arg);
			continue;
		}
		lflag = 0;
		sflag = 0;
		pad = 0;
reswitch:	c = *fmt++;
		switch (c) {
		case '#':
			sflag = 1;
			goto reswitch;
		case '%':
			ret += putc('%', arg);
			break;
		case 'c':
			c = va_arg(ap, int);
			ret += putc(c, arg);
			break;
		case 'd':
			if (lflag == 0) {
				ui = (u_int)va_arg(ap, int);
				if (ui < (int)ui) {
					ui = -ui;
					ret += putc('-', arg);
				}
				s = __uitoa(nbuf, ui, 10);
			} else {
				ul = (u_long)va_arg(ap, long);
				if (ul < (long)ul) {
					ul = -ul;
					ret += putc('-', arg);
				}
				s = __ultoa(nbuf, ul, 10);
			}
			ret += __puts(s, putc, arg);
			break;
		case 'l':
			lflag = 1;
			goto reswitch;
		case 'o':
			if (lflag == 0) {
				ui = (u_int)va_arg(ap, u_int);
				s = __uitoa(nbuf, ui, 8);
			} else {
				ul = (u_long)va_arg(ap, u_long);
				s = __ultoa(nbuf, ul, 8);
			}
			ret += __puts(s, putc, arg);
			break;
		case 'p':
			ul = (u_long)va_arg(ap, void *);
			s = __ultoa(nbuf, ul, 16);
			ret += __puts("0x", putc, arg);
			ret += __puts(s, putc, arg);
			break;
		case 's':
			s = va_arg(ap, char *);
			ret += __puts(s, putc, arg);
			break;
		case 'u':
			if (lflag == 0) {
				ui = va_arg(ap, u_int);
				s = __uitoa(nbuf, ui, 10);
			} else {
				ul = va_arg(ap, u_long);
				s = __ultoa(nbuf, ul, 10);
			}
			ret += __puts(s, putc, arg);
			break;
		case 'x':
			if (lflag == 0) {
				ui = va_arg(ap, u_int);
				s = __uitoa(nbuf, ui, 16);
			} else {
				ul = va_arg(ap, u_long);
				s = __ultoa(nbuf, ul, 16);
			}
			if (sflag)
				ret += __puts("0x", putc, arg);
			ret += __puts(s, putc, arg);
			break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			pad = pad * 10 + c - '0';
			goto reswitch;
		default:
			break;
		}
	}
	return (ret);
}

static int
__puts(const char *s, putc_func_t *putc, void *arg)
{
	const char *p;
	int ret;

	ret = 0;
	for (p = s; *p != '\0'; p++)
		ret += putc(*p, arg);
	return (ret);
}

static char *
__uitoa(char *buf, u_int ui, int base)
{
	char *p;

	p = buf;
	*p = '\0';
	do
		*--p = digits[ui % base];
	while ((ui /= base) != 0);
	return (p);
}

static char *
__ultoa(char *buf, u_long ul, int base)
{
	char *p;

	p = buf;
	*p = '\0';
	do
		*--p = digits[ul % base];
	while ((ul /= base) != 0);
	return (p);
}
