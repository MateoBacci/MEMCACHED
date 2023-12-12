/* Macro READ */
#define READ(fd, buf, n) ({						\
	int rc = read(fd, buf, n);					\
	if (rc == 0)							\
		return DISCONNECT;						\
	if (rc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))	\
		rc = 0;						\
	rc; })

/* Macro READ binario */
#define READB(ci, fd, tbuf, n) ({					\
	int __n = ci->bl - ci->bo;				\
	if (__n == 0) {							\
		int rc = READ(fd, ci->buf, sizeof(ci->buf));	\
		ci->bo = 0;						\
		ci->bl = rc;					\
		__n = rc;						\
	}								\
	int __m = __n > n ? n : __n;					\
	memcpy(tbuf, ci->buf + ci->bo, __m);			\
	ci->bo += __m;						\
	__m; })

	