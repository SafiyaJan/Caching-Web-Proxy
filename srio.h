#ifndef SRIO_H
#define SRIO_H

#include "csapp.h"

#define RIO_BUFSIZE 8192
typedef struct {
    int srio_fd;                /* Descriptor for this internal buf */
    int srio_cnt;               /* Unread bytes in internal buf */
    char *srio_bufptr;          /* Next unread byte in internal buf */
    char srio_buf[RIO_BUFSIZE]; /* Internal buffer */
} srio_t;


/* SRio (SafiyaRobust I/O) package */
ssize_t srio_readn(int fd, void *usrbuf, size_t n);
ssize_t srio_writen(int fd, void *usrbuf, size_t n);
void srio_readinitb(srio_t *rp, int fd); 
ssize_t	srio_readnb(srio_t *rp, void *usrbuf, size_t n);
ssize_t	srio_readlineb(srio_t *rp, void *usrbuf, size_t maxlen);

/* Wrappers for SRio package */
ssize_t SRio_readn(int fd, void *usrbuf, size_t n);
void SRio_writen(int fd, void *usrbuf, size_t n);
void SRio_readinitb(srio_t *rp, int fd); 
ssize_t SRio_readnb(srio_t *rp, void *usrbuf, size_t n);
ssize_t SRio_readlineb(srio_t *rp, void *usrbuf, size_t maxlen);

#endif /* __SRIO_H__ */