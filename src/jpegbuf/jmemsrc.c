#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"
#include "bufio.h"

typedef struct {
	struct jpeg_source_mgr pub; /* public fields */

	buf_t *inbuf;		/* source stream */
	JOCTET *buffer;		/* start of buffer */
	boolean start_of_file;	/* have we gotten any data yet? */
} my_source_mgr;

typedef my_source_mgr * my_src_ptr;


METHODDEF(void)
init_source (j_decompress_ptr cinfo)
{
	my_src_ptr src = (my_src_ptr)cinfo->src;
	src->start_of_file = TRUE;
}


METHODDEF(boolean)
fill_input_buffer (j_decompress_ptr cinfo)
{
	static unsigned char EOI[] = {0xFF, JPEG_EOI};
	my_src_ptr src = (my_src_ptr) cinfo->src;
	size_t nbytes = src->inbuf->writecnt;

	src->inbuf->writecnt -= nbytes;

	if (nbytes <= 0) {
		if (src->start_of_file)
			ERREXIT(cinfo, JERR_INPUT_EMPTY);
		WARNMS(cinfo, JWRN_JPEG_EOF);
			/* Insert a fake EOI marker */
		src->buffer = EOI;
		nbytes = 2;
	}

	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;
	src->start_of_file = FALSE;

	return TRUE;
}


METHODDEF(void)
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	my_src_ptr src = (my_src_ptr) cinfo->src;

	if (num_bytes > 0) {
		if (num_bytes > (long) src->pub.bytes_in_buffer) {
			num_bytes -= (long) src->pub.bytes_in_buffer;
			(void) fill_input_buffer(cinfo);
		}
		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}


METHODDEF(void)
term_source (j_decompress_ptr cinfo)
{
  /* no work necessary here */
}


GLOBAL(JOCTET*)
jpeg_mem_src_init(j_decompress_ptr cinfo, size_t input_buf_size)
{
	my_src_ptr src;
	if (cinfo->src == NULL){

		cinfo->src =
			(struct jpeg_source_mgr *)(*cinfo->mem->alloc_small)(
				(j_common_ptr) cinfo, JPOOL_PERMANENT,
				SIZEOF(my_source_mgr));
		src = (my_src_ptr) cinfo->src;
		src->buffer =
			(JOCTET *)(*cinfo->mem->alloc_large)(
				(j_common_ptr) cinfo, JPOOL_PERMANENT,
				input_buf_size * SIZEOF(JOCTET));
	}

	src = (my_src_ptr) cinfo->src;
	if (src->inbuf == NULL){
		if((src->inbuf = malloc(sizeof(buf_t)))==NULL){
			ERREXIT(cinfo, JERR_OUT_OF_MEMORY);
		}
	}

	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart;
	src->pub.term_source = term_source;

	return src->buffer;
}


GLOBAL(void)
jpeg_mem_src(j_decompress_ptr cinfo, unsigned char *inbuffer, unsigned long insize)
/*
  unsigned long insize : append for libjpeg8 at 2010/03/30
   I don't know how do I treat this variable
 */
{
	my_src_ptr src;

	if (inbuffer == NULL || insize == 0)  /* Treat empty input as fatal error */
		ERREXIT(cinfo, JERR_INPUT_EMPTY);

	if (cinfo->src == NULL){
		ERREXIT(cinfo, JERR_OUT_OF_MEMORY);
	}
	src = (my_src_ptr) cinfo->src;
	src->pub.bytes_in_buffer = 0;
	src->pub.next_input_byte = NULL;
	src->inbuf->buf = inbuffer;
	src->inbuf->writecnt = insize;
	src->inbuf->readcnt = 0;
}
