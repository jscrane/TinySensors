#!/usr/bin/perl -w

#use strict;
#use warnings;
use IO::Select;
use IO::Socket;
#use Time::HiRes qw(time);

use sensors;

my $listen = IO::Socket::INET->new(LocalPort => 5555, Type => SOCK_STREAM, 
		Reuse => 1, Listen => 10) 
	or die "Failed to create listener\n";

my $rin = '';
my $rout;
vec($rin, fileno($listen), 1) = 1;

my $timeout = 30;
my $nfound;
my @conns;

$SIG{PIPE} = 'IGNORE';

while (1) {
	($nfound, $timeout) = select($rout=$rin, undef, undef, $timeout);

	if ($nfound > 0) { 
		my $conn = $listen->accept;
		$conn->autoflush(1);
		push(@conns, $conn);
	} else {
		$timeout = 30; 
		my $temp = '';
		my $light = '';
		my $now = time;
		my $v = '';
		while ($device = each %sensors) {
			my $id = $sensors{$device}{id};
			for (my $i = 0; $i < 5; $i++) {
				$t = `cat /mnt/1-wire/$device/temperature`;
				if ($t && int $t ne 85) {
					$temp = $t + 0.0;
					last;
				}
				sleep 2;
			}
			if ($sensors{$device}{light}) {
				for (my $i = 0; $i < 5; $i++) {
					$v = `cat /mnt/1-wire/$device/VAD`;
					if ($v) {
						if ($v > "5" || $v < "0") {
							$v = 0;
						}
						$light = int $v * 50;
						last;
					}
					sleep 2;
				}
			}
			my $data = ",$id,,$light,$temp,,,,,$now\n";
			for (my $c=0; $c < @conns; $c++) {
				my $conn = $conns[$c];
				if ($conn && ! print $conn $data ) {
					$conn->close;
					delete $conns[$c];
				}
			}
		}
	}
}
