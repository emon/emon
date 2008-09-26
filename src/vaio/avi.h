/* the AVI format
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

struct riff_head {
	char riff[4]; /* "RIFF" */
	u32 size;
	char avistr[4]; /* "AVI " */
};


struct stream_head { /* 56 bytes */
	char strh[4]; /* "strh" */
	u32 size;
	char vids[4]; /* "vids" */
	char codec[4]; /* codec name */
	u32 flags;
	u32 reserved1;
	u32 initialframes;
	u32 scale; /* 1 */
	u32 rate; /* in frames per second */
	u32 start;
	u32 length; /* what units?? fps*nframes ?? */
	u32 suggested_bufsize;
	u32 quality; /* -1 */
	u32 samplesize;
};


struct avi_head { /* 64 bytes */
	char avih[4]; /* "avih" */
	u32 size;
	u32 time; /* microsec per frame? 1e6 / fps ?? */
	u32 maxbytespersec;
	u32 reserved1;
	u32 flags;
	u32 nframes;
	u32 initialframes;
	u32 numstreams; /* 1 */
	u32 suggested_bufsize;
	u32 width;
	u32 height;
	u32 scale; /* 1 */
	u32 rate; /* fps */
	u32 start;
	u32 length; /* what units?? fps*nframes ?? */
};

struct list_head { /* 12 bytes */
	char list[4]; /* "LIST" */
	u32 size;
	char type[4];
};

struct db_head {
	char db[4]; /* "00db" */
	u32 size;
};

struct frame_head { /* 48 bytes */
	char strf[4]; /* "strf" */
	u32 size;
	u32 size2; /* repeat of previous field? */
	u32 width;
	u32 height;
	u16 planes; /* 1 */
	u16 bitcount; /* 24 */
	char codec[4]; /* MJPG */
	u32 unpackedsize; /* 3 * w * h */
	u32 r1;
	u32 r2;
	u32 clr_used;
	u32 clr_important;
};
