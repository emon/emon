/* a very simple AVI writer - no fancy stuff 
   Tridge, July 2000
*/
/* 
   Copyright (C) Andrew Tridgell 2000
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "capture.h"


static int nframes;
static int totalsize;

/* start writing an AVI file */
void avi_start(int fd)
{
	int ofs = sizeof(struct riff_head)+
		sizeof(struct list_head)+
		sizeof(struct avi_head)+
		sizeof(struct list_head)+
		sizeof(struct stream_head)+
		sizeof(struct frame_head)+
		sizeof(struct list_head);

	lseek(fd, ofs, SEEK_SET);

	nframes = 0;
	totalsize = 0;
}


/* add a jpeg frame to an AVI file */
void avi_add(int fd, u8 *buf, int size)
{
	struct db_head db = {"00db", size};
	write(fd, &db, sizeof(db));
	write(fd, buf, size);
	nframes++;
	totalsize += size;
}

/* finish writing the AVI file - filling in the header */
void avi_end(int fd, int width, int height, int fps)
{
	struct riff_head rh = {"RIFF", 0, "AVI "};
	struct list_head lh1 = {"LIST", 0, "hdrl"};
	struct avi_head ah;
	struct list_head lh2 = {"LIST", 0, "strl"};
	struct stream_head sh;
	struct frame_head fh;
	struct list_head lh3 = {"LIST", 0, "movi"};

	bzero(&ah, sizeof(ah));
	strcpy(ah.avih, "avih");
	ah.time = 1e6 / fps;
	ah.numstreams = 1;
	ah.scale = 1;
	ah.rate = fps;
	ah.length = fps * nframes;

	bzero(&sh, sizeof(sh));
	strcpy(sh.strh, "strh");
	strcpy(sh.vids, "vids");
	strcpy(sh.codec, "MJPG");
	sh.scale = 1;
	sh.rate = fps;
	sh.length = fps * nframes;
	sh.quality = -1;

	bzero(&fh, sizeof(fh));
	strcpy(fh.strf, "strf");
	fh.width = width;
	fh.height = height;
	fh.planes = 1;
	fh.bitcount = 24;
	strcpy(fh.codec,"MJPG");
	fh.unpackedsize = 3*width*height;

	rh.size = sizeof(lh1)+sizeof(ah)+sizeof(lh2)+sizeof(sh)+
		sizeof(fh)+sizeof(lh3)+
		nframes*sizeof(struct db_head)+
		totalsize;
	lh1.size = 4+sizeof(ah)+sizeof(lh2)+sizeof(sh)+sizeof(fh);
	ah.size = sizeof(ah)-8;
	lh2.size = 4+sizeof(sh)+sizeof(fh);
	sh.size = sizeof(sh)-8;
	fh.size = sizeof(fh)-8;
	fh.size2 = fh.size;
	lh3.size = 4+
		nframes*sizeof(struct db_head)+
		totalsize;

	lseek(fd, 0, SEEK_SET);
	
	write(fd, &rh, sizeof(rh));
	write(fd, &lh1, sizeof(lh1));
	write(fd, &ah, sizeof(ah));
	write(fd, &lh2, sizeof(lh2));
	write(fd, &sh, sizeof(sh));
	write(fd, &fh, sizeof(fh));
	write(fd, &lh3, sizeof(lh3));
}


/* NOTE: This is not a general purpose routine - it is meant to only
   cope with AVIs saved using the other functions in this file */
void avi_explode(char *fname)
{
	struct riff_head rh;
	struct list_head lh1;
	struct avi_head ah;
	struct list_head lh2;
	struct stream_head sh;
	struct frame_head fh;
	struct list_head lh3;
	int tsize;
	u16 *tables = mchip_tables(&tsize);
	int fd, i;

	fd = open(fname,O_RDONLY);
	if (fd == -1) {
		perror(fname);
		return;
	}

	read(fd, &rh, sizeof(rh));
	read(fd, &lh1, sizeof(lh1));
	read(fd, &ah, sizeof(ah));
	read(fd, &lh2, sizeof(lh2));
	read(fd, &sh, sizeof(sh));
	read(fd, &fh, sizeof(fh));
	read(fd, &lh3, sizeof(lh3));
	
	for (i=0; ; i++) {
		u8 buf[100*1024];
		struct db_head db;
		char fname[100];
		int fd2;

		if (read(fd, &db, sizeof(db)) != sizeof(db) ||
		    read(fd, buf, db.size) != db.size) break;

		snprintf(fname, sizeof(fname)-1,"frame.%d", i);

		fd2 = open(fname,O_WRONLY|O_CREAT, 0644);
		if (fd2 == -1) {
			perror(fname);
			continue;
		}
		write(fd2, buf, 2);
		write(fd2, tables, tsize);
		write(fd2, buf+2, db.size-2);
		close(fd2);
	}
	close(fd);
	printf("exploded %d frames\n", i);
}
