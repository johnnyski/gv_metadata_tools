/* print_metadata.c
 * 
 * Metadata is stored in Vdata.
 * 1. Read each Vdata and output the name and value.
 *    There are two types of Vdata that TSDISTK uses:
 *    a. HDF fields.
 *    b. A string containing multiple values (OBJECTS).
 *       This must be parsed.  Two parsing methods are
 *       required:
 *
 *        i. If Vdata name  == Comment1.
 *       ii. If Vdata class == Attr0.0
 *
 */

#include "hdf.h"
#include "vg.h"
#include "gv_metadata_tools.h"

#define NRECORDS 20

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
     uint8  *databuf;
	 char **bufptrs;
	 int *dtypes;
	 char *filename;
	 int num;


	 printf("PROGRAM = <%s (Version %s)>\n", argv[0], GV_METADATA_TOOLS_VERSION_STR);
     /* Open the HDF file. */
	 if (argc == 2) filename = strdup(argv[1]);
	 else filename = "VD_Ex4.hdf";
	 printf("FILENAME = <%s>\n", filename);

     file_id = Hopen(filename, DFACC_READ, 0);

     /* Initialize the Vset interface. */
     istat = Vstart(file_id);

     /* 
     * Get the reference number for the first Vdata in 
     * the file. 
     */
     vdata_ref = -1;

	 while((vdata_ref = VSgetid(file_id, vdata_ref)) != -1) {

     /* Attach to the first Vdata in read mode. */
     vdata_id = VSattach(file_id, vdata_ref, "r");
	 memset(fields, '\0', sizeof(fields));

     /* Get the list of field names. */
     istat =VSinquire(vdata_id, &n_records, &interlace, 
            fields, &vdata_size, vdata_name);
	 nfields = VFnfields(vdata_id);
     
	 /*
     printf("nfields: %d, fields: <%s>, n_records: %d, vdata_size: %d\n",
              nfields, fields, n_records, vdata_size);
			  */
     /* Get the class. */
     istat = VSgetclass(vdata_id, vdata_class);

     /* Determine the fields that will be read. */
     istat = VSsetfields(vdata_id, fields);

     /* Print the Vdata information. */
     printf("Vdata_name  = <%s> \nVdata_class = <%s>\n", 
             vdata_name, vdata_class);

     /* Read the data. */
	 databuf = (uint8 *)calloc(vdata_size*n_records, sizeof(char));

     istat = VSread(vdata_id, (VOIDP)databuf, n_records,
                     FULL_INTERLACE);


	 bufptrs = (char **)calloc(nfields, sizeof(char *));
	 dtypes = (int *)calloc(nfields, sizeof(int));
	 for (i=0; i<nfields; i++) {
	   num = VSfnattrs(vdata_id, i);

	   /*	   printf("%30s  #attr/size/type in field[%d] is %d/", VFfieldname(vdata_id,i), i, num); */
	   num = VFfieldisize(vdata_id, i);
	   /*	   printf("%d/", num); */
	   bufptrs[i] = (char *)calloc(num*n_records, sizeof(char));
	   num = VFfieldtype(vdata_id, i);
	   /*	   printf("%d\n", num); */
	   dtypes[i] = num;
	 }

	 /* Use NULL for 'fields' to get all fields, if you don't subset. */
	 istat = VSfpack(vdata_id, _HDF_VSUNPACK,
					 fields, /*IN*/ databuf,
					 vdata_size*n_records, n_records,
					 fields, /*OUT*/(VOIDP)bufptrs);

	 /*	 printf ("istat = %d ... Call print_buf <", istat); */
	 /*
	 for (i=0; i<50; i++)
	   printf("%x", databuf[i]);
	 printf (">\n");
	 */
	 print_buf(vdata_id, bufptrs, dtypes, nfields);

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

	 /* Call VSfpack ... */

	 /* close the interface and the file. */
     istat = Vend(file_id);
     istat = Hclose(file_id);

}
