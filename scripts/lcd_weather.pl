#!/usr/bin/perl -w

use sensors;

use 5.005;
#use strict;
use Getopt::Std;
use IO::Socket;
use IO::Select;
use Fcntl;
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

# set variables
$SERVER = defined($opt{s}) ? $opt{s} : $SERVER;
$PORT = defined($opt{p}) ? $opt{p} : $PORT;


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

for $wid ("time", "temp", "wind", "astro") {
	lcdproc $remote, "widget_add weather $wid string";
}

# NOTE: You have to ask LCDd to send you keys you want to handle
lcdproc $remote, "client_add_key Enter";

$SIG{ALRM} = sub {
	lcdproc $remote, "backlight off";
};

$rin = '';
vec($rin, fileno($remote), 1) = 1;
alarm $LIGHT;
sleep 5;
while (1) {

	my $w = do $WEATHER;

	$temp = "$w->{current_observation}->{temp_c}";
	$chill = "$w->{current_observation}->{feelslike_c}";
	$text = "$w->{current_observation}->{weather}";
	$wind = "$w->{current_observation}->{wind_degrees}";
	$speed = "$w->{current_observation}->{wind_kph}";
	$sunrise_h = "$w->{sun_phase}->{sunrise}->{hour}";
	$sunrise_m = "$w->{sun_phase}->{sunrise}->{minute}";
	$sunset_h = "$w->{sun_phase}->{sunset}->{hour}";
	$sunset_m = "$w->{sun_phase}->{sunset}->{minute}";
	$temp_unit = "C";
	$press_unit = "mb";
	$speed_unit = "km/h";
	$pressure = "$w->{current_observation}->{pressure_mb}";
	$rising = "$w->{current_observation}->{pressure_trend}";
	if ($rising == "1") { $rchange = "+"; }
        elsif ($rising == "0") { $rchange = " "; }
	else { $rchange = "-"; }
	$humidity = "$w->{current_observation}->{relative_humidity}";
	$pressure = ceil($pressure);
	$speed = ceil($speed);
	$precip = "$w->{current_observation}->{precip_1hr_metric}";
	$epoch = "$w->{current_observation}->{observation_epoch}";
	$now = strftime("%a %b %e %H:%M", localtime($epoch));

	$ip = int($precip);
	$line1 = sprintf("%16s %dmm", $now, $ip < 0? 0: $ip);
        $ftmp = sprintf("%.0f%.1s", $temp, $temp_unit);
        $ltext = sprintf("%*s", length($ftmp) - 20, $text);
	$line2 = sprintf("%.*s %s", 19 - length($ftmp), $ltext, $ftmp);
	$line3 = sprintf("%2s:%2s %2s:%2s %4s%3.0f%.1s", $sunrise_h, $sunrise_m, $sunset_h, $sunset_m, $humidity, $chill, $temp_unit);
	$line4 = sprintf("%4s%.2s%.1s %3s  %3s%.4s", $pressure, $press_unit, $rchange, wind_direction($wind), $speed, $speed_unit);

	lcdproc $remote, "widget_set weather time 1 1 {$line1}";
	lcdproc $remote, "widget_set weather temp 1 2 {$line2}";
	lcdproc $remote, "widget_set weather wind 1 3 {$line3}";
	lcdproc $remote, "widget_set weather astro 1 4 {$line4}";

	$timeleft = 60;
	while ($timeleft > 0) {
		($nfound, $timeleft) = select($rout=$rin, undef, undef, $timeleft);
		if ($nfound > 0) {
print "awake $timeleft\n";
			my $input = <$remote>;
			if (! defined($input) ) {
				last;
			}
print "input is: $input";
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
