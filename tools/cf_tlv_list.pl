#/usr/bin/perl
use strict;
use warnings;

use zFile;
use zTrace;

use zError;
use zDebug;

our $doc_tlv_list;

our $embeddedstructs='';
my $g_datetime = `date`;

our $doc_code_c;
our $doc_code_h;
our $doc_code_type;
our $doc_dec_byte;
our $doc_dec_word;
our $doc_dec_dword;
our $doc_dec_words;
our $doc_dec_string;
our $doc_dec_stringz;
our $doc_dec_bytes;
our $doc_dec_nbytes;
our $doc_dec_nest;
our $doc_dec_calltlv;  
our $doc_dec_calltlvs;
our $type_bytes;
our $type_words;
our $type_string;
our $type_stringz;
our $type_nbytes;


our $type_define;

&Main();

sub Main
{
  my $TREE = {};

  #&PrintTypeDefine($type_define);
  
  &SplitDocTlvList($TREE);  
  #&PrintTlvNest($TREE);  
  
  &PrecompileTlvNest($TREE); #fix minor errors in tlv list
  #&PrintTlvNest($TREE);
  
  my $prefix = 'Cf';

  my $h_enums = lc $prefix.'_enum_def.h';
  my $h_structs = lc $prefix.'_struct_def.h';
  my $h_funs = lc $prefix.'_codec_funs.h';
  my $c_funs = lc $prefix.'_codec_funs.c';

  my $doc_enums = &GenerateEnums($TREE, $prefix);
  #print $doc_enums;
  my $doc = $doc_code_h;
  $doc =~ s/CODELINES/$doc_enums/; 
  $doc =~ s/DATETIME/$g_datetime/;
  $doc =~ s/FILENAME/$h_enums/g;
  my $hm = uc $h_enums; $hm =~ s/\./_/g; $doc =~ s/HMACRO/$hm/g;
  &FileSave($h_enums, $doc);

  my $doc_structs = &GenerateStructs($TREE, $prefix, $type_define);
  #print $doc_structs;
  $doc = $doc_code_h;
  $doc =~ s/DATETIME/$g_datetime/;
  $doc =~ s/FILENAME/$h_structs/g;
  $doc =~ s/CODELINES/$doc_code_type\n\n$embeddedstructs\n\n$doc_structs/;
  $hm = uc $h_structs; $hm =~ s/\./_/g; $doc =~ s/HMACRO/$hm/g;
  &FileSave($h_structs, $doc);
  
  my ($doc_hfuns, $doc_cfuns) = &GenerateCode($TREE, $prefix, $type_define);
  #print $doc_hfuns;  
  $doc = $doc_code_h;
  $doc =~ s/CODELINES/$doc_hfuns/;
  $doc =~ s/DATETIME/$g_datetime/;
  $doc =~ s/FILENAME/$h_funs/g;
  $hm = uc $h_funs; $hm =~ s/\./_/g; $doc =~ s/HMACRO/$hm/g;
  &FileSave($h_funs, $doc);

  #print $doc_cfuns;
  $doc = $doc_code_c;
  $doc =~ s/CODELINES/$doc_cfuns/;
  $doc =~ s/DATETIME/$g_datetime/;  
  $doc =~ s/FILENAME/$c_funs/g;
  my $in = "#include \"$h_enums\"\n#include \"$h_structs\"\n#include \"$h_funs\"\n"; $doc =~ s/INCLUDELINES/$in/g;
  &FileSave($c_funs, $doc);
}

sub GenerateEnums
{
  my $tree = shift;
  my $prefix = shift || "";

  die "must provide preleading." unless $prefix;
  
  #print "GenerateEnums prefix $prefix\n";
  
  my @list;
  my @docs;
  foreach my $id (sort {$a <=> $b} keys %$tree)
  {
    my $name = $tree->{$id}->{name};
    my $node = $tree->{$id};
	
	#print "GenerateEnums name $name id $id\n";
    
    push @list, "  e$prefix$name = $id,";
    if($node->{nested})
    {
      push @docs, &GenerateEnums($node->{nested}, $prefix.$name);      
    }
  }

  my $doc = join("\n", "enum Enum$prefix", "{", @list, "};\n", @docs);

  return $doc;
}

sub TypeName
{
  my $type = shift;
  my $min = shift;
  my $max = shift;
  my $name = shift;
  
  return $name.'_t' if($type eq 'nested');
  my $st = $type_define->{$type}->{struct} || die "unknown type [$type].";

  return $type_define->{$type}->{struct} if ($type_define->{$type}->{struct} =~ /_t$/);
  
  $st =~ s/MAX/$max/;
  $st =~ s/NAME/\U$name/;
  $st =~ s/name/$name/;
  $embeddedstructs .= "$st\n";
  
  return $name.'_t';
}

sub MultiValue
{
  my $maximum = shift;
  return '' if $maximum==1;
  return "[$maximum]";
}
sub GenerateStructs
{
  my $tree = shift;
  my $prefix = shift || "";
  my $types = shift;

  die "must provide preleading." unless $prefix;

  my @presents;
  my @list;
  my @docs;
  foreach my $id (sort {$a <=> $b} keys %$tree)
  {
    my $node = $tree->{$id};
	my $name = $node->{name};

    next if($node->{type} eq 'no_value');
    next unless (defined $node->{maximum});
	next if($node->{type} eq "nested" && not $node->{nested});
    
    push @presents, "    dword_t m$name:1;";

    my $typename = &TypeName($node->{type}, $node->{limit}->[0], $node->{limit}->[1], $prefix.$name);
    my $multi = &MultiValue($node->{maximum});
    if($multi)
    {
      push @list, "  $typename m$name$multi; int m$name"."Count;";
    }
    else
    {
      push @list, "  $typename m$name;";
    }
    if($node->{nested})
    {
      push @docs, &GenerateStructs($node->{nested}, $prefix.$name, $types);      
    }
  }

  my $doc = join("\n", @docs, "typedef struct STRUCT_".uc($prefix)."_T\n{", "  struct {", @presents, "  } present;\n", @list, "} $prefix"."_t;\n");

  return $doc;
}
sub DecodeCall
{
  my $type = shift;
  my $name = shift;  
  my $enum = shift;
  my $arg = shift;
  my $multi = shift;
  my $support = shift;
  
  my $code = $doc_dec_calltlv;
  $code = $doc_dec_calltlvs if($multi);

  $multi =~ s/\[|\]//g;
  #my $ctype = '';
  #$ctype = "(".$type_define->{$type}->{ctype}." *)" if $type_define->{$type}->{ctype};
  #$code =~ s/CTYPE/$ctype/g;
  $code =~ s/NAME/$name/g;
  $code =~ s/ENUM/$enum/g;
  $code =~ s/ARG/$arg/g;  
  $code =~ s/COUNT/$multi/g;
  $code =~ s/SUPPORT/$support/g;

  return $code;
}

sub DecodeCode
{
  my $type = shift;
  my $min = shift;
  my $max = shift;
  my $name = shift;

  my $code;
  if($type eq 'nested')
  {
    return ("", "");
  }
  
  $code = $type_define->{$type}->{decode} || die "unknown type $type.";
  
  $code =~ s/NAME/$name/g;
  $code =~ s/TYPE/$type_define->{$type}->{CTYPE}/g;
  $code =~ s/MAX/$max/g;
  $code =~ s/MIN/$min/g;

  $code =~ /(.*?\))/;
  my $pro = "";
  $pro = "extern $1;" if $1;

  return ($pro, $code);
}

sub GenerateCode
{
  my $tree = shift;
  my $prefix = shift || "";
  
  die "must provide preleading." unless $prefix;

  my @pre;
  my @imp;

  my @calls;

  my $code = $doc_dec_nest;
  $code =~ s/NAME/$prefix/g;
  foreach my $id (sort {$a <=> $b} keys %$tree)
  {
    my $node = $tree->{$id};
	my $name = $node->{name};

    next if($node->{type} eq 'no_value');
    next unless (defined $node->{maximum});
	next if($node->{type} eq "nested" && not $node->{nested});

	#print "GenerateCode name $name\n";   
	
	push @calls, &DecodeCall($node->{type}, "$prefix$name", "e$prefix$name", "m$name", &MultiValue($node->{maximum}), $node->{support});
    if($node->{nested})
    {
	  #print "GenerateCode nested name $name\n";
      my ($h, $c) = &GenerateCode($node->{nested}, $prefix.$name);
      push @pre, $h;
      push @imp, $c;      
    }
    else    
    {
	  #print "GenerateCode code name $name\n";
      my ($h, $c) = &DecodeCode($node->{type}, $node->{limit}->[0], $node->{limit}->[1], $prefix.$name);
      push @pre, $h;
      push @imp, $c; 
    }
  }

  my $call = join("\n", @calls);
  $code =~ s/CODELINES/$call/g;
  #print "GenerateCode call $call code $code\n";
  
  $code =~ /(.*?\))/;
  my $hdoc = join("\n", @pre, "extern $1;", "");
  my $cdoc = join("\n", @imp, $code, "");

  return ($hdoc, $cdoc);
}

sub PrintTlvNest
{
  my $nest = shift;
  
  my $key;	
  foreach $key (sort {$a <=> $b} keys %$nest) #
  {
    &PrintTlvLeaf($nest->{$key});
  }
}

sub PrintTlvLeaf
{
  my $node = shift;
  
  #       tag   name   support type maximum range[]
  printf("%-12s  %-32s %-12s %-16s     %-2d   %16d ~ %d\n", $node->{code}, $node->{name}, $node->{support}, 
                          $node->{type}, $node->{maximum}, $node->{limit}->[0], $node->{limit}->[1]);  
  if($node->{"type"} eq "nested")
  {
    &PrintTlvNest($node->{"nested"});
  }
}

sub PrecompileTlvNest
{
  my $nest = shift;
  
  my $key;
  foreach $key (sort {$a <=> $b} keys %$nest) #
  {
    &PrecompileTlvLeaf($nest->{$key});
  }
}

sub PrecompileTlvLeaf
{
  my $node = shift;
  
  $node->{type} = "nested" if($node->{nested});
  
  if($node->{type} eq "nested")
  {
    $node->{type} = "hexstr" if not $node->{nested};
    &PrecompileTlvNest($node->{nested});
  }
}

sub SplitDocTlvList
{
  my $tree = shift;

  my @tlv_lines = split /\n/, $doc_tlv_list;

  foreach(0..$#tlv_lines)
  {
    my $line = $tlv_lines[$_];    
    #print "#$_   $line\n";

    $line =~ s/^\s+|\s+$//g;
    next if(not $line =~ /^\./);
    
    my @words = split /[\s\~]+/, $line;

    die "at least contains tag, name, support.\n" if(@words < 3); #
    
    my %leaf;		
	$leaf{type}    = "";
    $leaf{maximum} = "";
    $leaf{limit}   = "";	
	  
    $leaf{code}    = $words[0];
	$leaf{name}    = $words[1];
    $leaf{support} = $words[2];
    $leaf{type}    = $words[3] if defined $words[3];
    $leaf{maximum} = $words[4] if defined $words[4];
    $leaf{limit}   = [$words[5], $words[6]] if defined $words[6];
	
	&InsertTlv2Tree($tree, \%leaf);
  }
  
  return;
}

sub InsertTlv2Tree
{
  my $tree = shift;
  my $leaf   = shift;
  
  my @hi = $leaf->{code} =~ /\d+/g; 
  die "invalid leaf, ", $leaf->{code}, " -- ", $leaf->{name}, "\n" if @hi < 1;
  
  my $leaf_id = pop @hi;
  my $hi_code = "";
  foreach(@hi)
  {
    my $sub = $_;
	$hi_code = "$hi_code.$sub";
    $tree->{$sub} = { code => $hi_code, name => "Tag$sub", support => "may", type => "nested", maximum => 1, limit => [0, 0]} if(not defined $tree->{$sub});
	$tree->{type} = "nested" if $tree->{type} && $tree->{type} ne "nested";
	$tree->{$sub}->{nested} = {} if(not defined $tree->{$sub}->{nested});
    $tree = $tree->{$sub}->{nested};
  }
  
  $tree->{$leaf_id} = $leaf;
}


sub PrintTypeDefine
{
  my $td = shift;
  
  foreach my $key (keys %$td)
  {
    print "$key => ", $td->{$key}->{struct}, "\n";
  }
}


INIT {


$doc_code_c = <<EOF;
/*
FILE: FILENAME
This file was created at DATETIME
Auto-generated source. Don't change it manually.
Contact with hhao020\@gmail.com for bug reporting and supporting.
*/
#include "zType_Def.h"
#include "zTraceApi.h"


INCLUDELINES

enum ProvisionSupportEnum{
  mustProvision = 1,
  shouldProvision,
  mayProvision,
  mustnotProvision,
  shouldnotProvision,
};

#define ValidateProvison(tag, tlv, support) do{ \\
  if(tlv) { \\
    if(support == mustnotProvision || support == shouldnotProvision) { \\
      zTraceWarn("invalid tlv tag %d: must not or should not support\\n", tag); return -1; \\
    } \\
  } \\
}while(0)

CfTlv_t* CfTlvFind(byte_t *pBuffer, int nSize, byte_t tag)
{
  byte_t *ptr = pBuffer;
  
  if(!pBuffer)
  {
    zTraceError("invalid input null buffer.\\n");
    return 0;
  }

  while(ptr < pBuffer+nSize - 2)
  {
    CfTlv_t *tlv = (CfTlv_t *)ptr;

    
    if(tlv->tag == 0) //eCfTagPad
    {
      ptr ++;
      continue;
    }

    if(ptr + tlv->len + 2 > pBuffer+nSize)
    {
      zTraceWarn("invalid tlv buffer: tag: %d, len: %d.\\n", tlv->tag, tlv->len);
      return 0;
    }

    if(tag == tlv->tag) return tlv;
    
    ptr += 2 + tlv->len;
  }
  
  //if(*ptr == eCfTagEndOfData) return 0; //reach end

  return 0;
}

CODELINES

EOF

$doc_code_h = <<EOF;
/*
FILE: FILENAME
This file was created at DATETIME
Auto-generated source. Don't change it manually.
Contact with hhao020\@gmail.com for bug reporting and supporting.
*/
#ifndef HMACRO
#define HMACRO

#ifdef __cplusplus
    extern "C" {
#endif

#include "zType_Def.h"

CODELINES

#ifdef __cplusplus
}
#endif

#endif /*HMACRO*/
EOF

$doc_code_type = <<EOF;
typedef struct STRUCT_CfTLV_T
{
  byte_t tag;
  byte_t len;
  byte_t value[1];
} CfTlv_t;
EOF

$doc_dec_byte = <<EOF;
int NAMEDecode(byte_t *buf, int size, byte_t *value)
{
  long min = MIN;
  long max = MAX;

  if(size != 1) return -1;

  *value = *buf;
  if(*value < min || *value > max) return -1;

  return 0;
}
EOF
$doc_dec_word = <<EOF;
int NAMEDecode(byte_t *buf, int size, word_t *value)
{
  long min = MIN;
  long max = MAX;

  if(size != 2) return -1;

  *value = *(word_t*)buf;
  if(*value < min || *value > max) return -1;

  return 0;
}
EOF
$doc_dec_dword = <<EOF;
int NAMEDecode(byte_t *buf, int size, dword_t *value)
{
  long min = MIN;
  long max = MAX;

  if(size != 4) return -1;

  *value = *(dword_t*)buf;  
  if(*value < min || *value > max) return -1;

  return 0;
}
EOF

$doc_dec_words = <<EOF;
int NAMEDecode(byte_t *buf, int size, NAME_t *value)
{
  if(size&1) return -1;
  value->size = size >> 1;
  if(value->size < MIN || value->size > MAX) return -1;

  memcpy(value->data, buf, size);

  return 0;
}
EOF

$doc_dec_string = <<EOF;
int NAMEDecode(byte_t *buf, int size, NAME_t *value)
{
  value->size = size;
  if(size < MIN || size > MAX) return -1;

  memcpy(value->data, buf, size);
  return 0;
}
EOF
$doc_dec_stringz = <<EOF;
int NAMEDecode(byte_t *buf, int size, NAME_t *value)
{
  value->size = size;
  if(size < MIN || size > MAX) return -1;

  memcpy(value->data, buf, size);
  value->data[size+1] = 0; //ensure the last is 0

  return 0;
}
EOF
$doc_dec_bytes = <<EOF;
int NAMEDecode(byte_t *buf, int size, NAME_t *value)
{
  value->size = size;
  if(size < MIN || size > MAX) return -1;

  memcpy(value->data, buf, size);

  return 0;
}
EOF
$doc_dec_nbytes = <<EOF;
int NAMEDecode(byte_t *buf, int size, NAME_t *value)
{
  if(size < MIN || size > MAX) return -1;

  memcpy(value->data, buf, size);

  return 0;
}
EOF

$doc_dec_nest = <<EOF;
int NAMEDecode(byte_t *buf, int size, NAME_t *value)
{
CODELINES
  return 0;
}
EOF
$doc_dec_calltlv = <<EOF;  
  {    
    CfTlv_t* tlv = CfTlvFind(buf, size, ENUM);
    ValidateProvison(ENUM, tlv, SUPPORTProvision);
    if(tlv)
    {
      int ret = NAMEDecode(tlv->value, tlv->len, &value->ARG);
      if(ret < 0)
      {
        zTraceWarn("Failed to decode ENUM (ARG, tlv_len = %d).\\n", tlv->len);
        return -1;
      }
      value->present.ARG = 1;
    }
  }
EOF
$doc_dec_calltlvs = <<EOF;
  {
    int ret;
    byte_t *next = buf;
    int    left = size;
    while(left > 0)
    {
      CfTlv_t* tlv = CfTlvFind(next, left, ENUM);
      ValidateProvison(ENUM, tlv, SUPPORTProvision);

      if(!tlv) break;

      next = tlv->value + 2 + tlv->len;
      left = size - (next - buf);
      if(left < 0)
      {
        zTraceWarn("Failed to decode ENUM (ARG[%d], tlv_len = %d).\\n", value->ARGCount, tlv->len);
        return -1;
      }
      if(value->ARGCount >= COUNT)
      {
        zTraceWarn("Failed to decode ENUM (ARG[%d], overflow).\\n", value->ARGCount);
        return -1;
      }
      ret = NAMEDecode(tlv->value, tlv->len, &value->ARG[value->ARGCount]);
      if(ret < 0)
      {
        zTraceWarn("Failed to decode ENUM (ARG, tlv_len = %d).\\n", tlv->len);
        return -1;
      }
      value->ARGCount += 1;
      value->present.ARG = 1;
    }
  }
EOF

$type_bytes = <<EOF;
typedef struct STRUCT_NAME_T
{
  byte_t size;
  byte_t data[MAX];
} name_t;
EOF
$type_words = <<EOF;
typedef struct STRUCT_NAME_T
{
  byte_t size;
  word_t data[MAX];
} name_t;
EOF
$type_string = <<EOF;
typedef struct STRUCT_NAME_T
{
  byte_t size;
  char   data[MAX];
} name_t;
EOF
$type_stringz = <<EOF;
typedef struct STRUCT_NAME_T
{
  byte_t size;
  char data[MAX+1];
} name_t;
EOF
$type_nbytes = <<EOF;
typedef struct STRUCT_NAME_T
{
  byte_t data[MAX];
} name_t;
EOF


$type_define = {
  ushort_list => {struct => $type_words, decode => $doc_dec_words},

  snmp_object => {struct => $type_bytes, decode => $doc_dec_bytes},
  vendor      => {struct => $type_bytes, decode => $doc_dec_bytes},
  hexstr      => {struct => $type_bytes, decode => $doc_dec_bytes}, 

  string      => {struct => $type_string, decode => $doc_dec_string},
  stringz     => {struct => $type_stringz,decode => $doc_dec_stringz}, 

  mic    => {struct => $type_nbytes, decode => $doc_dec_nbytes}, 
  ether  => {struct => $type_nbytes, decode => $doc_dec_nbytes}, 
  ipv6    => {struct => $type_nbytes, decode => $doc_dec_nbytes}, 

  ip     => {struct => 'dword_t', decode => $doc_dec_dword}, 
  uint   => {struct => 'dword_t', decode => $doc_dec_dword}, 
  ushort => {struct => 'word_t',  decode => $doc_dec_word}, 
  uchar  => {struct => 'byte_t',  decode => $doc_dec_byte}, 

};

my $doc_tlv_list_1= <<EOF;
.0            TagPad                           must         no_value             1                   0 ~ 0
.1            DownstreamFrequency              mustnot      nested                 1                 0 ~ 64
.2            UpstreamChannelId                mustnot      uchar                1                   0 ~ 255
EOF

$doc_tlv_list = <<EOF;
.0            TagPad                           must         no_value             1                   0 ~ 0
.1            DownstreamFrequency              mustnot      uint                 1            88000000 ~ 860000000
.2            UpstreamChannelId                mustnot      uchar                1                   0 ~ 255
.3            NetworkAccess                    must         uchar                1                   0 ~ 1
.4            ClassOfService                   mustnot      nested               4                   0 ~ 0
.4.1          ClassID                          must         uchar                1                   1 ~ 16
.4.2          MaxRateDown                      must         uint                 1                   0 ~ 52000000
.4.3          MaxRateUp                        must         uint                 1                   0 ~ 10000000
.4.4          PriorityUp                       must         uchar                1                   0 ~ 7
.4.5          GuaranteedUp                     must         uint                 1                   0 ~ 10000000
.4.6          MaxBurstUp                       must         ushort               1                   0 ~ 65535
.4.7          PrivacyEnable                    must         uchar                1                   0 ~ 1
.5            ModemCapabilities                must         nested               1                   0 ~ 0
.5.1          ConcatenationSupport             must         uchar                1                   0 ~ 1
.5.2          ModemDocsisVersion               must         uchar                1                   0 ~ 2
.5.3          FragmentationSupport             must         uchar                1                   0 ~ 1
.5.4          PHSSupport                       must         uchar                1                   0 ~ 1
.5.5          IGMPSupport                      must         uchar                1                   0 ~ 1
.5.6          BaselinePrivacySupport           must         uchar                1                   0 ~ 1
.5.7          DownstreamSAIDSupport            must         uchar                1                   0 ~ 255
.5.8          UpstreamSIDSupport               must         uchar                1                   0 ~ 255
.5.12         DCCSupport                       must         uchar                1                   0 ~ 1
.6            CmMic                            may          mic                  1                   4 ~ 4
.7            CmtsMic                          may          mic                  1                   4 ~ 4
.9            SwUpgradeFilename                must         hexstr               1                   1 ~ 32
.10           SnmpWriteAccessControl           shouldnot    uint                 1                   0 ~ 1
.11           SnmpMibObject                    must         snmp_object          256                  1 ~ 255
.14           CpeMacAddress                    shouldnot    ether                1                   6 ~ 6
.17           BaselinePrivacy                  mustnot      nested               1                   0 ~ 0
.17.1         AuthTimeout                      mustnot      uint                 1                   1 ~ 30
.17.2         ReAuthTimeout                    mustnot      uint                 1                   1 ~ 30
.17.3         AuthGraceTime                    mustnot      uint                 1                   1 ~ 6047999
.17.4         OperTimeout                      mustnot      uint                 1                   1 ~ 10
.17.5         ReKeyTimeout                     mustnot      uint                 1                   1 ~ 10
.17.6         TEKGraceTime                     mustnot      uint                 1                   1 ~ 302399
.17.7         AuthRejectTimeout                mustnot      uint                 1                   1 ~ 600
.17.8         SAMapWaitTimeout                 mustnot      uint                 1                   1 ~ 10
.17.9         SAMapMaxRetries                  mustnot      uint                 1                   0 ~ 10
.18           MaxCPE                           must         uchar                1                   1 ~ 254
.19           TftpTimestamp                    mustnot      uint                 1                   0 ~ 2147483647 
.20           TftpModemAddress                 mustnot      ip                   1                   4 ~ 4
.21           SwUpgradeServer                  must         ip                   1                   4 ~ 4
.22           UsPacketClass                    must         nested               4                   0 ~ 0
.22.1         ClassifierRef                    must         uchar                1                   1 ~ 255
.22.3         ServiceFlowRef                   must         ushort               1                   1 ~ 65535
.22.5         RulePriority                     must         uchar                1                   1 ~ 255
.22.6         ActivationState                  shouldnot    uchar                1                   0 ~ 1
.22.7         DscAction                        must         uchar                1                   0 ~ 2
.22.9         IpPacketClassifier               must         nested               1                   0 ~ 0
.22.9.1       IpTos                            must         hexstr               1                   3 ~ 3
.22.9.2       IpProto                          must         ushort               1                   0 ~ 257
.22.9.3       IpSrcAddr                        must         ip                   1                   4 ~ 4
.22.9.4       IpSrcMask                        must         ip                   1                   4 ~ 4
.22.9.5       IpDstAddr                        must         ip                   1                   4 ~ 4
.22.9.6       IpDstMask                        must         ip                   1                   4 ~ 4
.22.9.7       SrcPortStart                     must         ushort               1                   0 ~ 65535
.22.9.8       SrcPortEnd                       must         ushort               1                   0 ~ 65535
.22.9.9       DstPortStart                     must         ushort               1                   0 ~ 65535
.22.9.10      DstPortEnd                       must         ushort               1                   0 ~ 65535
.22.10        LLCPacketClassifier              must         nested               1                   0 ~ 0
.22.10.1      DstMacAddress                    must         ether                1                   6 ~ 6
.22.10.2      SrcMacAddress                    must         ether                1                   6 ~ 6
.22.10.3      EtherTypeOrDsapOrMacType         must         hexstr               1                   2 ~ 2
.22.11        IEEE802Classifier                must         nested               1                   0 ~ 0
.22.11.1      UserPriority                     must         ushort               1                   0 ~ 65535
.22.11.2      VlanID                           must         ushort               1                   0 ~ 65535
.22.11        IEEE8021                         must         nested               1                   0 ~ 0
.22.11.1      UserPriority                     must         uchar                1                   0 ~ 1
.22.11.2      VlanID                           must         uchar                1                   0 ~ 1
.22.12        Ipv6Classifier                   must         nested               1                   0 ~ 0
.22.12.1      IpTcRngAndMask                   must         hexstr               1                   3 ~ 3
.22.12.2      IpFlowLable                      must         ushort               1                   0 ~ 257
.22.12.3      IpNextHdrType                    must         ip                   1                   0 ~ 257
.22.12.4      IpSrcAddr                        must         ipv6                 1                  16 ~ 16
.22.12.5      IpSrcPrefixLen                   must         ushort               1                   0 ~ 257
.22.12.6      IpDstAddr                        must         ipv6                 1                  16 ~ 16
.22.12.7      IpDstPrefixLen                   must         ushort               1                   0 ~ 257
.22.13        CmimEncoding                     must         hexstr               1                   0 ~ 16
.22.14        IEEE8021ad                       must         nested               1                   0 ~ 0
.22.14.1      S_TPID                           must         ushort               1                   0 ~ 65535
.22.14.2      S_VID                            must         ushort               1                   0 ~ 65535
.22.14.3      S_PCP                            must         uchar                1                   0 ~ 255
.22.14.4      S_DEI                            must         uchar                1                   0 ~ 255
.22.14.5      C_TPID                           must         ushort               1                   0 ~ 65535
.22.14.6      C_VID                            must         ushort               1                   0 ~ 65535
.22.14.7      C_PCP                            must         uchar                1                   0 ~ 255
.22.14.8      C_CFI                            must         uchar                1                   0 ~ 255
.22.14.9      S_TCI                            must         ushort               1                   0 ~ 65535
.22.14.10     C_TCI                            must         ushort               1                   0 ~ 65535
.22.15        IEEE8021ah                       must         nested               1                   0 ~ 0
.22.15.1      I_TPID                           must         ushort               1                   0 ~ 65535
.22.15.2      I_SID                            must         hexstr               1                   3 ~ 3
.22.15.3      I_TCI                            must         uint                 1                   0 ~ 2147483647
.22.15.4      I_PCP                            must         uchar                1                   0 ~ 255
.22.15.5      I_DEI                            must         uchar                1                   0 ~ 255
.22.15.6      I_UCA                            must         uchar                1                   0 ~ 255
.22.15.7      B_TPID                           must         ushort               1                   0 ~ 65535
.22.15.8      B_TCI                            must         ushort               1                   0 ~ 65535
.22.15.9      B_PCP                            must         uchar                1                   0 ~ 255
.22.15.10     B_DEI                            must         uchar                1                   0 ~ 255
.22.15.11     B_VID                            must         ushort               1                   0 ~ 65535
.22.15.12     B_DA                             must         ether                1                   6 ~ 6
.22.15.13     B_SA                             must         ether                1                   6 ~ 6
.22.16        ICMPv6                           must         ushort               1                   0 ~ 65535
.22.17        MplsTc                           must         nested               1                   0 ~ 0
.22.17.1      MplsTcBits                       must         uchar                1                   0 ~ 255
.22.17.2      MplsLable                        must         hexstr               1                   3 ~ 3
.22.43        VendorSpecific                   must         vendor               1                   1 ~ 255
.23           DsPacketClass                    must         nested               4                   0 ~ 0
.23.1         ClassifierRef                    must         uchar                1                   1 ~ 255
.23.3         ServiceFlowRef                   must         ushort               1                   1 ~ 65535
.23.5         RulePriority                     must         uchar                1                   1 ~ 255
.23.6         ActivationState                  shouldnot    uchar                1                   0 ~ 1
.23.7         DscAction                        must         uchar                1                   0 ~ 2
.23.9         IpPacketClassifier               must         nested               1                   0 ~ 0
.23.9.1       IpTos                            must         hexstr               1                   3 ~ 3
.23.9.2       IpProto                          must         ushort               1                   0 ~ 257
.23.9.3       IpSrcAddr                        must         ip                   1                   4 ~ 4
.23.9.4       IpSrcMask                        must         ip                   1                   4 ~ 4
.23.9.5       IpDstAddr                        must         ip                   1                   4 ~ 4
.23.9.6       IpDstMask                        must         ip                   1                   4 ~ 4
.23.9.7       SrcPortStart                     must         ushort               1                   0 ~ 65535
.23.9.8       SrcPortEnd                       must         ushort               1                   0 ~ 65535
.23.9.9       DstPortStart                     must         ushort               1                   0 ~ 65535
.23.9.10      DstPortEnd                       must         ushort               1                   0 ~ 65535
.23.10        LLCPacketClassifier              must         nested               1                   0 ~ 0
.23.10.1      DstMacAddress                    must         ether                1                   6 ~ 6
.23.10.2      SrcMacAddress                    must         ether                1                   6 ~ 6
.23.10.3      EtherType                        must         hexstr               1                   2 ~ 2
.23.11        IEEE802Classifier                must         nested               1                   0 ~ 0
.23.11.1      UserPriority                     must         ushort               1                   0 ~ 65535
.23.11.2      VlanID                           must         ushort               1                   0 ~ 65535
.23.11        IEEE8021                         must         nested               1                   0 ~ 0
.23.11.1      UserPriority                     must         uchar                1                   0 ~ 1
.23.11.2      VlanID                           must         uchar                1                   0 ~ 1
.23.12        Ipv6Classifier                   must         nested               1                   0 ~ 0
.23.12.1      IpTcRngAndMask                   must         hexstr               1                   3 ~ 3
.23.12.2      IpFlowLable                      must         ushort               1                   0 ~ 257
.23.12.3      IpNextHdrType                    must         ip                   1                   0 ~ 257
.23.12.4      IpSrcAddr                        must         ipv6                 1                  16 ~ 16
.23.12.5      IpSrcPrefixLen                   must         ushort               1                   0 ~ 257
.23.12.6      IpDstAddr                        must         ipv6                 1                  16 ~ 16
.23.12.7      IpDstPrefixLen                   must         ushort               1                   0 ~ 257
.23.13        CmimEncoding                     must         hexstr               1                   0 ~ 16
.23.14        IEEE8021ad                       must         nested               1                   0 ~ 0
.23.14.1      S_TPID                           must         ushort               1                   0 ~ 65535
.23.14.2      S_VID                            must         ushort               1                   0 ~ 65535
.23.14.3      S_PCP                            must         uchar                1                   0 ~ 255
.23.14.4      S_DEI                            must         uchar                1                   0 ~ 255
.23.14.5      C_TPID                           must         ushort               1                   0 ~ 65535
.23.14.6      C_VID                            must         ushort               1                   0 ~ 65535
.23.14.7      C_PCP                            must         uchar                1                   0 ~ 255
.23.14.8      C_CFI                            must         uchar                1                   0 ~ 255
.23.14.9      S_TCI                            must         ushort               1                   0 ~ 65535
.23.14.10     C_TCI                            must         ushort               1                   0 ~ 65535
.23.15        IEEE8021ah                       must         nested               1                   0 ~ 0
.23.15.1      I_TPID                           must         ushort               1                   0 ~ 65535
.23.15.2      I_SID                            must         hexstr               1                   3 ~ 3
.23.15.3      I_TCI                            must         uint                 1                   0 ~ 2147483647
.23.15.4      I_PCP                            must         uchar                1                   0 ~ 255
.23.15.5      I_DEI                            must         uchar                1                   0 ~ 255
.23.15.6      I_UCA                            must         uchar                1                   0 ~ 255
.23.15.7      B_TPID                           must         ushort               1                   0 ~ 65535
.23.15.8      B_TCI                            must         ushort               1                   0 ~ 65535
.23.15.9      B_PCP                            must         uchar                1                   0 ~ 255
.23.15.10     B_DEI                            must         uchar                1                   0 ~ 255
.23.15.11     B_VID                            must         ushort               1                   0 ~ 65535
.23.15.12     B_DA                             must         ether                1                   6 ~ 6
.23.15.13     B_SA                             must         ether                1                   6 ~ 6
.23.16        ICMPv6                           must         ushort               1                   0 ~ 65535
.23.17        MplsTc                           must         nested               1                   0 ~ 0
.23.17.1      MplsTcBits                       must         uchar                1                   0 ~ 255
.23.17.2      MplsLable                        must         hexstr               1                   3 ~ 3
.23.43        VendorSpecific                   must         nested               1                   0 ~ 0
.23.43.1      NetworkInfo                      must         nested               1                   0 ~ 0
.23.43.5.1    VpnIdentifier                    must         hexstr               1                   0 ~ 32
.23.43.8      GenExtInfo                       must         uint                 1            16777215 ~ 16777215
.24           UsServiceFlow                    must         nested               8                   0 ~ 0
.24.1         ServiceFlowRef                   must         ushort               1                   1 ~ 65535
.24.4         ServiceClassName                 must         stringz              1                   2 ~ 16
.24.6         QosParamSetType                  must         uchar                1                   0 ~ 255
.24.7         TrafficPriority                  must         uchar                1                   0 ~ 7
.24.8         MaxRateSustained                 must         uint                 1                   0 ~ 2147483647
.24.9         MaxTrafficBurst                  must         uint                 1                   0 ~ 2147483647
.24.10        MinReservedRate                  must         uint                 1                   0 ~ 2147483647
.24.11        MinAssumedPacketSize             shouldnot    ushort               1                   0 ~ 65535
.24.12        ActQosParamsTimeout              shouldnot    ushort               1                   0 ~ 65535
.24.13        AdmQosParamsTimeout              shouldnot    ushort               1                   0 ~ 65535
.24.14        MaxConcatenatedBurst             mustnot      ushort               1                   0 ~ 65535
.24.15        SchedulingType                   must         uchar                1                   0 ~ 6
.24.16        RequestOrTxPolicy                must         hexstr               1                   0 ~ 255
.24.17        NominalPollInterval              must         uint                 1                   0 ~ 2147483647
.24.18        ToleratedPollJitter              shouldnot    uint                 1                   0 ~ 2147483647
.24.19        UnsolicitedGrantSize             shouldnot    ushort               1                   0 ~ 65535
.24.20        NominalGrantInterval             shouldnot    uint                 1                   0 ~ 2147483647
.24.21        ToleratedGrantJitter             shouldnot    uint                 1                   0 ~ 2147483647
.24.22        GrantsPerInterval                shouldnot    uchar                1                   0 ~ 127
.24.23        IpTosOverwrite                   must         hexstr               1                   0 ~ 255
.24.26        MultiNumOfBytesReq               mustnot      hexstr               1                   0 ~ 255
.24.27        PeakTrafficRate                  shouldnot    uint                 1                   0 ~ 2147483647
.24.31        SfReqiredAttriMask               must         uint                 1                   0 ~ 2147483647
.24.32        SfForbidAttriMask                may          uint                 1                   0 ~ 2147483647
.24.33        SfAttriAggRuleMask               mustnot      uint                 1                   0 ~ 2147483647
.24.34        ApplicationIdentifier            shouldnot    uint                 1                   0 ~ 2147483647
.24.36        AggServiceFlowRef                must         ushort               1                   0 ~ 65535
.24.37        MespRef                          must         ushort               1                   0 ~ 65535
.24.41        DataRateUnitSetting              must         uint                 1                   0 ~ 2147483647
.24.43        VendorSpecific                   must         nested               1                   0 ~ 0
.24.43.1      NetworkInfo                      must         nested               1                   0 ~ 0
.24.43.5.1    VpnIdentifier                    must         hexstr               1                   0 ~ 32
.24.43.8      GenExtInfo                       must         uint                 1            16777215 ~ 16777215
.25           DsServiceFlow                    must         nested               8                   0 ~ 0
.25.1         ServiceFlowRef                   must         ushort               1                   1 ~ 65535
.25.2         ServiceFlowId                    must         uint                 1                   1 ~ 2147483647
.25.4         ServiceClassName                 must         stringz              1                   2 ~ 16
.25.6         QosParamSetType                  must         uchar                1                   0 ~ 255
.25.7         TrafficPriority                  must         uchar                1                   0 ~ 7
.25.8         MaxRateSustained                 must         uint                 1                   0 ~ 2147483647
.25.9         MaxTrafficBurst                  must         uint                 1                   0 ~ 2147483647
.25.10        MinReservedRate                  must         uint                 1                   0 ~ 2147483647
.25.11        MinAssumedPacketSize             shouldnot    ushort               1                   0 ~ 65535
.25.12        ActQosParamsTimeout              shouldnot    ushort               1                   0 ~ 65535
.25.13        AdmQosParamsTimeout              shouldnot    ushort               1                   0 ~ 65535
.25.14        MaxDsLatency                     shouldnot    uint                 1                   0 ~ 2147483647
.25.17        DsResequencing                   mustnot      uint                 1                   0 ~ 2147483647
.25.23        IpTosOverwrite                   must         hexstr               1                   0 ~ 255
.25.27        PeakTrafficRate                  shouldnot    uint                 1                   0 ~ 2147483647
.25.31        SfReqiredAttriMask               must         uint                 1                   0 ~ 2147483647
.25.32        SfForbidAttriMask                may          uint                 1                   0 ~ 2147483647
.25.33        SfAttriAggRuleMask               mustnot      uint                 1                   0 ~ 2147483647
.25.34        ApplicationIdentifier            shouldnot    uint                 1                   0 ~ 2147483647
.25.36        AggServiceFlowRef                must         ushort               1                   0 ~ 65535
.25.37        MespRef                          must         ushort               1                   0 ~ 65535
.25.41        DataRateUnitSetting              must         uint                 1                   0 ~ 2147483647
.25.43        DsVendorSpecific                 must         vendor               1                   1 ~ 255
.26           PHS                              mustnot      nested               1                   0 ~ 0
.26.1         PHSClassifierRef                 must         uchar                1                   1 ~ 255
.26.2         PHSClassifierId                  must         ushort               1                   1 ~ 65535
.26.3         PHSServiceFlowRef                must         ushort               1                   1 ~ 65535
.26.4         PHSServiceFlowId                 must         uint                 1                   1 ~ 2147483647
.26.7         PHSField                         must         hexstr               1                   1 ~ 255
.26.8         PHSIndex                         must         uchar                1                   1 ~ 255
.26.9         PHSMask                          must         hexstr               1                   1 ~ 255
.26.10        PHSSize                          must         uchar                1                   1 ~ 255
.26.11        PHSVerify                        must         uchar                1                   0 ~ 1
.28           MaxClassifiers                   shouldnot    ushort               1                   0 ~ 65535
.29           GlobalPrivacyEnable              must         uchar                1                   0 ~ 255
.32           MfgCVCData                       must         hexstr               1                   0 ~ 32
.33           CosgnCVCData                     must         hexstr               1                   0 ~ 32
.34           SnmpV3Kickstart                  should       nested               1                   0 ~ 0
.34.1         SnmpV3SecurityName               should       string               1                   1 ~ 16
.34.2         SnmpV3MgrPublicNumber            should       hexstr               1                   1 ~ 514
.35           SubMgmtControl                   must         hexstr               1                   3 ~ 3
.36           SubMgmtCpeTable                  must         hexstr               1                   0 ~ 32
.37           SubMgmtFilters                   must         ushort_list          1                   4 ~ 4
.38           SnmpV3TrapReceiver               must         nested               1                   0 ~ 0
.38.1         SnmpV3TrapRxIP                   must         ip                   1                   4 ~ 4
.38.2         SnmpV3TrapRxPort                 must         ushort               1                   0 ~ 65535
.38.3         SnmpV3TrapRxType                 must         ushort               1                   1 ~ 5
.38.4         SnmpV3TrapRxTimeout              must         ushort               1                   0 ~ 65535
.38.5         SnmpV3TrapRxRetries              must         ushort               1                   0 ~ 65535
.38.6         SnmpV3TrapRxFilterParameters     must         ushort               1                   1 ~ 5
.38.7         SnmpV3TrapRxSecurityName         must         string               1                   1 ~ 16
.38.8         SnmpV3TrapRxIPv6                 must         ipv6                 1                  16 ~ 16
.39           DocsisTwoEnable                  mustnot      uchar                1                   0 ~ 1
.40           TestMode                         mustnot      hexstr               1                   0 ~ 1
.41           DsChannelList                    mustnot      nested               1                   1 ~ 255
.41.1         SingleDsChannel                  must         nested               1                   1 ~ 255
.41.1.1       SingleDsTimeout                  must         ushort               1                   0 ~ 65535
.41.1.2       SingleDsFrequency                must         uint                 1                   0 ~ 2147483647
.41.2         DsFreqRange                      must         nested               1                   1 ~ 255
.41.2.1       DsFreqRangeTimeout               must         ushort               1                   0 ~ 65535
.41.2.2       DsFreqRangeStart                 must         uint                 1                   0 ~ 2147483647
.41.2.3       DsFreqRangeEnd                   must         uint                 1                   0 ~ 2147483647
.41.2.4       DsFreqRangeStepSize              must         uint                 1                   0 ~ 2147483647
.41.3         DefaultScanTimeout               must         ushort               1                   0 ~ 65535
.42           StaticMcMac                      should       ether                1                   6 ~ 6
.43           VendorSpecific                   must         nested               1                   0 ~ 0
.43.1         CmLbpID                          mustnot      uint                 1                   0 ~ 2147483647
.43.2         CmLbpPriority                    mustnot      uchar                1                   0 ~ 255
.43.3         CmLbpGroupID                     mustnot      uint                 1                   0 ~ 2147483647
.43.4         CmRangingClassIDExt              mustnot      uint                 1                   0 ~ 2147483647
.43.5         L2vpnEncoding                    must         nested               1                   0 ~ 0
.43.5.1       VpnIdentifier                    must         uint                 1                   0 ~ 2147483647
.43.5.2       NsiEncapSubtype                  must         nested               1                   0 ~ 0
.43.5.2.1     Other                            shouldnot    uchar                1                   0 ~ 255
.43.5.2.2     IEEE8021q                        must         uchar                1                   0 ~ 255
.43.5.2.3     IEEE8021ad                       must         uchar                1                   0 ~ 255
.43.5.2.4     MplsPeer                         must         uchar                1                   0 ~ 255
.43.5.2.5     L2TPv3Peer                       shouldnot    uchar                1                   0 ~ 255
.43.5.2.6     IEEE8021ah                       must         nested               1                   0 ~ 0
.43.5.2.6.1   I_TCI                            must         uchar                1                   0 ~ 255
.43.5.2.6.2   B_DA                             must         uchar                1                   0 ~ 255
.43.5.2.6.3   B_TCI                            must         uchar                1                   0 ~ 255
.43.5.2.6.4   I_TPID                           must         uchar                1                   0 ~ 255
.43.5.2.6.5   I_PCP                            must         uchar                1                   0 ~ 255
.43.5.2.6.6   I_DEI                            must         uchar                1                   0 ~ 255
.43.5.2.6.7   I_UCA                            must         uchar                1                   0 ~ 255
.43.5.2.6.8   I_SID                            must         uchar                1                   0 ~ 255
.43.5.2.6.9   B_TPID                           must         uchar                1                   0 ~ 255
.43.5.2.6.10  B_PCP                            must         uchar                1                   0 ~ 255
.43.5.2.6.11  B_DEI                            must         uchar                1                   0 ~ 255
.43.5.2.6.12  B_VID                            must         uchar                1                   0 ~ 255
.43.5.2.8     IEEE8021adS_TPID                 must         ushort               1                   0 ~ 65535
.43.5.3       EnableEsafeDhcpSnooping          must         hexstr               1                   1 ~ 16
.43.5.4       CMInterfaceMask                  must         hexstr               1                   1 ~ 16
.43.5.5       AttachmentGroupID                shouldnot         hexstr               1                   1 ~ 16
.43.5.6       SrcAttachmentIndividualID        shouldnot         hexstr               1                   1 ~ 16
.43.5.7       TargetAttachmentIndividualID     shouldnot         hexstr               1                   1 ~ 16
.43.5.8       IngressUserPriority              shouldnot         hexstr               1                   1 ~ 16
.43.5.9       UserPriorityRange                shouldnot         hexstr               1                   1 ~ 16
.43.5.10      L2vpnSaDescriptor                mustnot         hexstr               1                   1 ~ 16
.43.5.13      L2vpnMode                        must         hexstr               1                   1 ~ 16
.43.5.14      DpoeTpidTranslation              must         hexstr               1                   1 ~ 16
.43.5.14.1    UsOutmostTpidTranslation         must         hexstr               1                   1 ~ 16
.43.5.14.2    DsOutmostTpidTranslation         must         hexstr               1                   1 ~ 16
.43.5.14.3    UsS_TpidTranslation              must         hexstr               1                   1 ~ 16
.43.5.14.4    DsS_TpidTranslation              must         hexstr               1                   1 ~ 16
.43.5.14.5    UsB_TpidTranslation              must         hexstr               1                   1 ~ 16
.43.5.14.6    DsB_TpidTranslation              must         hexstr               1                   1 ~ 16
.43.5.14.7    UsI_TpidTranslation              must         hexstr               1                   1 ~ 16
.43.5.14.8    DsI_TpidTranslation              must         hexstr               1                   1 ~ 16
.43.5.15.1    L2cpTunnelMode                   must         hexstr               1                   1 ~ 16
.43.5.15.2    L2cpDaMac                        must         hexstr               1                   1 ~ 16
.43.5.15.3    L2cpReplacingDaMac               must         hexstr               1                   1 ~ 16
.43.5.16      DacDisableOrEnable               must         hexstr               1                   1 ~ 16
.43.5.18      PseudowireClass                  must         hexstr               1                   1 ~ 16
.43.5.19      ServiceDelimiter                 must         nested               1                   1 ~ 16
.43.5.19.1    C_VID                            must         hexstr               1                   1 ~ 16
.43.5.19.2    S_VID                            must         hexstr               1                   1 ~ 16
.43.5.19.3    I_SID                            must         hexstr               1                   1 ~ 16
.43.5.19.4    B_VID                            must         hexstr               1                   1 ~ 16
.43.5.20.1    VplsClass                        must         hexstr               1                   1 ~ 16
.43.5.20.2    E_TreeRole                       must         hexstr               1                   1 ~ 16
.43.5.20.3    E_TreeRootVID                    must         hexstr               1                   1 ~ 16
.43.5.20.4    E_TreeLeafVID                    must         hexstr               1                   1 ~ 16
.43.5.21.1    RouteDistinguisher               must         hexstr               1                   1 ~ 16
.43.5.21.2    ImportRouteTarget                must         hexstr               1                   1 ~ 16
.43.5.21.3    ExportRouteTarget                must         hexstr               1                   1 ~ 16
.43.6         ExtCmtsMicCfgSetting             may          hexstr               1                   0 ~ 1
.43.7         SavEncoding                      must         nested               1                   0 ~ 0
.43.7.1       SavGroupNameSubType              must         uchar                1                   0 ~ 255
.43.7.2       SavStaticPrefixRuleSubtype       must         nested               1                   0 ~ 0
.43.7.2.1     SavStaticPrefixAddressSubtype    must         hexstr               1                   0 ~ 32
.43.7.2.2     SavStaticPrefixLengthSubtype     must         uchar                1                   0 ~ 255
.43.8         GeneralExtInfo                   must         hexstr               1                   0 ~ 1
.43.9         CmAttriMasks                     mustnot      hexstr               1                   0 ~ 1
.43.10        IpMcJoinAuthEncoding             must         hexstr               1                   0 ~ 1
.43.11        ServiceTypeIdentifier            shouldnot    hexstr               1                   0 ~ 1
.45           DutFiltering                     shouldnot    nested               1                   0 ~ 0
.45.1         DutCtrl                          shouldnot    ushort               1                   0 ~ 65535
.45.2         DutCmim                          shouldnot    ushort               1                   0 ~ 65535
.53           Snmpv1v2Coexistence              must         nested               1                   0 ~ 0
.53.1         SnmpV1v2CommunityName            must         string               1                   1 ~ 32
.53.2         Snmpv1v2TransAddrAccess          must         nested               1                   1 ~ 32
.53.2.1       SnmpV1v2TransAddr                must         string               1                   1 ~ 32
.53.2.2       SnmpV1v2TransAddrMask            must         string               1                   1 ~ 32
.53.3         SnmpV1v2AccessViewType           must         hexstr               1                   1 ~ 16
.53.4         SnmpV1v2AccessViewName           must         hexstr               1                   1 ~ 16
.54           Snmpv3AccessView                 must         nested               1                   0 ~ 0
.54.1         Snmpv3AccessViewName             must         string               1                   1 ~ 32
.54.2         Snmpv3AccessViewSubtree          must         hexstr               1                   0 ~ 0
.54.3         Snmpv3AccessViewMask             must         string               1                   1 ~ 32
.54.4         Snmpv3AccessViewType             must         string               1                   1 ~ 32
.55           SnmpCpeAccessControl             shouldnot    uchar                1                   0 ~ 1
.56           ChannelAssignment                mustnot      uint                 1                   0 ~ 1
.58           SwUpgradeIPv6TftpSvr             must         string               1                   0 ~ 64
.59           TftpSvrProvModemIPv6             must         string               1                   0 ~ 64
.60           UsDropClass                      must         nested               4                   0 ~ 0
.60.1         ClassifierRef                    must         uchar                1                   1 ~ 255
.60.5         RulePriority                     must         uchar                1                   0 ~ 255
.60.9         IpPacketClassifier               must         nested               1                   0 ~ 0
.60.9.1       IpTos                            must         hexstr               1                   3 ~ 3
.60.9.2       IpProto                          must         ushort               1                   0 ~ 257
.60.9.3       IpSrcAddr                        must         ip                   1                   4 ~ 4
.60.9.4       IpSrcMask                        must         ip                   1                   4 ~ 4
.60.9.5       IpDstAddr                        must         ip                   1                   4 ~ 4
.60.9.6       IpDstMask                        must         ip                   1                   4 ~ 4
.60.9.7       SrcPortStart                     must         ushort               1                   0 ~ 65535
.60.9.8       SrcPortEnd                       must         ushort               1                   0 ~ 65535
.60.9.9       DstPortStart                     must         ushort               1                   0 ~ 65535
.60.9.10      DstPortEnd                       must         ushort               1                   0 ~ 65535
.60.10        LLCPacketClassifier              must         nested               4                   0 ~ 0
.60.10.1      DstMacAddress                    must         ether                1                   6 ~ 6
.60.10.2      SrcMacAddress                    must         ether                1                   6 ~ 6
.60.10.3      EtherTypeOrDsapOrMacType         must         hexstr               1                   2 ~ 2
.60.11        IEEE802Classifier                must         nested               1                   0 ~ 0
.60.11.1      UserPriority                     must         ushort               1                   0 ~ 65535
.60.11.2      VlanID                           must         ushort               1                   0 ~ 65535
.60.12        Ipv6Classifier                   must         nested               1                   0 ~ 0
.60.12.1      IpTcRngAndMask                   must         hexstr               1                   3 ~ 3
.60.12.2      IpFlowLable                      must         ushort               1                   0 ~ 257
.60.12.3      IpNextHdrType                    must         ip                   1                   0 ~ 257
.60.12.4      IpSrcAddr                        must         ipv6                 1                  16 ~ 16
.60.12.5      IpSrcPrefixLen                   must         ushort               1                   0 ~ 257
.60.12.6      IpDstAddr                        must         ipv6                 1                  16 ~ 16
.60.12.7      IpDstPrefixLen                   must         ushort               1                   0 ~ 257
.60.13        CmimEncoding                     must         hexstr               1                   0 ~ 16
.60.14        IEEE8021ad                       must         nested               1                   0 ~ 0
.60.14.1      S_TPID                           must         ushort               1                   0 ~ 65535
.60.14.2      S_VID                            must         ushort               1                   0 ~ 65535
.60.14.3      S_PCP                            must         uchar                1                   0 ~ 255
.60.14.4      S_DEI                            must         uchar                1                   0 ~ 255
.60.14.5      C_TPID                           must         ushort               1                   0 ~ 65535
.60.14.6      C_VID                            must         ushort               1                   0 ~ 65535
.60.14.7      C_PCP                            must         uchar                1                   0 ~ 255
.60.14.8      C_CFI                            must         uchar                1                   0 ~ 255
.60.14.9      S_TCI                            must         ushort               1                   0 ~ 65535
.60.14.10     C_TCI                            must         ushort               1                   0 ~ 65535
.60.15        IEEE8021ah                       must         nested               1                   0 ~ 0
.60.15.1      I_TPID                           must         ushort               1                   0 ~ 65535
.60.15.2      I_SID                            must         hexstr               1                   3 ~ 3
.60.15.3      I_TCI                            must         uint                 1                   0 ~ 2147483647
.60.15.4      I_PCP                            must         uchar                1                   0 ~ 255
.60.15.5      I_DEI                            must         uchar                1                   0 ~ 255
.60.15.6      I_UCA                            must         uchar                1                   0 ~ 255
.60.15.7      B_TPID                           must         ushort               1                   0 ~ 65535
.60.15.8      B_TCI                            must         ushort               1                   0 ~ 65535
.60.15.9      B_PCP                            must         uchar                1                   0 ~ 255
.60.15.10     B_DEI                            must         uchar                1                   0 ~ 255
.60.15.11     B_VID                            must         ushort               1                   0 ~ 65535
.60.15.12     B_DA                             must         ether                1                   6 ~ 6
.60.15.13     B_SA                             must         ether                1                   6 ~ 6
.60.16        ICMPv6                           must         ushort               1                   0 ~ 65535
.60.17        MplsTc                           must         nested               1                   0 ~ 0
.60.17.1      MplsTcBits                       must         uchar                1                   0 ~ 255
.60.17.2      MplsLable                        must         hexstr               1                   3 ~ 3
.60.43        VendorSpecific                   must         vendor               1                   1 ~ 255
.61           SubMgmtCpeIPv6PrefixList         must         hexstr               1                   0 ~ 64
.62           UsDropClassGroupID               must         uint                 1                   0 ~ 2147483647
.63           SubMgmtCtrlMaxCpeIPv6Prefix      must         hexstr               1                   0 ~ 64
.64           CmtsStaticMcSessionEncoding      must         hexstr               1                   0 ~ 64
.65           L2vpnMacAgingEncoding            mustnot      nested               1                   0 ~ 0
.65.1         L2vpnMacAgingMode                must         uchar                1                   1 ~ 255
.66           MgmtEventCtrlEncoding            shouldnot    hexstr               1                   0 ~ 64
.67           SubMgmtCpeIPv6List               must         hexstr               1                   0 ~ 64
.70           UsAggregateServiceFlow           must         nested               4                   0 ~ 0
.70.1         ServiceFlowRef                   must         ushort               1                   1 ~ 65535
.70.37        MespRef                          must         ushort               1                   1 ~ 65535
.70.41        DataRateUnitSetting              must         uint                 1                   0 ~ 2147483647
.71           DsAggregateServiceFlow           must         nested               1                   0 ~ 0
.71.1         ServiceFlowRef                   must         ushort               1                   1 ~ 65535
.71.37        MespRef                          must         ushort               1                   1 ~ 65535
.71.41        DataRateUnitSetting              must         uint                 1                   0 ~ 2147483647
.72           MetroEthernetServiceProfile      must         nested               1                   0 ~ 0
.73           NetworkTimingProfile             must         nested               1                   0 ~ 0
.79           UNICtrlEncodings                 must         hexstr               1                   0 ~ 64
.255          TagEnd                           must         no_value             1                   0 ~ 0

EOF

}

