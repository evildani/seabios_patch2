#ifndef _XEN_XS_H
#define _XEN_XS_H

char * add_string(char * a,char *b);
char * strconcat(char *dest, const char *src);
void xenbus_setup(void);
char * xenstore_read(char *path);
char * xenstore_directory(char *path, u32 *ans_len);
void test_xenstore(void);

#endif
