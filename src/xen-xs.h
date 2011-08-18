#ifndef _XEN_XS_H
#define _XEN_XS_H

void xenbus_setup(void);
char * xenstore_read(char *path);
char * xenstore_directory(char *path, u32 *ans_len);
void test_xenstore(void);

#endif
