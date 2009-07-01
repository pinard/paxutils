/* Returns true if name is too long for archive format.
   Only used when writing.  */
bool (*name_too_long) PARAMS ((char *name));

/* Function to use to write a header.  */
void (*header_writer) PARAMS ((struct new_cpio_header *file_hdr,
			  int out_des));

/* Function to use to indicate that archive is finished.  */
void (*eof_writer) PARAMS ((int out_des));
