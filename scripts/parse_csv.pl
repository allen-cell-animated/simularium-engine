#!/usr/bin/perl -w
# This script is the MVP processing for a CSV file into the Simularium visualization data format

use warnings;
use strict;
use List::Util qw(first);
use File::Path;

my $num_args = $#ARGV + 1;
if ($num_args != 1) {
    print "Please specify a file to load\n";
    exit;
}

my $file_name = $ARGV[0];
print "Parsing file $file_name\n";

my $output_dir="$file_name.simularium";
open(my $data, '<', $file_name) or die $!;

my $firstLine = <$data>;
my @titles = split "," , $firstLine;
my %str_ids;

sub index_in_csv {
    my ($val) = @_;
    my $index = first { $titles[$_] eq "$val" } 0..$#titles;
    return (defined $index ? $index : -1);
}

sub get_val {
    my ($index, $default, @fields) = @_;
    return $index >= 0 ? $fields[$index] : $default;
}

sub alphanumeric_to_int {
    my ($string) = @_;
    if (exists($str_ids{$string})) { return $str_ids{$string}; }
    $str_ids{$string} = keys %str_ids
}

sub frame_to_json {
    my ($number, $time, @data) = @_;
    my $data_str = join(',',@data);
    my $frame_str = "{\n
    \"data\": [
        $data_str
    ],
    \"frameNumber\":$number,
    \"msgType\":1,
    \"time\":$time
}";
    return $frame_str
}

my $t_index = index_in_csv('t');
my $id_index = index_in_csv('id');
my $x_index = index_in_csv('x');
my $y_index = index_in_csv('y');
my $z_index = index_in_csv('z');
my $cr_index = index_in_csv('radius');

my @frame_data;
my $currentFrame = 0;
my $currentTime = -1;

my @frame_bundle;

while(my $line = <$data>){
    chomp $line;
    $line =~ tr/ //ds;
    my @fields = split "," , $line;

    my $vtype = 1000;
    my $t = get_val($t_index, 0, @fields);
    my $id = get_val($id_index, 0, @fields);
    my $id_hash = alphanumeric_to_int($id);

    my $x = get_val($x_index, 0, @fields);
    my $y = get_val($y_index, 0, @fields);
    my $z = get_val($z_index, 0, @fields);

    my $cr = get_val($cr_index, 0, @fields);

    push @frame_data, $vtype;
    push @frame_data, $id_hash;
    push @frame_data, $x;
    push @frame_data, $y;
    push @frame_data, $z;
    push @frame_data, 0; # xrot
    push @frame_data, 0; # yrot
    push @frame_data, 0; # zrot
    push @frame_data, $cr;
    push @frame_data, 0; # no sub-points

    if($currentTime < 0) {
        $currentTime = $t;
    }

    # This assumes the CSV is sorted by time
    if($t != $currentTime) {
        my $frame_json = frame_to_json($currentFrame, $currentTime, @frame_data);
        push @frame_bundle, $frame_json;
        $currentTime = $t;
        $currentFrame++;
        @frame_data = ();
    }
}
close($data);
my $numFrames = $currentFrame + 1;

my $bundle_file="$file_name.simularium";
open(my $fh, '>', "$bundle_file");
my $bundle_data_json = join(',',@frame_bundle);

print $fh "{\n
\"bundleData\": [
    $bundle_data_json
],
\"bundleSize\":$numFrames,
\"bundleStart\":0
}";

close $fh;
