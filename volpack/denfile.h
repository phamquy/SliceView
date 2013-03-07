#ifndef DENFILE_H
#define DENFILE_H
#endif

#ifdef __cplusplus
extern "C"{
#endif



unsigned char *read_den(char *filename,int *xptr,int *yptr,int *zptr);
int write_den(char *filename,unsigned char *data, int xlen, int ylen, int zlen);
int read_bytes(FILE *fd, char *buf, int bytecount);
int read_shorts(FILE *fd, short *sbuf, int shortcount, int swap);
int read_words(FILE *fd, int *wbuf, int wordcount, int swap);
int write_bytes(FILE *fd, char *buf, int bytecount);

#ifdef __cplusplus
}
#endif
