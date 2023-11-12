#!/usr/bin/perl

# This is the section that actually does the fetch. It calls out to carl,
# part of Crypto Ancienne, and does the TLS negotiation and parsing of the
# JSON (very rudimentary, but far faster than a 68K Mac).
#
# Copyright 2023 Cameron Kaiser, All rights reserved.
# Since this script is functionally standalone, we'll say BSD license.
# Cool?

$CARL = "/home/spectre/bin/carl";
$APIBASE = "https://hacker-news.firebaseio.com/v0";

alarm 30;
$SIG{'ALRM'} = sub { exit 255; };

select(STDOUT); $|++;
$x = `${CARL} ${APIBASE}/topstories.json`;
($x =~ /\[([0-9,]+)\]/) && (@arts = split(/,/, $1));
if (!scalar(@arts)) {
	print "could not parse article listing\n";
	exit 255;
}

print STDOUT "as of @{[ scalar localtime ]}\n \n";

$stories = 0;
$i = 0;
for(;;) {
	$x = `${CARL} ${APIBASE}/item/$arts[$i++].json`;
	# escape escaped quotes
	$qx = 0;
	$qx++ while ($x =~ /ESCAPE_QUOTE_SEQUENCE${qx}/);
	$x =~ s/\\"/ESCAPE_QUOTE_SEQUENCE${qx}/g;

	($x =~ /"type":"([^"]+)"/) && ($type = $1);
	next if ($type ne 'story');
	($x =~ /"title":"([^"]+)"/) && ($title = $1);
	($x =~ /"score":(\d+),/) && ($score = 0+$1);
	next if (!$score || !length($title));

	# unescape
	$title =~ s/ESCAPE_QUOTE_SEQUENCE${qx}/"/g;
	print STDOUT "$title ($score)\n";
	last if (++$stories == 10);
}

exit 0;

__DATA__
{"by":"przmk","descendants":40,"id":38144417,"kids":[38145546,38144475,38144869,38144710,38145126,38144743,38145240],"score":182,"time":1699126785,"title":"Bevy 0.12","type":"story","url":"https://bevyengine.org/news/bevy-0-12/"}
