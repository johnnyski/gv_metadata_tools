#! 

#---------------------------------------------------------------
# By: John H. Merritt
#     SM&A Corp.
#     John.Merritt@smawins.com
#     John.H.Merritt@gsfc.nasa.gov
#     6/24/99
#---------------------------------------------------------------
# PURPOSE:
#
# Modify all *.HDF and *.hdf, with .gz or .Z extensions, metadata
# using the program 'modify_vdata'.  All work is done in the current
# directory.
#
#---------------------------------------------------------------
# LICENSE
#
# GPL
#
#---------------------------------------------------------------

opendir(DIR, '.');
@allfiles = readdir(DIR);
closedir(DIR);

@options = @ARGV;
if (@options == "") {
	print STDERR "Usage: modify_all_hdf.pl options\n";
	print STDERR "\n";
	print STDERR "Where 'options' are those from 'modify_vdata'.\n";
	system("modify_vdata");
	exit(-1);
}

# Add double quotes around strings w/ spaces.  The shell has stripped
# off any double quotes from the command line.  Reverse quoting if double
# quotes are part of the string -- use single quotes.

for($i; $i<=$#options; $i++) {
	if ($options[$i] =~ /.*\s+.*/) {
		if ($options[$i] =~ /.*\'+.*/) {
            $options[$i] = sprintf("\"%s\"", $options[$i]);
        } else {
            $options[$i] = sprintf("\'%s\'", $options[$i]);
        }
	}
}

foreach (@allfiles) {
	# next if not *.HDF, *.hdf with or without the .gz or .Z extension. 
	next if !/.*\.[Hh][Dd][Ff](\.gz||\.Z)?$/;
#	print "\$_ is $_\n";
    if (/.*\.gz$/) {
		$compress_prog   = "gzip -f -1";
		$uncompress_prog = "gunzip -f";
		($file = $_) =~ s/(.*)(\.gz)/$1/;
	} elsif  (/.*\.Z$/) {
		$compress_prog   = "compress -f";
		$uncompress_prog = "uncompress -f";
		($file = $_) =~ s/(.*)(\.Z)/$1/;
	} else {
		$compress_prog   = "";
		$uncompress_prog = "";
		$file = $_;
	}
#	print "Compress w/ $compress_prog and uncompress w/ $uncompress_prog\n";
#	print "FILE = $file\n";

	print STDERR "$uncompress_prog $_\n" if ($uncompress_prog);
	system("$uncompress_prog $_") if ($uncompress_prog);
	print STDERR "modify_vdata @options $file\n";
	system("modify_vdata @options $file");
	print STDERR "$compress_prog $file\n" if ($compress_prog);
	system("$compress_prog $file") if ($compress_prog);
}
