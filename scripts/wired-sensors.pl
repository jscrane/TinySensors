#!/usr/bin/perl -w

use 5.005;
use DBI;
use sensors;

$dbh = DBI->connect('dbi:mysql:database=sensors;host=rho','sensors','s3ns0rs') or die "Connection Error: $DBI::errstr\n";

$ins = $dbh->prepare_cached('INSERT INTO sensor_data (node_id,temperature,light) VALUES (?,?,?)');
die "couldn't prepare query" unless defined $ins;

while (($key, $value) = each %sensors) {

	my $id = $sensors{$key}{id};

	for (my $i = 0; $i < 10; $i++) {
		$temp = `cat /mnt/1-wire/$key/temperature`;
		if (!$temp || int $temp eq 85) {
			sleep 4;
		} else {
			my $v = 0;
			if ($sensors{$key}{light}) {
				for (my $j = 0; $j < 3; $j++) {
					$v = `cat /mnt/1-wire/$key/VAD`;
					last if ($v);
					sleep 5;
				}
			}
			$ins->execute($id, $temp, ($v / 5.0) * 255) or die "failed to execute query: $DBI::errstr\n";
			last;
		}
	}
}

$dbh->disconnect;
