#!/usr/bin/perl

use strict;
use warnings;

use lib qw(/usr/mports/Tools/lib);
use Magus;
use Getopt::Std;
use File::Path;
use YAML qw(LoadFile);


main();

=head1 NAME

bless_run.pl -- Promote A Run

=head1 SYNOPSIS

 bless_run.pl -r RUN -m MIRRORLIST -f FTPROOT -a ALIASLIST
 
=head1 DESCRIPTION

A run in magus can be blessed into a index that is used by the mport package
system.  This index contains a list of availible packages, a list of
aliases, and a list of mirrors.  The index is an L<sqlite3> database file that
is compressed with L<bzip2>.

This script will create the index file and copy the bundle files into the 
given ftp root.  The ftp root is organized as such:

  $FTPROOT/$run->arch/$run->osversion/
        index.db.bz2
        bundle1.mport
        bundle2.mport
        ...
=cut

sub main {
  my %opts;
  
  getopts('a:m:f:r:', \%opts) || usage();
  
  unless ($opts{a} && -r $opts{a} && $opts{'m'} && -r $opts{'m'} && $opts{f} && -d $opts{f} && $opts{r}) {
    usage();
  }

  my $run = Magus::Run->retrieve($opts{r}) || die "No such run: $opts{r}\n";

  $run->status eq 'complete' || die "Run is not complete!\n";

  $opts{f} = join('/', $opts{f}, $run->arch, $run->osversion . ".new");
  
  mkpath($opts{f});

  my $index = make_db_file(\%opts, 'index.db');

  build_packages_table(\%opts, $index, $run);
  build_aliases_table(\%opts, $index, $run);
  build_mirror_list(\%opts, $index, $run);
  copy_bundle_files(\%opts, $index, $run);
  
  finish_index(\%opts, $index, 'index.db');  

  move_dirs(\%opts);
}

sub usage {
  (my $self = $0) =~ s:.*/::;
  
  die "Usage: $self -r <runid> -m <mirrorlist file> -a <alias file> -f <ftp root>\n";
}



sub make_db_file {
  my ($opts, $file) = @_;
  
  $file = "$opts->{f}/$file";
  
  my $dbh = DBI->connect("dbi:SQLite:dbname=$file","","", { RaiseError => 1 });
  
  $dbh->do("CREATE TABLE packages (pkg text NOT NULL, version text NOT NULL, comment text NOT NULL, www text NOT NULL, bundlefile text NOT NULL)");
  $dbh->do("CREATE UNIQUE INDEX packages_pkg ON packages (pkg)");
  
  $dbh->do("CREATE TABLE categories (pkg text NOT NULL, category text NOT NULL)");
  $dbh->do("CREATE INDEX categories_pkg ON categories (pkg, category)");

  $dbh->do("CREATE TABLE aliases (alias text NOT NULL, pkg text NOT NULL)");
  $dbh->do("CREATE UNIQUE INDEX aliases_als ON aliases (alias)");
  
  $dbh->do("CREATE TABLE mirrors (mirror text NOT NULL, country text NOT NULL)");
  
  return $dbh;
}
  

sub build_packages_table {
  my ($opts, $index, $run) = @_;

  my $ports = $run->ports;
  
  $index->begin_work;
  
  my $sth   = $index->prepare("INSERT INTO packages (pkg, version, comment, www, bundlefile) VALUES (?,?,?,?,?)");
  
  while (my $port = $ports->next) {
    next unless $port->status eq 'pass' || $port->status eq 'warn';
    
    $sth->execute($port->name, $port->version, $port->description, $port->www, $port->bundle_file);
  }
  
  $sth->finish;
  
  $index->commit;
}  
  

sub build_aliases_table {
  my ($opts, $index, $run) = @_;
  
  my $aliases = LoadFile($opts->{a});
  
  $index->begin_work;
  
  my $sth = $index->prepare("INSERT INTO aliases (alias, pkg) VALUES (?, ?)");
  
  while (my ($alias, $pkg) = each %$aliases) {
    $sth->execute($alias, $pkg);
  }
  
  $sth->finish;
  
  $index->commit;
}

sub build_mirror_list {
  my ($opts, $index, $run) = @_;
  
  my $mirrors = LoadFile($opts->{'m'});
  
  $index->begin_work;
  
  my $sth = $index->prepare("INSERT INTO mirrors (mirror, country) VALUES (?,?)");
  
  while (my ($country, $list) = each %$mirrors) {
    foreach my $mirror (@$list) {
      $sth->execute($mirror, $country);
    }
  }
  
  $sth->finish;
  $index->commit;
}


sub finish_index {
  my ($opts, $index, $file) = @_;
  
  $index->disconnect;

  my $cmnd = "bzip2 $opts->{f}/$file";
  
  if (system($cmnd) != 0) {
    die "$cmnd returned non-zero: $?\n";
  }
  
  unlink("$opts->{f}/$file");
}


=head1 BUGS

Many.  Contact ctriv@MidnightBSD.org.

=head1 AUTHOR

Chris Reinhardt <ctriv@midnightbsd.org>

=head1 COPYRIGHT

Copyright (c) 2009 Chris Reinhardt
All rights reserved.

=head1 LICENSE

This software is licensed under the 2-clause BSD license.

=cut

