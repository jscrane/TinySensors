#!/usr/bin/perl -w

use LWP::Simple;
use XML::Simple;
use Data::Dumper;
use lib qw(/usr/local/rrdtool-1.2.12/lib/perl);
use RRDs;

$KEY = "5a33baa5f85d1e436a14a25815655f76";
$WS = "Rathgar,IE";
$rrd = '/var/lib/rrd';
$img = '/var/www/rrdtool';
$out = '/var/tmp/weather.pl';

$xml = new XML::Simple;
$content = get("http://api.openweathermap.org//data/2.5/weather?q=$WS&units=metric&appid=$KEY&mode=xml");
$data = $xml->XMLin($content);

open my $fh, '>', $out;
print $fh Dumper($data);
close $fh;

$temp = "$data->{temperature}->{value}";
$chill = "$data->{temperature}->{min}";
$direction = "$data->{wind}->{direction}->{value}";
$speed = 3.6 * "$data->{wind}->{speed}->{value}";	# m/s
$pressure = "$data->{pressure}->{value}";
$humidity = "$data->{humidity}->{value}";

print "$temp, $chill, $direction, $speed, $pressure, $humidity\n";

if (! -e "$rrd/weather2.rrd" ) {
	RRDs::create "$rrd/weather2.rrd",
		"-s 3600",
		"DS:temp:GAUGE:7200:-273:200",
		"DS:chill:GAUGE:7200:-273:200",
		"DS:direction:GAUGE:7200:0:360",
		"DS:speed:GAUGE:7200:0:150",
		"DS:pressure:GAUGE:7200:0:2000",
		"DS:humidity:GAUGE:7200:0:100",
		"RRA:AVERAGE:0.5:1:168",
		"RRA:MIN:0.5:1:168",
		"RRA:MAX:0.5:1:168",
		"RRA:LAST:0.5:1:168",
                "RRA:AVERAGE:0.5:12:3650",
                "RRA:MIN:0.5:12:3650",
                "RRA:MAX:0.5:12:3650";
	if ($ERROR = RRDs::error) { 
		print "$0: failed to create weather2.rrd: $ERROR\n"; 
	}
}
RRDs::update "$rrd/weather2.rrd", "N:$temp:$chill:$direction:$speed:$pressure:$humidity";
if ($ERROR = RRDs::error) { 
	print "$0: failed to insert weather data into rrd: $ERROR\n"; 
}

foreach ('day', 'week', 'month', 'year') {
        RRDs::graph "$img/temp-outside-$_.png",
                "-s -1$_",
                "-t outside temperature :: last $_",
                "--lazy",
                "-h", "80", "-w", "600",
                "-a", "PNG",
                "-v degrees C",
                "--slope-mode",
                "DEF:temp=$rrd/weather2.rrd:temp:AVERAGE",
                "DEF:chill=$rrd/weather2.rrd:chill:AVERAGE",
                "LINE2:chill#FF0000:chill",
                "LINE2:temp#0000FF:temp",
                "GPRINT:temp:MAX:    Max\\: %4.1lf",
                "GPRINT:temp:AVERAGE: Avg\\: %4.1lf",
                "GPRINT:temp:MIN: Min\\: %4.1lf",
                "GPRINT:temp:LAST: Now\\: %4.1lf\\n";
        if ($ERROR = RRDs::error) { 
		print "$0: unable to generate weather $_ graph: $ERROR\n"; 
	}
        RRDs::graph "$img/wind-$_.png",
                "-s -1$_",
                "-t wind-speed :: last $_",
                "--lazy",
                "-h", "80", "-w", "600",
                "-a", "PNG",
                "-v km/h",
                "--slope-mode",
                "DEF:speed=$rrd/weather2.rrd:speed:AVERAGE",
                "LINE2:speed#0000FF:speed",
                "GPRINT:speed:MAX:    Max\\: %4.1lf",
                "GPRINT:speed:AVERAGE: Avg\\: %4.1lf",
                "GPRINT:speed:MIN: Min\\: %4.1lf",
                "GPRINT:speed:LAST: Now\\: %4.1lf\\n";
        if ($ERROR = RRDs::error) { 
		print "$0: unable to generate weather $_ graph: $ERROR\n"; 
	}
}
