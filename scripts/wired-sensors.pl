#!/usr/bin/perl -w

use 5.005;
use DBI;
use sensors;

$dbh = DBI->connect('dbi:mysql:database=sensors;host=rho','sensors','s3ns0rs') or die "Connection Error: $DBI::errstr\n";

$ins_temp = $dbh->prepare_cached('INSERT INTO sensordata (node_id,temperature) VALUES (?,?)');
die "couldn't prepare query" unless defined $ins_temp;

# FIXME: light
while (($key, $value) = each %sensors) {

	my $id = $sensors{$key}{id};

	for (my $i = 0; $i < 10; $i++) {
		$sensor = `cat /mnt/1-wire/$key/temperature`;
		if ($sensor && int $sensor ne 85) {
			print "node=$id temp=$sensor\n";
			$ins_temp->execute($id, $sensor) or die "failed to execute query: $DBI::errstr\n";
			last;
		}
		sleep 4;
	}

}

$dbh->disconnect;
