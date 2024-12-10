#include "gv_metadata_tools.h"

/***********************************************************************/
/*                                                                     */
/*                       parse_attr                                    */
/*                                                                     */
/***********************************************************************/
char **parse_attr(char *input)
{
  /* Each object looks like:

OBJECT=AlgorithmVersion;
        Value="1.0";
  	    Data_Location=PGE;
        Mandatory=FALSE;
END_OBJECT=AlgorithmVersion;

  Description:
  This function takes a Attr0.0 vdata class contained in 'input',
  which is a long list of objects, and parses it.
  The return is an array of pointers to strings, one
  per object.  The last pointer will be set to NULL.

*/

  int nobjects;
  char *s, *p;
  char *object_begin = "OBJECT=";
  char *object_end   = "END_OBJECT=";
  char **object_list;
  int i;

  /* Count the number of objects. */
  nobjects = 0;
  s = input;
  
  while ((s = strstr(s, object_end)) != NULL) {
	nobjects++;
	s += strlen(object_end);
  }

  /*  printf("NUMBER OF OBJECTS = %d\n", nobjects); */

  /* Now, allocate two more pointer to objects.
   * One is the 'END;' object and the last is the NULL termination.
   */
  object_list = (char **) calloc(nobjects+2, sizeof(char *));

  /* Traverse 'input' and peel off the objects.  Size is known from
   * tracking it from 'input'.  Now we look for 'OBJECT= ... END_OBJECT...;'
   * The string for each object will be from *s to *p.
   */
  s = input;

  for(i=0; i<nobjects; i++) {
	s = p = strstr(s, object_begin);
	p += strlen(object_begin);
	p = strstr(p, object_end);
	/* look for the ';' */
	while(*p && *p != ';') p++;
	if (*p == '\0') break; /* Beyond object ... error, return what we've got */
	object_list[i] = (char *) calloc(p-s+2, sizeof(char));
	memmove(object_list[i], s, p-s+1);
	/*	printf("OBJECT FOUND = <\n%s>\n", object_list[i]); */
	s = p;
  }

  object_list[nobjects] = strdup("END;");

  return object_list;
}

/***********************************************************************/
/*                                                                     */
/*                          isstring                                   */
/*                                                                     */
/***********************************************************************/
int isstring(char *s)
{
  while(*s) {
	if (isalpha(*s) || isspace(*s)) return 1;
	s++;
  }
  return 0;
}

/***********************************************************************/
/*                                                                     */
/*                       parse_attr_object                             */
/*                                                                     */
/***********************************************************************/

char **parse_attr_object(char *object)
{
  /* Create a 4 element array of strings: name, value, location, mandatory */
  char **parse;
  char *s, *p; /* pointers into object string. */
  int i;

  if (object == NULL) return NULL;
  if (strstr(object, "END;") != 0) return NULL;
  parse = (char **) calloc(4, sizeof(char *));
  s = object;
  for (i=0; i<4; i++) {
	s = strstr(s, "="); s++; p = strstr(s, ";");
	parse[i] = (char *) calloc(p-s+1, sizeof(char));
	memcpy(parse[i], s, p-s);
	s = p;
  }
  return parse;
}

/***********************************************************************/
/*                                                                     */
/*                        find_attr_object                             */
/*                                                                     */
/***********************************************************************/

char *find_attr_object(char **list, char *item)
{
  char *new_object;

  if (list == NULL) return NULL;

  while(*list) {
	if (strstr(*list, item) != NULL) {
	  new_object = strdup(*list);
	  return new_object;
	}
	list++;
  }
  return NULL;
}


/***********************************************************************/
/*                                                                     */
/*                          print_pvl                                  */
/*                                                                     */
/***********************************************************************/
void print_pvl(char *databuf)
{
  char **object_list;
  int i;
  char **parsed_attr;

  object_list = parse_attr(databuf);
  /*  printf ("databuf = <%s>\n", databuf); */
  /*  printf ("Name....................................  ...............Value .............Location ............Mandatory\n"); */
  for (i=0; object_list[i]; i++) {
	parsed_attr = parse_attr_object(object_list[i]);
	if (parsed_attr) {
	  printf("%40s  %20s  %20s  %20s\n",
			 parsed_attr[0],
			 parsed_attr[1],
			 parsed_attr[2],
			 parsed_attr[3]);
	} else {
	  printf("%s\n", object_list[i]);
	}
  }
}
	 /*
Data Type       Data Type Flag and Value      Description
 char8          DFNT_CHAR8 (4)                8-bit character type
 uchar8         DFNT_UCHAR8 (3)               8-bit unsigned character type
 int8           DFNT_INT8 (20)                8-bit integer type
 uint8          DFNT_UINT8 (21)               8-bit unsigned integer type
 int16          DFNT_INT16 (22)               16-bit integer type
 uint16         DFNT_UINT16 (23)              16-bit unsigned integer type
 int32          DFNT_INT32 (24)               32-bit integer type
 uint32         DFNT_UINT32 (25)              32-bit unsigned integer type
 float32        DFNT_FLOAT32 (5)              32-bit floating-point type
 float64        DFNT_FLOAT64 (6)              64-bit floating-point type
 */
/***********************************************************************/
/*                                                                     */
/*                           print_buf                                 */
/*                                                                     */
/***********************************************************************/
void print_buf(int32 vdata_id, char **bufptrs, int *dtype, int nfields) 
{
  int i;
  char *ctype = (char *)calloc(3, sizeof(char)); /* %d, %f, etc */
  char fmt[100];
  char *cval;
 /* Context sensitive: printing str, int and float. */
  union {
	int ival;
	char c[8];
	float x;
	double d;
  } uval;
  char *fieldname;

  int itype;

  for (i=0; i<nfields; i++) {
	fieldname = VFfieldname(vdata_id, i);
	uval.ival = 0;
	if (dtype[i] == DFNT_CHAR8 ||
		dtype[i] == DFNT_UCHAR8) {
	  ctype = "%s"; itype = 0;
	  cval = bufptrs[i];
	  uval.ival = (int) cval;  /* I don't know length ;) */
	} else if (dtype[i] == DFNT_INT32 ||
			   dtype[i] == DFNT_UINT32) {
	  memcpy(&uval.c, bufptrs[i], 4);
	  ctype = "%d"; itype = 1;
	} else if (dtype[i] == DFNT_INT16 ||
			   dtype[i] == DFNT_UINT16) {
	  memcpy(&uval.c[0], bufptrs[i], 2);
	  ctype = "%d"; itype = 1;
	} else if (dtype[i] == DFNT_INT8 ||
			   dtype[i] == DFNT_UINT8) {
	  uval.c[0] = (char)*bufptrs[i];
	  ctype = "%d"; itype = 1;
	} else if (dtype[i] == DFNT_FLOAT32) {
      memcpy(&uval.c, bufptrs[i], 4);
	  ctype = "%f"; itype = 2;
	} else if (dtype[i] == DFNT_FLOAT64) {
	  memcpy(&uval.c, bufptrs[i], 8);
	  ctype = "%g"; itype = 3;
	}

	sprintf(fmt, "%30s = <%s>", fieldname, ctype);
	switch (itype) {
	case 0:
	  /* Is this a PVL?  If so, handle differently. */
	  if (strstr(cval, "OBJECT="))
		print_pvl(cval);
	  else
		printf(fmt, uval.ival);
	  break;
	case 1:	printf(fmt, uval.ival); break;
	case 2: printf(fmt, uval.x);    break;
	case 3: printf(fmt, uval.d);    break;
	}
	printf("\n");
  }
  return;
}

/***********************************************************************/
/*                                                                     */
/*                           pvl_len                                   */
/*                                                                     */
/***********************************************************************/
int pvl_len(char **bufptrs)
{
  int i, len;

  len = 0;

  for (i=0; bufptrs[i]; i++) {
	len += strlen(bufptrs[i]);
  }

  return len;
}
