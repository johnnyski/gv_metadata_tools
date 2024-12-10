/* modify_vdata.c
 * 6/11/1999
 * By: John H. Merritt
 *     SM&A Corp.
 *     John.Merritt@smawins.com
 *     http://www.sma.com
 ************************************************************************
 * 
 * Metadata is stored in Vdata.
 * 1. Find the meta data item to modify.
 * 2. Format for PVL, if necessary.
 * 3. Output to HDF.
 *
 * Usage:
 *
 *   modify_vdata [--help]
 *                --vname  vdata_name
 *                [--vclass vdata_class]
 *                --field field_str
 *                --value value_str
 *                -h  (help)
 *                -v  (verbose)
 *                file.hdf
 *
 * Example:
 *
 *   modify_vdata --vn CoreMetadata.0
 *                --vc Attr0.0
 *                --f QualityAssuranceParameterValue
 *                --v "NOT BEING INVESTIGATED"
 *                file.hdf
 *
 *   Note: For PVL, you can have up to 3 values.  Order is important.
 *         The first is the value, the second is the data location and
 *         it defaults to 'PGE' and the third is the mandatory flag, it
 *         defaults to 'FALSE'.  (Only changing the value is IMPLEMENTED)
 *
 *   Besure to quote ' ' and special characters.
 *
 *   See: 'print_vdata' to list all metadata parameters.
 *
 *-----------------------------------------------------------------------
 *
 * Interface concerns:
 * 
 *    This interface requires 3 things: vdata_name, field, and value.
 *    It is not sufficient to specify only the field and value pair,
 *    as the field is not unique across vdatas.  The uniquness comes
 *    from the pair (vdata_name, field).
 *
 *    TSDIS used a bizaar method to store the Comment1, CoreMetadata.0,
 *    and ArchiveMetadata.0 vdatas.  For the ArchiveMetadata.0 vdata there
 *    is one massive string that stores the actual metadata fields.
 *    Parsing code is needed to disect this string to access individual
 *    fields.  Annoying.
 *
 *    The program/command 'print_vdata' is needed to see what all the
 *    vdata_name, field and values exist.
 *
 *    You can only modify one vdata field and value at a time.  This is
 *    merely the API and should be sufficient for the most part.
 *********************************************************************
 */
/*    
    Copyright (C) 1999  John H. Merritt

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

#include "hdf.h"
#include "vg.h"
#include "gv_metadata_tools.h"

#define NRECORDS 20

void usage(int argc, char **argv)
{
  fprintf(stderr, "%12s (Version %s) [--help]\n\
              --vname  vdata_name\n\
             [--vclass vdata_class]\n\
              --field field_str\n\
              --value value_str\n\
              -h (help)\n\
              -v (verbose)\n\
             file.hdf\n\
\n\
     Where:\n\
\n\
         vdata_name  = A vdata name.  Ex: CoreMetadata.0, ArchiveMetadata.0\n\
         vdata_class = A class name.  Ex: Attr0.0 \n\
         field_str   = Field name in vdata.  Ex: QualityAssuranceParameterValue\n\
         value_str   = Value string   Ex: VERIFIED\n\
         file.hdf    = The input/output HDF file.\n\
\n\
     Only --vname, --field, --value and file.hdf are required.\n\
     Run 'print_vdata' to list all the vdata information in an HDF file.\n\
     Be sure to quote special shell characters: ',\",*,?, and space.\n\
", argv[0], GV_METADATA_TOOLS_VERSION_STR);

  exit(-1);
}


/***********************************************************************/
/*                                                                     */
/*                         process_args                                */
/*                                                                     */
/***********************************************************************/
#include <stdio.h>
#include "getopt.h"
#include <stdlib.h>
#include <string.h>
  /* Any changes to the defaults? */
void  process_args(int argc, char **argv,
				   char **vname, char **vclass,
				   char **field, char **value,
				   char **infile,
				   int *verbose)
{

  int c;
  int long_index = 0;
  static struct option long_options[] =
  {
	{"vname",  1, 0, 0},
	{"vclass", 1, 0, 0},
	{"field", 1, 0, 0},
	{"value", 1, 0, 0},
	{"help", 0, 0, 0},
	{NULL, 0, 0, 0}
  };

  while ((c = getopt_long(argc, argv, "hv", long_options, &long_index)) != -1)
	switch (c) {
	case 0:
	  if (strcmp(long_options[long_index].name, "vname") == 0)
		*vname = (char *)strdup(optarg);
	  else if (strcmp(long_options[long_index].name, "vclass") == 0)
		*vclass = (char *)strdup(optarg);
	  else if(strcmp(long_options[long_index].name, "field") == 0)
		*field = (char *)strdup(optarg);
	  else if(strcmp(long_options[long_index].name, "value") == 0)
		*value = (char *)strdup(optarg);
	  else if(strcmp(long_options[long_index].name, "help") == 0)
		usage(argc, argv);
	  break;
	case 'h': usage(argc, argv); break;
	case 'v': *verbose = 1;      break;
	case '?': usage(argc, argv); break;
	default:  break;
	}

  if (*vname == NULL) usage(argc, argv);
  if (*field == NULL) usage(argc, argv);
  if (*value == NULL) usage(argc, argv);
  if (argc - optind != 1) usage(argc, argv);
  *infile = strdup(argv[optind]);
}
/***********************************************************************/
/*                                                                     */
/*                        make_pvl_entry                               */
/*                                                                     */
/***********************************************************************/
char *make_pvl_entry(char *name, char *value, char *location, char *mandatory)
{
  int len;
  char *new_object;
  char *new_mand;

  if (strcasecmp("true", mandatory) == 0) new_mand = "TRUE";
  else if (strcasecmp("false", mandatory) == 0) new_mand = "FALSE";
  else new_mand = mandatory;

  len = 2*strlen(name) + strlen(value) + strlen(location) + strlen(mandatory);
  len += 3*8; /* leading space */
  len += strlen("OBJECT=") + strlen("END_OBJECT=");
  len += 5; /* \n */
  len += 5; /*;'s*/
  len += strlen("Value=");
  len += strlen("Data_location=");
  len += strlen("Mandatory=");
  len += 2; /* Any additional "" */
  len += 1; /* NULL termination. */
  /* FIXME: check for "" and ' ' in value.  If space, then use "value". */

  new_object = (char *) calloc(len, sizeof(char));
  if (isstring(value) == 0) 
	sprintf(new_object,"\
OBJECT=%s;\n\
        Value=%s;\n\
        Data_location=%s;\n\
        Mandatory=%s;\n\
END_OBJECT=%s;",
		  name, value, location, new_mand, name);
  else /* Add double quotes */
	sprintf(new_object,"\
OBJECT=%s;\n\
        Value=\"%s\";\n\
        Data_location=%s;\n\
        Mandatory=%s;\n\
END_OBJECT=%s;",
		  name, value, location, new_mand, name);
  return new_object;
}

/***********************************************************************/
/*                                                                     */
/*                          combine_pvls                               */
/*                                                                     */
/***********************************************************************/
char *combine_pvls(char **list)
{
  char *new_pvl;  /* Output */
  int i;
  int len;
  char *s, *p; /* String cursor for output. */

  /* Determine the size. */
  for (i=0, len=0; list[i]; i++) {
	len += strlen(list[i]) + 2; /* Add an extra \n beween each PVL entry. */
  }
  /* Now, write the entire list. */
  new_pvl = (char *) calloc(len+1, sizeof(char));
  if (len == 0) return NULL;

  for (i=0, s=new_pvl; list[i]; i++) {
	p = list[i];
	while((*s++=*p++)) continue; /* Copy */
	s--;
	*s++ = '\n';
	*s = '\0';
  }

  return new_pvl;
}
  
  

/***********************************************************************/
/*                                                                     */
/*                          change_pvl                                 */
/*                                                                     */
/***********************************************************************/
char *change_pvl(char *databuf, char *name, char *value)
{
  /* Each PVL has:
   *            NAME          is any string
   *            VALUE         is any string
   *            DATA_LOCATION is PGE
   *            MANDATORY     is FALSE
   */
  char **object_list;
  int i;
  char *object;
  char **parsed_attr;
  char *new_pvl;

  object_list = parse_attr(databuf);
  object = NULL;
  for (i=0; object_list[i]; i++) {
	parsed_attr = parse_attr_object(object_list[i]);
	if (parsed_attr == NULL) continue;

	if (strcasecmp(parsed_attr[0], name) != 0) continue;

	/* We have a match. */
	object = make_pvl_entry(name, value, parsed_attr[2], parsed_attr[3]);
	if (object)	object_list[i] = object;
	break;
  }

  new_pvl = combine_pvls(object_list);
  return new_pvl;
}

/***********************************************************************/
/*                                                                     */
/*                          change_value                               */
/*                                                                     */
/***********************************************************************/
void change_value(int32 vdata_id, char **bufptrs, int *dtype, int nfields,
				  char *meta_param, char *meta_value) 
{
  /* 
   * Change the bufptr[i] value with the new one from 'meta_param' and
   * 'meta_value'.  For strings, set a new size if the meta_value exceeds
   * the old size -- VFfieldisize.
   */

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
  int size;
  char    vdata_name[MAX_NC_NAME];

  for (i=0; i<nfields; i++) {
	fieldname = VFfieldname(vdata_id, i);
	size      = VFfieldisize(vdata_id, i);
	VSgetname(vdata_id, vdata_name);
	/* For PVL the field name is "VALUES" */
	if (strcasecmp(fieldname, meta_param) != 0 &&
		strcasecmp(fieldname, "VALUES") != 0) continue;
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
	  ctype = "%d"; itype = 2;

	} else if (dtype[i] == DFNT_INT8 ||
			   dtype[i] == DFNT_UINT8) {
	  uval.c[0] = (char)*bufptrs[i];
	  ctype = "%d"; itype = 3;

	} else if (dtype[i] == DFNT_FLOAT32) {
      memcpy(&uval.c, bufptrs[i], 4);
	  ctype = "%f"; itype = 4;

	} else if (dtype[i] == DFNT_FLOAT64) {
	  memcpy(&uval.c, bufptrs[i], 8);
	  ctype = "%g"; itype = 5;

	}

	sprintf(fmt, "%s", ctype);
	switch (itype) {
	case 0:
	  /* Is this a PVL?  If so, handle differently. */
	  if (strstr(cval, "OBJECT=")) {
		cval = change_pvl(cval, meta_param, meta_value);
		bufptrs[i] = cval;
	  } else
		bufptrs[i] = strdup(meta_value);
	  if (strlen(bufptrs[i]) > size) {
		fprintf(stderr,
		"OVERFLOW.(change_value) for <%s>...new length %d cannot exceed %d\n", 
				vdata_name, strlen(bufptrs[i]), size);
		exit(-1);
	  }
	  break;
	case 1:	
	  sscanf(meta_value, fmt, &uval.ival);
	  memcpy(bufptrs[i], uval.c, 4);
	  break;
	case 2:	
	  sscanf(meta_value, fmt, &uval.ival);
	  memcpy(bufptrs[i], uval.c, 2);
	  break;
	case 3:	
	  sscanf(meta_value, fmt, &uval.ival);
	  memcpy(bufptrs[i], uval.c, 1);
	  break;
	case 4:	
	  sscanf(meta_value, fmt, &uval.x);
	  memcpy(bufptrs[i], uval.c, 4);
	  break;
	case 5:	
	  sscanf(meta_value, fmt, &uval.d);
	  memcpy(bufptrs[i], uval.c, 8);
	  break;
	}
  }
  return;
}
/***********************************************************************/
/*                                                                     */
/*                           m a i n                                   */
/*                                                                     */
/***********************************************************************/
void main(int argc, char **argv)
{

     char    vdata_name[MAX_NC_NAME], vdata_class[MAX_NC_NAME];
     char    fields[5000];
	 int32   nfields;
     int32   file_id, vdata_id, istat;
     int32   n_records, interlace, vdata_size, vdata_ref;
     int    i;
	 /*     int    new_len;*/
     uint8  *databuf;
	 char **bufptrs;
	 int *dtypes;
	 int *dsize;
	 char *filename;
	 int num;

	 char *meta_param, *meta_value;
	 char *vname, *vclass;

	 int qfound;
	 int verbose = 0;


	 process_args(argc, argv,
				  &vname, &vclass,
				  &meta_param, &meta_value,
				  &filename,
				  &verbose);

	 if (verbose) printf("FILENAME = <%s>\n", filename);

     file_id = Hopen(filename, DFACC_WRITE, 0);

     /* Initialize the Vset interface. */
     istat = Vstart(file_id);
	 if (istat != 0) fprintf(stderr, "Vstart: Unable to find vdata");
     /* 
     * Get the reference number for the first Vdata in 
     * the file. 
     */
     vdata_ref = -1;
	 qfound = 0;
	 while((vdata_ref = VSgetid(file_id, vdata_ref)) != -1) {

	   /* Attach to the first Vdata in read mode. */
	   vdata_id = VSattach(file_id, vdata_ref, "w");
	   memset(fields, '\0', sizeof(fields));
	   
	   /* Get the list of field names. */
	   istat =VSinquire(vdata_id, &n_records, &interlace, 
						fields, &vdata_size, vdata_name);
	   nfields = VFnfields(vdata_id);
	   
	   /* Is this the correct class? */
	   if (vname == NULL || strcasecmp(vdata_name, vname) != 0) { /* Nope. */
		 /* Detach from the Vdata */
		 istat = VSdetach(vdata_id);
		 continue;
	   }
	   qfound = 1;
	   /* Get the class. */
	   istat = VSgetclass(vdata_id, vdata_class);
	   
	   /* Determine the fields that will be read. */
	   istat = VSsetfields(vdata_id, fields);
		   
	   /* Print the Vdata information. */
	   if (verbose) printf("Vdata_name  = <%s> \nVdata_class = <%s>\n", 
			  vdata_name, vdata_class);
	   
	   /* Read the data. */
	   databuf = (uint8 *)calloc(vdata_size*n_records, sizeof(char));

	   istat = VSread(vdata_id, (VOIDP)databuf, n_records, FULL_INTERLACE);
	   if (istat < 0) fprintf(stderr, "VSread: unable to read vdata.\n");

	   bufptrs = (char **)calloc(nfields, sizeof(char *));
	   dtypes = (int *)calloc(nfields, sizeof(int));
	   dsize  = (int *)calloc(nfields, sizeof(int));
	   for (i=0; i<nfields; i++) {
		 num = VSfnattrs(vdata_id, i);

		 /*		    printf("%30s  #attr/size/type in field[%d] is %d/", VFfieldname(vdata_id,i), i, num); */
		 num = VFfieldisize(vdata_id, i);
		 /*		    printf("%d/", num); */
		 dsize[i] = num;
		 bufptrs[i] = (char *)calloc(num*n_records, sizeof(char));
		 num = VFfieldtype(vdata_id, i);
		 /*		    printf("%d\n", num); */
		 dtypes[i] = num;
	   }

	   /*FIXME: pass the field size to the change routine and
                check that there is no overflow during the new
                assignment -- especially true for strings.
				*/
	   /* Use NULL for 'fields' to get all fields, if you don't subset. */
	   istat = VSfpack(vdata_id, _HDF_VSUNPACK,
					   fields, /*IN*/ databuf,
					   vdata_size*n_records, n_records,
					   fields, /*OUT*/(VOIDP)bufptrs);
	   if (istat != 0) fprintf(stderr, "VSfpack: unable to unpack vdata.\n");
	   
	   /* Now, find the particular field within this vname and change its
        * value.
		*/

	   /* Abort (internally) if buffer overrun. */
	   change_value(vdata_id, bufptrs, dtypes, nfields, meta_param, meta_value);

	   if (verbose) print_buf(vdata_id, bufptrs, dtypes, nfields);
	   /*	FIXME:   new_len = pvl_len(bufptrs); */

	   istat = VSfpack(vdata_id, _HDF_VSPACK,
					   fields, /*OUT*/ databuf,
					   vdata_size*n_records, n_records,
					   fields, /*IN*/(VOIDP)bufptrs);
	   if (istat != 0) fprintf(stderr, "VSfpack: unable to pack vdata.\n");

	   istat = VSseek(vdata_id, 0);
	   if (istat < 0) fprintf(stderr, "VSseek: unable to seek vdata.\n");

	   istat = VSwrite(vdata_id, (VOIDP)databuf, n_records, FULL_INTERLACE);
	   if (istat <= 0) fprintf(stderr, "VSwrite: unable to write vdata.\n");

	   free(databuf);
	   databuf = NULL;
	   for (i=0; i<nfields; i++) {
		 free(bufptrs[i]);
	   }
	   free(bufptrs);
	   bufptrs = NULL;
	   
	   /* Detach from the Vdata */
	   istat = VSdetach(vdata_id);
	 }
	 
	 if (qfound == 0) {
	   fprintf(stderr, "Unable to find vdata name <%s>\n\
Run the program 'print_vdata' to list all vdata elements.\n",
			   vname);
	 }
	 /* Call VSfpack ... */

	 /* close the interface and the file. */
     istat = Vend(file_id);
     istat = Hclose(file_id);

}
