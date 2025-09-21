#/usr/bin/perl
use strict;
use warnings;


&Main();
#&MainTest();




=pod

=cut

use constant c_sym_table => <<EOF;
/*
FILE: FILENAME -- C Style Symbol Table
This file was created at DATETIME

Auto-generated source. Don't change it manually.
Contact with hhao020\@gmail.com for bug reporting and supporting.
*/

#include "csym.h"

SYM_DECLARE

Sym_t gSymTable[SYM_MAX_NUM] =
{
  //example of data and func, refer to cshell::Sym_t.
  // address              name         type          module    size

  //size=sizeof(data)
  //{&idata,              "idata",    SYM_DATA,       0,       4},  

  //size=lineno(last line of func) - lineno(first line of func)
  //{vfun_sic,            "vfun_sic", SYM_FUNC,       0,       56}, 

SYM_LINE

};

EOF

sub GenCStyleSymTable
{
  my $refTab = shift;
  
  my @list;
  my @declares;
  foreach my $name(sort keys %$refTab)
  {
    my $refSym = $refTab->{$name};
    
    next if not $refSym->{BIND};
    next if($refSym->{BIND} ne "GLOBAL");
    my $module = 0;

    my $type = 'SYM_FUNC';
    my $addr = $name;
    if($refSym->{TYPE} eq 'OBJECT')
    {
      $type = 'SYM_DATA';
      $addr = '&'.$name;
    }
    if($type eq 'SYM_DATA')
    {
      push @declares, sprintf("extern int $name;");
    }
    else
    {
      push @declares, sprintf("extern int $name();");
    }
    push @list, sprintf("  { %-64s %-64s %10s, %10s, %4d },", 
                    "$addr,", "\"$name\",", $type, $module, $refSym->{SIZE});  
  }

  return (\@declares, \@list);
} 
sub Main
{
  my @argv = @ARGV;
  
  my $code = c_sym_table;
  my $dec = '';
  my $line = '';

  my $count = 0;

  my $fn_out = 'c_sym_table.c';

  my $date = `date`;
  $code =~ s/DATETIME/$date/g;
  $code =~ s/FILENAME/$fn_out/g;

  for my $argv (@argv)
  {
    my $text = "$argv.sym.txt";
    `readelf --wide --symbols $argv > $text`;
    my $refSymTab = &read_symtab($text);
    next if not $refSymTab;

    my ($refDeclares, $refList) = &GenCStyleSymTable($refSymTab);

    my @declares = grep(!/@|\b(gSymTable|main|_init|_start)\b|\b(__lib|_IO_)/, @$refDeclares);
    $declares[$_] .= "  //".($count+$_) foreach(0..$#declares);  
    $dec .= join "\n", @declares;

    my @list = grep(!/@|\b(gSymTable|main|_init|_start)\b|\b(__lib|_IO_)/, @$refList);
    $list[$_] .= "  //".($count+$_) foreach(0..$#list);  
    $line .= join "\n", @list;

    $dec .= "\n\n";
    $line .= "\n\n";

    $count += @declares;    
  }
  
  $dec = sprintf("\nint gSymCount=%d;\n", $count).$dec if $dec;
  
  $code =~ s/SYM_DECLARE/$dec/mg;
  $code =~ s/SYM_LINE/$line/mg;
  &FileSave($fn_out, $code);
}

#[3041] 00000000100e9ad8      264       FUNC     GLOBAL zremote_encode_tunnel_inf
sub PrintSymTab
{
  my $refTab = shift;
  my $count = 0;
  foreach my $name(sort keys %$refTab)
  {
    my $refSym = $refTab->{$name};

    next if($refSym->{BIND} ne "GLOBAL");
    printf("[%4d] %16s %8s %10s %10s %s\n", 
                    ++$count, $refSym->{ADDRESS}, $refSym->{SIZE}, 
                    $refSym->{TYPE}, $refSym->{BIND}, $name);
  }
} 
sub MainTest
{  
  my $refSymTab = &read_symtab('elf.txt');

  &PrintSymTab($refSymTab);
}

sub read_symtab
{
  my $fname = shift;

  my $symDoc = &FileLoad($fname, '\'.symtab\'', '^\s*$', 0, 1);   
  #&DataDump(undef, $symDoc);
  
  my %symTab;
  
  #  34: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS d.c
  #  35: 0000000000601058     4 OBJECT  LOCAL  DEFAULT   24 stg
  #  36: 000000000060105c     4 OBJECT  LOCAL  DEFAULT   24 sa.2162
  #  37: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS   
  my $absDoc = &DocSliceList($symDoc, 'ABS\s+\S+\s*$', 'ABS.*$', 0);  
  #&DataDump(undef, $absDoc);
  
  foreach(@$absDoc)
  {
  	my $absBlock = $_;
  	next if(scalar @$absBlock < 3);
    next if not($absBlock->[0] =~ /FILE\s+LOCAL\s+DEFAULT\s+ABS\s+(\S+)\s*$/);
    my $fname = $1;
  	foreach my $line(@$absBlock[1..$#$absBlock-1])
  	{
      if($line =~ /(\d+)\s*\:\s*([\da-fA-F]+)\s+(\d+)\s+OBJECT\s+LOCAL\s+DEFAULT\s+\d+\s+(\S+)\s*$/)
      {
        my %abs = (ADDRESS=>$2, SIZE=>$3, TYPE=>'STATIC', BIND=>'LOCAL', FILE=>$fname);
        
        $symTab{"$4\.$1"} = \%abs;
      }
    }
  }
  #&DataDump(undef, \%symTab);

  #  45: 0000000000601028     0 NOTYPE  WEAK   DEFAULT   24 data_start
  #  55: 0000000000601028     0 NOTYPE  GLOBAL DEFAULT   24 __data_start
  my $refDataStart = &DocGrep($symDoc, 'data_start\s*$') || die "No data_start.";
  #&DataDump(undef, $refDataStart);  
  {
    foreach my $line (@$refDataStart)
    {
      next if not ($line =~ /^\s*\d+\s*\:\s+([\da-zA-Z]+)\s/);
      
      my %data_start = (ADDRESS=>$1, TYPE=>'NOTYPE', BIND=>'WEAK');
      $symTab{DATA_START} = \%data_start;

      last;
    }
  }
  #&DataDump(undef, \%symTab);

  my $data_start_addr = 0;
  $data_start_addr = hex($symTab{DATA_START}->{ADDRESS}) if $symTab{DATA_START}->{ADDRESS};

  #  49: 0000000000400628     4 OBJECT  GLOBAL DEFAULT   15 cd
  #  50: 0000000000400604     0 FUNC    GLOBAL DEFAULT   14 _fini 
  {
    foreach my $line (@$symDoc)
    {
      next if not ($line =~ /^\s*\d+\s*\:\s+([\da-zA-Z]+)\s+(\d+)\s+(OBJECT|FUNC)\s+(GLOBAL|LOCAL)\s+DEFAULT\s+\d+\s+(\S+)/);
	  
      my %sym = (ADDRESS=>$1, SIZE=>$2, TYPE=>$3, BIND=>$4);
      if($3 eq 'OBJECT')
      {
        $sym{TYPE} = 'CONST' if (hex($1) < $data_start_addr);
      }
      $symTab{$5} = \%sym;
    }
  }
  #&DataDump(undef, \%symTab);

  return \%symTab;
}


sub FileSave
{
  my $fn_out = shift;
  my $code = shift;

  open(WH,">$fn_out") or die "Failed to save file.\n";

  print WH "$code\n";

  close WH;
}


sub DocSlice
{
  my $refText = shift;
  my $begins = shift || "";
  my $ends   = shift || "";
  my $offset = shift || 0;

  my $a = scalar @$refText;
  my $b = $#$refText;

  foreach($offset..$#$refText)
  {
    my $num = $_;
    my $line = $refText->[$num];
    
    #print "slice $num : $line\n";
    if ($a > $num)
    {
      next if $begins && (not $line =~ /$begins/);
      $a = $num;
      #print "slice # offset: $offset a: $a\n";
      next;
    }
    
    if($ends && $line =~ /$ends/)
    {
      $b = $num;      
      #print "slice # offset: $offset a: $a b: $b\n";

      last;
    }
  }

  return ($a, $b);
}

sub DocSliceList
{
  my $refText = shift;
  my $begins = shift || "";
  my $ends   = shift || "";
  my $offset = shift || 0;

  my @list;
  while($offset < scalar @$refText)
  {
    my ($a, $b) = &DocSlice($refText, $begins, $ends, $offset);
    last if($a >= scalar @$refText);

    #print "DOC LIST: offset: $offset  a: $a  b: $b\n";

    my @slice = @$refText[$a..$b];
    push @list, \@slice;

    $offset = $b;
  }

  return \@list;
}
sub DocGrep
{
  my $refText = shift;
  my $targets = shift || "";
  my $offset = shift || 0;

  my @list;
  foreach my $num($offset..$#$refText)
  {
    my $line = $refText->[$num];

    next if $targets && (not $line =~ /$targets/);
    push @list, $line;
  }

  return \@list;
}
sub FileLoad
{
  my $fname = shift;
  my $begins = shift;
  my $ends   = shift;
  my $offset = shift;
  my $msects = shift;
  
  open(RH,$fname) or die "can't read $fname:$!";
  my @lines = <RH>;
  close RH;
  
  my $refData = \@lines;
    
  return $refData if not defined $begins;
  
  chomp @$refData;
  
  my @text;
  while(1)
  {
    my ($a, $b) = &DocSlice($refData, $begins, $ends, $offset);
    
    $offset = $b;
    if($a <= $b)
    {
      push @text, @$refData[$a..$b];
      next if $msects;
    }
    last;
  };

  return \@text;
}