#include "hdf.h"
#include "vg.h"

#define GV_METADATA_TOOLS_VERSION_STR "v1.4"

/* Prototype for common routines. */
char **parse_attr(char *input);
int isstring(char *s);
char **parse_attr_object(char *object);
char *find_attr_object(char **list, char *item);
void print_pvl(char *databuf);
void print_buf(int32 vdata_id, char **bufptrs, int *dtype, int nfields) ;
int pvl_len(char **bufptrs);
