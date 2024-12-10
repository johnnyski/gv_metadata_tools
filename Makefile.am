AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(prefix)/include

bin_PROGRAMS = modify_vdata print_vdata 
bin_SCRIPTS  = $(scripts)
scripts = modify_all_hdf.pl

modify_vdata_SOURCES = modify_vdata.c misc.c $(build_headers) getopt.c getopt1.c getopt.h
print_vdata_SOURCES = print_vdata.c misc.c $(build_headers)

build_headers = gv_metadata_tools.h

gv_metadata_tools.h: Makefile
	@for p in $(build_headers) $(scripts); do \
	   echo -n "Checking file $$p ... ";\
	   cp $$p $$p.in;\
	   sed -e 's/\$PROG_VERSION[ ]*=.*/$PROG_VERSION = \"$(PACKAGE)-$(VERSION)\";/' \
         -e 's/GV_METADATA_TOOLS_VERSION_STR.*/GV_METADATA_TOOLS_VERSION_STR \"$(VERSION)\"/' \
		-e 's&#\!.*perl&#\! $(perlbin)&' \
		-e 's&(\@INC,.*&(\@INC, \"$(bindir)\"\)\;&' \
	   < $$p.in > $$p.new;\
	   if cmp -s $$p $$p.new; then \
          rm $$p.new; \
          echo "$$p remains untouched."; \
       else \
          mv $$p.new $$p; \
          echo "substitutions made in $$p."; \
       fi;\
	   rm -f $$p.in;\
    done


EXTRA_DIST = $(bin_SCRIPTS) CHANGES GPL 