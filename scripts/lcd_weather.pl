#!/usr/bin/perl -w

use sensors;

use 5.005;
use strict;
use Getopt::Std;
use IO::Socket;
use IO::Select;
use POSIX qw(strftime ceil);
use Date::Parse;

############################################################
# Configurable part. Set it according your setup.
############################################################

# Host which runs LCDproc daemon (LCDd)
my $SERVER = "pitv3";

# Port on which LCDd listens to requests
my $PORT = "13666";

my $WEATHER = "/var/tmp/weather.pl";

my $LIGHT = 20;

############################################################
# End of user configurable parts
############################################################

my $width = 20;
my $lines = 4;

my $progname = $0;
   $progname =~ s#.*/(.*?)$#$1#;

# declare functions
sub error($@);
sub usage($);
sub lcdproc;
sub wind_direction;

## main routine ##
my %opt = ();
getopts("s:p:w:", \%opt);

# set variables
$SERVER = defined($opt{s}) ? $opt{s} : $SERVER;
$PORT = defined($opt{p}) ? $opt{p} : $PORT;
$WEATHER = defined($opt{w})? $opt{f} : $WEATHER;

# Connect to the server...
my $remote = IO::Socket::INET->new(
		Proto     => 'tcp',
		PeerAddr  => $SERVER,
		PeerPort  => $PORT,
	)
	or  error(1, "cannot connect to LCDd daemon at $SERVER:$PORT");

# Make sure our messages get there right away
$remote->autoflush(1);

my $read_set = new IO::Select();
$read_set->add($remote);

sleep 1;	# Give server plenty of time to notice us...

my $lcdresponse = lcdproc $remote, "hello";
print $lcdresponse;

# get width & height from server's greet message
if ($lcdresponse =~ /\bwid\s+(\d+)\b/) {
	$width = 0 + $1;
}	
if ($lcdresponse =~ /\bhgt\s+(\d+)\b/) {
	$lines = 0 + $1;
}	

lcdproc $remote, "client_set name {$progname}";
lcdproc $remote, "screen_add weather";
lcdproc $remote, "screen_set weather name {Weather}";

for my $wid ("time", "temp", "wind", "astro") {
	lcdproc $remote, "widget_add weather $wid string";
}
lcdproc $remote, "widget_add weather description scroller";

# NOTE: You have to ask LCDd to send you keys you want to handle
lcdproc $remote, "client_add_key Enter";

$SIG{ALRM} = sub {
	lcdproc $remote, "backlight off";
};

my $rin = '';
vec($rin, fileno($remote), 1) = 1;
alarm $LIGHT;
sleep 5;
while (1) {

	my $w = do $WEATHER;

	my $text = $w->{weather}->{value};
	my $temp = $w->{temperature}->{value};
	my $chill = $w->{temperature}->{min};
	my $temp_unit = "C";

	my $wind = $w->{wind}->{direction}->{value};
	my $speed = ceil(3.6 * $w->{wind}->{speed}->{value});
	my $speed_unit = "km/h";

	my $sr = str2time($w->{city}->{sun}->{rise});
	my $sunrise = strftime("%H:%m", localtime($sr));
	my $ss = str2time($w->{city}->{sun}->{set});
	my $sunset = strftime("%H:%m", localtime($ss));

	my $pressure = $w->{pressure}->{value};
	my $press_unit = "mb";

	# pressure trend n/a
	my $rchange = " ";

	my $humidity = $w->{humidity}->{value};
	my $humidity_unit = $w->{humidity}->{unit};

	my $epoch = str2time($w->{lastupdate}->{value});
	my $now = strftime("%a %b %e %H:%M", localtime($epoch));
	my $precip = "0";
	if (exists $w->{precipitation}->{value}) { 
		$precip = "$w->{precipitation}->{value}"; 
	}

	my $line1 = sprintf("%16s %dmm", $now, $precip);
        my $ftmp = sprintf("%.0f%.1s", $temp, $temp_unit);
	my $tlen = length($ftmp);
	my $tpos = $width - $tlen + 1;
	my $dlen = $width - $tlen;
	my $line3 = sprintf("%2s %2s %3s%.1s%3.0f%.1s", $sunrise, $sunset, $humidity, $humidity_unit, $chill, $temp_unit);
	my $line4 = sprintf("%4s%.2s%.1s %3s  %3s%.4s", $pressure, $press_unit, $rchange, wind_direction($wind), $speed, $speed_unit);

	lcdproc $remote, "widget_set weather time 1 1 {$line1}";
	lcdproc $remote, "widget_set weather description 1 2 {$dlen} 2 h 8 {$text}";
	lcdproc $remote, "widget_set weather temp {$tpos} 2 {$ftmp}";
	lcdproc $remote, "widget_set weather wind 1 3 {$line3}";
	lcdproc $remote, "widget_set weather astro 1 4 {$line4}";

	my $timeleft = 60;
	while ($timeleft > 0) {
		(my $nfound, $timeleft) = select($rin, undef, undef, $timeleft);
		if ($nfound > 0) {
			my $input = <$remote>;
			if (! defined($input) ) {
				last;
			}
			if ($input eq "key Enter\n") {
				lcdproc $remote, "backlight on";
				alarm $LIGHT;
			}
		}
	}
}

close ($remote)  or  error(1, "close() failed");
exit;


## print out error message and eventually exit ##
# Synopsis:  error($status, $message)
sub error($@)
{
my $status = shift;
my @msg = @_;

  print STDERR $progname . ": " . join(" ", @msg) . "\n";

  exit($status)  if ($status);
}


## print out usage message and exit ##
# Synopsis:  usage($status)
sub usage($)
{
my $status = shift;

  print STDERR "Usage: $progname [<options>] <file>\n";
  if (!$status) {
    print STDERR "  where <options> are\n" .
                 "    -s <server>                connect to <server> (default: $SERVER)\n" .
                 "    -p <port>                  connect to <port> on <server> (default: $PORT)\n" .
		 "    -h                         show this help page\n" .
		 "    -V                         display version number\n";
  }
  else {
    print STDERR "For help, type: $progname -h\n";
  }  

  exit($status);
}

sub wind_direction($)
{
	my $deg = shift;

	if ($deg <= 11.25 || $deg > 348.75) { return "N"; }
	if ($deg > 11.25 && $deg <= 33.75) { return "NNE"; }
	if ($deg > 33.75 && $deg <= 56.25) { return "NE"; }
	if ($deg > 56.25 && $deg <= 78.75) { return "ENE"; }
	if ($deg > 78.75 && $deg <= 101.25) { return "E"; }
	if ($deg > 101.25 && $deg <= 123.75) { return "ESE"; }
	if ($deg > 123.75 && $deg <= 146.25) { return "SE"; }
	if ($deg > 146.25 && $deg <= 168.75) { return "SSE"; }
	if ($deg > 168.75 && $deg <= 191.25) { return "S"; }
	if ($deg > 191.25 && $deg <= 213.75) { return "SSW"; }
	if ($deg > 213.75 && $deg <= 236.25) { return "SW"; }
	if ($deg > 236.25 && $deg <= 258.75) { return "WSW"; }
	if ($deg > 258.75 && $deg <= 281.25) { return "W"; }
	if ($deg > 281.25 && $deg <= 303.75) { return "WNW"; }
	if ($deg > 303.75 && $deg <= 326.25) { return "NW"; }
	if ($deg > 326.25 && $deg <= 348.75) { return "NNW"; }
}

sub lcdproc
{
	my $fd = shift;
	my $cmd = shift;

	print $fd "$cmd\n";
	return <$fd>;
}
